#!/usr/bin/env python3
"""Apply supported scalar fields; retain explicit provenance and parity gaps."""
import argparse,copy,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SCALE=80.0 # .7 Halo standing collision height -> existing 56-unit Quake hull.
KINDS={'WP_MACHINEGUN':'bullet','WP_SHOTGUN':'shotgun','WP_ROCKET_LAUNCHER':'rocket','WP_RAILGUN':'rail','WP_PLASMAGUN':'plasma','WP_LIGHTNING':'plasma','WP_BFG':'bullet','WP_GRENADE_LAUNCHER':'plasma'}
def ms(seconds):return round(seconds*1000)
def import_profile(report,profile):
 p=copy.deepcopy(profile);p['schema_version']=2;p['status']='mixed'
 p['provenance']={'reference_build':report['map']['build'],'region':'NTSC-U build identifier','source_maps':[report['map']],'source_tag_hashes':[], 'notes':'Supported scalar values extracted from uploaded Xbox cache. Conversions and incomplete behaviors are listed in data/halo-import-report.json; retail parity is not validated.'}
 p['units']['halo_to_quake_scale']=SCALE
 audit={'source_map':report['map'],'scale':SCALE,'scale_basis':'0.7 world-unit standing hull mapped to 56 existing Quake units; chosen adaptation scale, not an official Quake conversion','tick_rate':30,'weapons':{},'player':{},'remaining':['plasma charged tracking/acquisition and animation events; exact fractional battery age','Needler target acquisition/material response and exact supercombine scheduling','damage ranges/falloff/material-response details and integer damage rounding','projectile acceleration/gravity/range and finite bullet travel','spread growth/recovery and recoil animation','full/empty reload animation state, weapon ready/recovery timing','melee damage ranges, moving strike modifiers and animation timing','movement acceleration interpretation, slopes and collision/camera transitions','grenade throw impulse/material bounce response, spawn counts and 10-second flight safety expiry','retail multiplayer and latency parity validation','Quake placeholder art/audio remains']}
 for w in p['weapons'].values():
  w.update(ammo_initial=0,ammo_max=200,reload_rounds=w['magazine'],tracking_radians=0.0,projectile_range=0.0,attachment_ms=0,combine_count=0,combine_damage=0,combine_radius=0.0,heat_per_shot=0,heat_loss_per_second=0,heat_recovery=0,heat_overheat=0,heat_age_penalty=0,charge_ms=0,charged_ammo=0,charged_heat=0,charged_damage=0,charged_speed=0.0,charged_range=0.0,charged_tracking_radians=0.0,charged_health_multiplier=0.0,charged_shield_multiplier=0.0)
 for slot,source in report['weapons'].items():
  w=p['weapons'][slot];v=source['values'];t=v['triggers'][0];projectile=source['projectiles'][0];pv=projectile['values'];mag=v['magazines'][0]
  name=source['path'].split('\\')[-1];w['name']='halo_'+name.replace(' ','_');w['fire_kind']=KINDS[slot];w['headshot_mode']=2 if slot=='WP_RAILGUN' else 1 if slot=='WP_BFG' else 0
  converted={};assumptions=[]
  for tag in [source,projectile,source['melee'],source['animations']]:
   if tag:p['provenance']['source_tag_hashes'].append({'path':tag['path'],'class':tag['class'],'root_sha256':tag['root_sha256']})
  if mag['rounds loaded maximum']:
   w['magazine']=mag['rounds loaded maximum'];w['ammo_initial']=mag['rounds total initial'];w['ammo_max']=mag['rounds loaded maximum']+mag['rounds reserved maximum'];w['reload_rounds']=mag['rounds reloaded']
   if mag['reload time']>0:w['reload_ms']=ms(mag['reload time'])
   else:
    animation=next(a for a in source['animations']['values']['animations'] if a['name']=='first-person reload-empty')
    w['reload_ms']=round(animation['frame count']*1000/30);assumptions.append('Zero reload-time tag: provisional empty reload animation duration at 30 frames/s, not verified runtime reload completion.')
  else:
   # Shot-equivalent battery inventory; does not claim Halo battery/heat behavior.
   age=t['age generated per round'];w['magazine']=round(1/age) if age>0 else 100;w['ammo_initial']=w['ammo_max']=w['magazine'];w['reload_rounds']=0
   assumptions.append('Battery represented as shot-equivalent inventory from age-per-round; Heat uses imported per-shot/loss/threshold scalars; Charged consumption uses normal-shot equivalents and clamps at the remaining battery.')
  if slot in ('WP_PLASMAGUN','WP_LIGHTNING'):
   w.update(heat_per_shot=round(t['heat generated per round']*10000),heat_loss_per_second=round(v['heat loss rate']*10000),heat_recovery=round(v['heat recovery threshold']*10000),heat_overheat=round(v['overheated threshold']*10000),heat_age_penalty=round(v['age heat recovery penalty']*1000))
  if slot=='WP_LIGHTNING':
   ct=v['triggers'][1];cp=source['projectiles'][1];cd=cp['damage_effects']['impact damage'];cv=cp['values'];dvcharge=cd['values']
   if dvcharge['damage side effect']!=3:raise ValueError('Charged plasma damage must have the supported EMP side effect')
   w.update(charge_ms=ms(t['charging time']),charged_ammo=round(ct['age generated per round']/t['age generated per round']),charged_heat=round(ct['heat generated per round']*10000),charged_damage=round(dvcharge['damage upper bound'][0]),charged_speed=cv['initial velocity']*30*SCALE,charged_range=cv['maximum range']*SCALE,charged_tracking_radians=cv['guided angular velocity'],charged_health_multiplier=dvcharge['cyborg armor'],charged_shield_multiplier=dvcharge['cyborg energy shield'])
   for tag in [cp,cd]:p['provenance']['source_tag_hashes'].append({'path':tag['path'],'class':tag['class'],'root_sha256':tag['root_sha256']})
   assumptions.append('Release before 600 ms fires primary bolt; full charge fires secondary bolt with EMP shield depletion. Behavior follows build-2342 reference. Charge pause/cancellation, aim cone, constant speed and shot-equivalent battery rounding await retail validation.')
  rate=t['maximum rate of fire'][1]
  w['fire_ms']=round(math.ceil(30/rate-1e-6)*1000/30) if rate>0 else round(1000/30)
  if t['maximum rate of fire'][0]!=rate:assumptions.append('Uses final fire rate; trigger acceleration/ramp is not yet simulated.')
  # Quake spread takes ray offsets at 8192 units, not radians.
  w['spread']=math.tan(t['error angle'][0])*8192;w['pellets']=t['projectiles per shot']
  assumptions.append('Initial angular error converted to Quake ray offsets; random distribution and sustained-fire spread are still Quake.')
  w['projectile_range']=pv['maximum range']*SCALE;w['projectile_speed']=pv['initial velocity']*30*SCALE;w['splash_damage']=0;w['splash_radius']=0
  effect=projectile.get('damage_effects',{}).get('impact damage')
  if slot=='WP_GRENADE_LAUNCHER':
   effect=projectile['damage_effects']['attached detonation damage']
   superdamage=next(d for d in projectile['effects']['super detonation']['damage_effects'] if d['values']['damage upper bound'][1]>0)
   w.update(tracking_radians=pv['guided angular velocity'],projectile_range=pv['maximum range']*SCALE,attachment_ms=ms(pv['timer'][0]),combine_count=7,combine_damage=round(superdamage['values']['damage upper bound'][0]),combine_radius=superdamage['values']['radius'][1]*SCALE)
   p['provenance']['source_tag_hashes'].append({'path':superdamage['path'],'class':superdamage['class'],'root_sha256':superdamage['root_sha256']})
   assumptions.append('Seven-needle threshold follows halo-ce-universal build 2342 reference, commit 76b1898ee14e6fb58e0412acc183da509c10e001; uploaded tags are build 2276. Acquisition cone, world-contact behavior and grouped explosion scheduling are adaptations awaiting retail validation.')
  if slot=='WP_ROCKET_LAUNCHER':effect=next(d for d in projectile['effects']['effect']['damage_effects'] if d['values']['damage upper bound'][1]>0)
  dv=effect['values'];w['damage']=round(dv['damage upper bound'][0]);w['shield_multiplier']=dv['cyborg energy shield'];w['health_multiplier']=dv['cyborg armor']
  if slot=='WP_ROCKET_LAUNCHER':w['splash_damage']=w['damage'];w['splash_radius']=dv['radius'][1]*SCALE
  if len(set(dv['damage upper bound']+[dv['damage lower bound']]))>1:assumptions.append('Uses the first upper-bound damage value; randomized damage and distance falloff remain pending.')
  p['provenance']['source_tag_hashes'].append({'path':effect['path'],'class':effect['class'],'root_sha256':effect['root_sha256']})
  w['melee_damage']=round(source['melee']['values']['damage upper bound'][0])
  converted={k:w[k] for k in ['name','magazine','ammo_initial','ammo_max','reload_rounds','reload_ms','fire_ms','damage','shield_multiplier','health_multiplier','headshot_mode','spread','pellets','projectile_speed','projectile_range','tracking_radians','attachment_ms','combine_count','combine_damage','combine_radius','heat_per_shot','heat_loss_per_second','heat_recovery','heat_overheat','heat_age_penalty','charge_ms','charged_ammo','charged_heat','charged_damage','charged_speed','charged_range','charged_tracking_radians','charged_health_multiplier','charged_shield_multiplier']}
  audit['weapons'][slot]={'tag':source['path'],'root_sha256':source['root_sha256'],'imported':converted,'source_magazine':mag,'source_trigger':t,'source_damage':{k:dv[k] for k in ['damage lower bound','damage upper bound','cyborg armor','cyborg energy shield']},'assumptions':assumptions}
 collision=report['player']['collision'];c=collision['values'];p['player']={'health':round(c['maximum body vitality']),'shield':round(c['maximum shield vitality']),'shield_delay_ms':ms(c['stun time']),'shield_recharge_ms':ms(c['recharge time'])}
 p['provenance']['source_tag_hashes'].append({'path':collision['path'],'class':collision['class'],'root_sha256':collision['root_sha256']})
 audit['player']={'tag':report['player']['path'],'collision_tag':collision['path'],'imported':p['player'],'movement_source':report['globals']['values']['player information'][0],'biped_source':{k:report['player']['values'][k] for k in ['jump velocity','standing camera height','crouching camera height','standing collision height','crouching collision height','collision radius']},'note':'Movement values preserved for the next behavior/conversion pass, not applied as unverified accelerations.'}
 # Grenade splash values can be imported independently of bounce/throw behavior.
 for i,g in enumerate(report['grenades']):
  effects=g['projectile']['effects']['effect']['damage_effects'];d=next(x for x in effects if x['values']['damage upper bound'][1]>0);v=d['values']
  p['grenades'][i].update(fuse_ms=ms(g['projectile']['values']['timer'][0]),timer_start=g['projectile']['values']['detonation timer starts'],max_flight_ms=10000,max_count=g['globals']['maximum count'],splash_damage=round(v['damage upper bound'][0]),splash_radius=v['radius'][1]*SCALE,shield_multiplier=v['cyborg energy shield'],health_multiplier=v['cyborg armor'])
  p['provenance']['source_tag_hashes'].append({'path':d['path'],'class':d['class'],'root_sha256':d['root_sha256']})
 return p,audit
def main():
 a=argparse.ArgumentParser();a.add_argument('report',type=Path);a.add_argument('--ui-index',type=Path);args=a.parse_args()
 p,audit=import_profile(json.loads(args.report.read_text()),json.loads((ROOT/'data/gameplay-profile.json').read_text()))
 if args.ui_index:
  ui=json.loads(args.ui_index.read_text())['map']
  if ui['build']!=audit['source_map']['build'] or ui['version']!=5 or ui['name']!='ui':raise ValueError('UI cache does not match the reference build/type')
  audit['ui_map']=ui
 (ROOT/'data/gameplay-profile.json').write_text(json.dumps(p,indent=2)+'\n');(ROOT/'data/halo-import-report.json').write_text(json.dumps(audit,indent=2)+'\n')
 print('Imported supported Xbox scalars; status remains mixed, with explicit parity gaps.')
if __name__=='__main__':main()
