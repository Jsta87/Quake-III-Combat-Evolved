#!/usr/bin/env python3
"""Validate the gameplay profile and emit shared C89/QVM-compatible definitions."""
import argparse,hashlib,json,math,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ORDER=['WP_NONE','WP_GAUNTLET','WP_MACHINEGUN','WP_SHOTGUN','WP_GRENADE_LAUNCHER','WP_ROCKET_LAUNCHER','WP_LIGHTNING','WP_RAILGUN','WP_PLASMAGUN','WP_BFG','WP_GRAPPLING_HOOK','WP_NAILGUN','WP_PROX_LAUNCHER','WP_CHAINGUN']
KINDS=['none','melee','bullet','shotgun','grenade','rocket','lightning','rail','plasma']
FIELDS=['magazine','reload_ms','fire_ms','damage','spread','recoil_degrees','fire_kind','projectile_speed','splash_damage','splash_radius','fuse_ms','shield_multiplier','health_multiplier','headshot_mode','melee_damage','melee_reach','lunge_reach','lunge_speed','melee_impact_ms','melee_ms','pellets','ammo_initial','ammo_max','reload_rounds','tracking_radians','projectile_range','attachment_ms','combine_count','combine_damage','combine_radius','heat_per_shot','heat_loss_per_second','heat_recovery','heat_overheat','heat_age_penalty','charge_ms','charged_ammo','charged_heat','charged_damage','charged_speed','charged_range','charged_tracking_radians','charged_health_multiplier','charged_shield_multiplier','spread_max','spread_grow','spread_recover','projectile_final_speed','falloff_start','falloff_end']
FIELDS+=['reload_commit_ms','reload_empty_commit_ms']
FIELDS+=['reload_enter_ms','reload_exit_ms','reload_exit_empty_ms']
FIELDS+=['damage_minimum', 'damage_maximum', 'splash_inner', 'projectile_gravity', 'water_gravity', 'water_falloff_start', 'water_falloff_end', 'rate_min', 'rate_max', 'rate_grow', 'rate_recover', 'ready_ms', 'reload_empty_ms', 'overheat_ms', 'battery_cost', 'charged_battery_cost', 'zoom_levels', 'zoom_min', 'zoom_max', 'scoped_error']
RFIELDS=['response', 'potential', 'flags', 'skip', 'angle_min', 'angle_max', 'velocity_min', 'velocity_max', 'initial', 'parallel', 'perpendicular', 'angular_noise', 'velocity_noise']
MFIELDS=['forward', 'backward', 'sideways', 'crouch_forward', 'crouch_backward', 'crouch_sideways', 'acceleration', 'crouch_acceleration', 'air_acceleration', 'jump', 'gravity', 'radius', 'standing_height', 'crouch_height', 'standing_view', 'crouch_view', 'crouch_ms', 'slope_falloff', 'slope_cutoff', 'uphill_scale', 'downhill_scale', 'grenade_up']
GFIELDS=['spawn_count','max_count','throw_speed','fuse_ms','sticky','splash_damage','splash_radius','shield_multiplier','health_multiplier','timer_start','max_flight_ms']
GFIELDS+=['damage_minimum', 'damage_maximum', 'splash_inner', 'gravity', 'bounce_parallel', 'bounce_perpendicular', 'metal_parallel', 'metal_perpendicular']
INTS={'magazine','reload_ms','fire_ms','damage','splash_damage','fuse_ms','headshot_mode','melee_damage','melee_impact_ms','melee_ms','pellets','spawn_count','max_count','ammo_initial','ammo_max','reload_rounds','timer_start','max_flight_ms','attachment_ms','combine_count','combine_damage','heat_per_shot','heat_loss_per_second','heat_recovery','heat_overheat','heat_age_penalty','charge_ms','charged_ammo','charged_heat','charged_damage','spread_grow','spread_recover'}
INTS.update(['response','potential','flags'])
INTS.update(['reload_commit_ms','reload_empty_commit_ms'])
INTS.update(['reload_enter_ms','reload_exit_ms','reload_exit_empty_ms'])
INTS.update(['rate_grow', 'rate_recover', 'ready_ms', 'reload_empty_ms', 'overheat_ms', 'battery_cost', 'charged_battery_cost', 'zoom_levels', 'scoped_error'])
def validate(p):
 if p['schema_version']!=3 or p['status'] not in ('provisional','mixed','reference'):raise ValueError('Unsupported profile schema/status')
 if set(p['weapons'])!=set(ORDER):raise ValueError('Weapon membership must match the engine slots')
 if p['units']['distance']!='quake_units' or p['units']['time']!='milliseconds':raise ValueError('Convert values to engine units before generating')
 if p['status'] in ('mixed','reference') and (not p['provenance']['reference_build'] or not p['provenance']['source_maps'] or not p['provenance']['source_tag_hashes']):raise ValueError('Reference profiles require build, maps and tag hashes')
 if set(p['materials'])!=set(ORDER):raise ValueError('Invalid material membership')
 for responses in p['materials'].values():
  if len(responses)!=33:raise ValueError('Exactly 33 material responses required')
  for response in responses:
   if set(response)!=set(RFIELDS) or any(type(v) not in (int,float) or not math.isfinite(v) or v<0 or v>1000000 for v in response.values()) or any(type(response[k])!=int for k in ('response','potential','flags')) or response['response']>4 or response['potential']>4 or response['flags']>3 or any(response[k]>1 for k in ('skip','initial','parallel','perpendicular')):raise ValueError('Invalid material response')
 movement=p['movement']
 if set(movement)!=set(MFIELDS) or any(type(v) not in (int,float) or not math.isfinite(v) or v<=0 or v>10000 for v in movement.values()):raise ValueError('Invalid movement profile')
 player=p['player']
 if set(player)!={'health','shield','shield_delay_ms','shield_recharge_ms'} or any(type(v)!=int or v<=0 for v in player.values()) or max(player['health'],player['shield'])>1000 or max(player['shield_delay_ms'],player['shield_recharge_ms'])>600000:raise ValueError('Invalid player vitality/recharge profile')
 for key,w in p['weapons'].items():
  if set(w)!=set(FIELDS+['name']):raise ValueError(f'{key}: unknown/missing fields')
  if w['fire_kind'] not in KINDS:raise ValueError(f'{key}: invalid firing behavior')
  if not isinstance(w['name'],str) or not re.fullmatch(r'[A-Za-z0-9 _-]+',w['name']):raise ValueError(f'{key}: ASCII weapon name required')
  numeric={k:w[k] for k in FIELDS if k!='fire_kind'}
  for k,v in numeric.items():
   if type(v) not in (int,float) or not math.isfinite(v) or v<0 or v>1000000:raise ValueError(f'{key}.{k}: invalid nonnegative value')
   if k in INTS and type(v)!=int:raise ValueError(f'{key}.{k}: integer required')
  if w['reload_commit_ms']>w['reload_ms'] or w['reload_empty_commit_ms']>w['reload_empty_ms']:raise ValueError(f'{key}: ammo commit must fall inside reload')
  if any(w[k] for k in ('reload_enter_ms','reload_exit_ms','reload_exit_empty_ms')) and w['reload_rounds']!=1:raise ValueError(f'{key}: staged reload requires one-round insertion')
  if w['ready_ms']<1 or w['reload_empty_ms']<1 or w['rate_min']<0 or (w['rate_max']==0 and not w['charge_ms']) or w['battery_cost']>1000000 or w['charged_battery_cost']>1000000 or w['zoom_levels'] and (w['zoom_min']<1 or w['zoom_max']<w['zoom_min']):raise ValueError(f'{key}: invalid timing/battery/zoom definition')
  if w['damage_maximum']<w['damage'] or w['damage_minimum']>w['damage'] or w['rate_max']<w['rate_min'] or w['zoom_levels']>2 or w['scoped_error']>1 or w['splash_inner']>w['splash_radius']:raise ValueError(f'{key}: invalid damage/rate/zoom bounds')
  if w['spread_max']<w['spread'] or w['falloff_end']<w['falloff_start']:raise ValueError(f'{key}: reversed spread/falloff bounds')
  if w['projectile_final_speed']>w['projectile_speed'] or w['projectile_final_speed']<w['projectile_speed'] and w['falloff_end']<=w['falloff_start']:raise ValueError(f'{key}: unsupported projectile deceleration bounds')
  if w['charge_ms'] and (key!='WP_LIGHTNING' or not w['heat_per_shot'] or w['charge_ms']>600000 or not 0<w['charged_ammo']<=w['magazine'] or not 0<w['charged_heat']<=10000 or w['charged_damage']<=0 or w['charged_speed']<=0 or w['charged_range']<=0 or not 0<w['charged_shield_multiplier']<=16 or not 0<=w['charged_health_multiplier']<=16 or w['charged_tracking_radians']>6.283186):raise ValueError(f'{key}: invalid charged-shot definition')
  if w['heat_per_shot'] and (key not in ('WP_PLASMAGUN','WP_LIGHTNING') or not 0<w['heat_recovery']<w['heat_overheat']<=10000 or not 0<w['heat_loss_per_second']<=10000 or not 0<=w['heat_age_penalty']<1000 or w['heat_per_shot']>10000):raise ValueError(f'{key}: invalid heat definition')
  if key!='WP_GRENADE_LAUNCHER' and any(w[k] for k in ['tracking_radians','attachment_ms','combine_count','combine_damage','combine_radius']):raise ValueError(f'{key}: needle behavior is only supported in the Needler slot')
  if w['attachment_ms'] and (w['fire_kind']!='plasma' or w['projectile_speed']<=0 or w['projectile_range']<=0):raise ValueError(f'{key}: invalid needle travel definition')
  if w['tracking_radians']>6.283186 or w['combine_count']>64 or w['attachment_ms']>600000:raise ValueError(f'{key}: invalid projectile behavior')
  if w['combine_count'] and (w['attachment_ms']<1 or w['combine_damage']<1 or w['combine_radius']<=0):raise ValueError(f'{key}: incomplete supercombine definition')
  if w['headshot_mode'] and w['fire_kind'] not in ('bullet','rail'):raise ValueError(f'{key}: headshot policy requires precision bullet/rail firing')
  if w['magazine']>10000 or not w['magazine']<=w['ammo_max']<=10000 or not 0<=w['ammo_initial']<=w['ammo_max'] or not 0<=w['reload_rounds']<=w['magazine'] or w['headshot_mode'] not in (0,1,2) or w['fire_ms']<1 or w['reload_ms']<1 or not 1<=w['pellets']<=64:raise ValueError(f'{key}: invalid capacity/timing/policy')
  if not 0<w['shield_multiplier']<=16 or not 0<=w['health_multiplier']<=16 or w['lunge_reach']<w['melee_reach'] or not 0<w['melee_impact_ms']<w['melee_ms']:raise ValueError(f'{key}: invalid damage/melee scaling')
 if len(p['grenades'])!=2:raise ValueError('Exactly frag/plasma grenade definitions are required')
 for i,g in enumerate(p['grenades']):
  if set(g)!=set(GFIELDS+['name']) or g['name']!=['frag','plasma'][i] or type(g['sticky'])!=bool:raise ValueError('Invalid grenade definition')
  for k in GFIELDS:
   if k=='sticky':continue
   v=g[k]
   if type(v) not in (int,float) or not math.isfinite(v) or v<0 or v>1000000 or (k in INTS and type(v)!=int):raise ValueError(f'Invalid grenade {k}')
  if g['damage_minimum']>g['splash_damage'] or g['damage_maximum']<g['splash_damage'] or g['splash_inner']>g['splash_radius'] or any(g[k]>1 for k in ('bounce_parallel','bounce_perpendicular','metal_parallel','metal_perpendicular')):raise ValueError('Invalid grenade damage/material bounds')
  if g['timer_start'] not in (0,1,2) or not 1<=g['max_flight_ms']<=600000:raise ValueError('Invalid grenade arming/lifetime')
  if not 0<=g['spawn_count']<=g['max_count']<=4 or g['fuse_ms']<1 or not 0<g['shield_multiplier']<=16 or not 0<=g['health_multiplier']<=16:raise ValueError('Invalid grenade capacity/timing/scaling')
 return p
