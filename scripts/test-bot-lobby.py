#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run a sustained 128-bot native/QVM smoke test using local owned Quake data."""
import argparse
from collections import Counter
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('native', 'qvm'), default='native')
    parser.add_argument('--duration', type=int, default=60)
    parser.add_argument('--map', default='q3dm7')
    args = parser.parse_args()
    if args.duration < 10:
        parser.error('duration must be at least 10 seconds')
    build = ROOT/'build/client/RelWithDebInfo'
    if not (build/'ioq3ded').exists() or not (ROOT/'assets/baseq3/pak0.pk3').exists():
        parser.error('build the client and supply assets/baseq3/pak0.pk3 first')
    logs = ROOT/'build/tests'; logs.mkdir(parents=True, exist_ok=True)
    log_path = logs/f'bot-lobby-{args.mode}.log'
    with tempfile.TemporaryDirectory(prefix='qce-bot-lobby-') as temporary:
        home = Path(temporary); game = home/'baseq3'; (game/'vm').mkdir(parents=True)
        for source in (build/'baseq3').glob('*.so'):
            shutil.copyfile(source, game/source.name)
        shutil.copyfile(build/'baseq3/vm/qagame.qvm', game/'vm/qagame.qvm')
        command = [str(build/'ioq3ded'), '+set', 'fs_basepath', str(ROOT/'assets'),
                   '+set', 'fs_homepath', str(home), '+set', 'sv_pure', '0',
                   '+set', 'vm_game', '0' if args.mode == 'native' else '2',
                   '+set', 'dedicated', '1', '+set', 'sv_master1', '',
                   '+set', 'sv_maxclients', '128', '+set', 'g_qceCombat', '1',
                   '+set', 'g_qceMovement', '1', '+set', 'bot_nochat', '1',
                   '+set', 'gv_scoreLimit', '0', '+set', 'gv_primaryWeapon', 'random',
                   '+set', 'gv_secondaryWeapon', 'random', '+set', 'gv_mapWeaponSet', 'random',
                   '+devmap', args.map]
        with log_path.open('w') as output:
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=output,
                                       stderr=output, text=True)
            def send(text):
                if process.poll() is not None:
                    raise RuntimeError(f'Server exited unexpectedly: {process.returncode}')
                process.stdin.write(text+'\n'); process.stdin.flush()
            try:
                time.sleep(5); send('bot_add 0 128')
                for _ in range((args.duration+4)//5):
                    time.sleep(5)
                for text in ('status', 'game_memory', 'entitylist', 'dumpuser 0', 'sv_cheats 0',
                             'sv_cheats', 'sv_cheats 1', 'gv_save lobby', 'gv_movespeed 0.5'):
                    send(text)
                time.sleep(1); send('gv_load lobby'); send('gv_movespeed'); send('map_restart 0')
                time.sleep(10); send('status'); send('quit'); process.wait(timeout=20)
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill(); process.wait()
        text = log_path.read_text(errors='replace')
        assert process.returncode == 0, f'server exit {process.returncode}: {log_path}'
        assert not re.search(r'(?:ERROR:|Fatal:|FATAL:|no free entities|read only)', text), log_path
        connected = re.findall(r'ClientUserinfoChanged: (\d+) n\\Spartan (\d+)\\.*?\\skill\\([1-5])\.00', text)
        first = connected[:128]
        assert len(first) == 128 and {int(n) for _, n, _ in first} == set(range(1, 129)), log_path
        assert len({skill for _, _, skill in first}) > 1, 'random skill was reused for the entire batch'
        assert text.count('Spartan 128') >= 2 and 'qce_colorRGB' in text, log_path
        assert 'Saved variants/lobby.cfg' in text and 'Loaded variants/lobby.cfg' in text, log_path
        assert '"gv_movespeed" is:"1' in text and '"sv_cheats" is:"0' in text, log_path
        assert 'Server quit' in text and 'Game memory status:' in text, log_path
        entities = [int(n) for n in re.findall(r'^\s*(\d+):ET_', text, re.MULTILINE)]
        skills = Counter(int(skill) for _, _, skill in first)
        print(f'PASS: {args.mode}, 128 independently randomized Spartan bots, sustained combat, '
              f'map restart, editable cheats and variant roundtrip; skills={dict(sorted(skills.items()))}; '
              f'highest entity={max(entities, default=0)}; log={log_path}')

if __name__ == '__main__':
    main()