def literal(k,v):
 if k=='fire_kind':return str(KINDS.index(v))
 if type(v) is bool:return str(int(v))
 if k in INTS:return str(v)
 return format(float(v),'.8f')+'f'
def render(p):
 digest=hashlib.sha256(json.dumps(p,sort_keys=True,separators=(',',':')).encode()).hexdigest()
 lines=['/* Generated by scripts/generate-profile.py; edit data/gameplay-profile.json. */',f'#define QCE_PROFILE_SHA "{digest}"','static const qce_weapondef_t qce_weapondefs[WP_NUM_WEAPONS] = {']
 for i,key in enumerate(ORDER):
  if i==11:lines.append('#ifdef MISSIONPACK')
  w=p['weapons'][key];lines.append(' { '+', '.join(literal(k,w[k]) for k in FIELDS)+' }, /* '+key+' */')
 lines+=['#endif','};','static const qce_grenadedef_t qce_grenadedefs[2] = {']
 for g in p['grenades']:lines.append(' { '+', '.join(literal(k,g[k]) for k in GFIELDS)+' },')
 lines+=['};','static const char *qce_weapon_names[WP_NUM_WEAPONS] = {']
 for i,key in enumerate(ORDER):
  if i==11:lines.append('#ifdef MISSIONPACK')
  label=p['weapons'][key]['name'].removeprefix('halo_').removesuffix('_placeholder').replace('_',' ').title()
  if key=='WP_BFG':label='Magnum'
  if key=='WP_GAUNTLET':label='Melee'
  lines.append(' '+json.dumps(label)+',')
 lines+=['#endif','};','static const qce_materialdef_t qce_materialdefs[WP_NUM_WEAPONS][33] = {']
 for i,key in enumerate(ORDER):
  if i==11:lines.append('#ifdef MISSIONPACK')
  lines.append(' {')
  for response in p['materials'][key]:lines.append('  { '+', '.join(literal(k,response[k]) for k in RFIELDS)+' },')
  lines.append(' },')
 lines+=['#endif','};','static const qce_movementdef_t qce_movementdef = { '+', '.join(literal(k,p['movement'][k]) for k in MFIELDS)+' };','static const qce_playerdef_t qce_playerdef = { '+', '.join(str(p['player'][k]) for k in ['health','shield','shield_delay_ms','shield_recharge_ms'])+' };','']
 return '\n'.join(lines)
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');parser.add_argument('--profile',type=Path,default=ROOT/'data/gameplay-profile.json');args=parser.parse_args()
 contents=render(validate(json.loads(args.profile.read_text())))
 dest=ROOT/'engine/code/game/bg_qce_profile.generated.h'
 if args.check:
  if not dest.exists() or dest.read_text()!=contents:raise SystemExit('Generated profile is stale; run scripts/generate-profile.py')
 elif not dest.exists() or dest.read_text()!=contents:dest.write_text(contents)
if __name__=='__main__':main()
