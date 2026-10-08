#!/usr/bin/env python3
"""Apply supported scalar fields; retain explicit provenance and parity gaps."""
import argparse,copy,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SCALE=80.0 # .7 Halo standing collision height -> existing 56-unit Quake hull.
KINDS={'WP_MACHINEGUN':'bullet','WP_SHOTGUN':'shotgun','WP_ROCKET_LAUNCHER':'rocket','WP_RAILGUN':'rail','WP_PLASMAGUN':'plasma','WP_LIGHTNING':'plasma','WP_BFG':'bullet','WP_GRENADE_LAUNCHER':'plasma'}
def ms(seconds):return round(seconds*1000)
def import_profile(report,profile):
 p=copy.deepcopy(profile);p['schema_version']=3;p['status']='mixed'
 p['provenance']={'reference_build':report['map']['build'],'region':'NTSC-U build identifier','source_maps':[report['map']],'source_tag_hashes':[], 'notes':'Supported scalar values extracted from uploaded Xbox cache. Conversions and incomplete behaviors are listed in data/halo-import-report.json; retail parity is not validated.'}
 p['units']['halo_to_quake_scale']=SCALE
 p['materials']={slot:[dict(response=0,potential=0,flags=0,skip=0.0,angle_min=0.0,angle_max=0.0,velocity_min=0.0,velocity_max=0.0,initial=0.0,parallel=0.0,perpendicular=0.0,angular_noise=0.0,velocity_noise=0.0) for _ in range(33)] for slot in p['weapons']}
 audit={'source_map':report['map'],'scale':SCALE,'scale_basis':'0.7 world-unit standing hull mapped to 56 existing Quake units; chosen adaptation scale, not an official Quake conversion','tick_rate':30,'weapons':{},'player':{},'remaining':['retail-2276 and latency parity validation; source reference is build2342','animation channel/index resolution, reload cancellation/event/chamber details and rendered animations','animated collision/model head nodes; current head ellipsoid and hull-center explosion distance are adaptations','30Hz retail scheduling versus Quake command/server ticks, integer vitality/heat rounding and random spread distributions','damage impulses, fall/landing/stun behavior and arena water/media/material classification','grenade release keyframe, spawn counts and 10-second unarmed safety expiry','charged plasma media/material responses and target acquisition cone validation','unlimited-range projectile 60-second safety expiry and within-substep bounce remainder','vehicle simulation and Quake placeholder art/audio remain']}
 for w in p['weapons'].values():
  w.update(ammo_initial=0,ammo_max=200,reload_rounds=w['magazine'],tracking_radians=0.0,projectile_range=0.0,attachment_ms=0,combine_count=0,combine_damage=0,combine_radius=0.0,heat_per_shot=0,heat_loss_per_second=0,heat_recovery=0,heat_overheat=0,heat_age_penalty=0,charge_ms=0,charged_ammo=0,charged_heat=0,charged_damage=0,charged_speed=0.0,charged_range=0.0,charged_tracking_radians=0.0,charged_health_multiplier=0.0,charged_shield_multiplier=0.0,spread_max=w['spread'],spread_grow=0,spread_recover=0,projectile_final_speed=w['projectile_speed'],falloff_start=0.0,falloff_end=0.0)
 for slot,source in report['weapons'].items():
  w=p['weapons'][slot];v=source['values'];t=v['triggers'][0];projectile=source['projectiles'][0];pv=projectile['values'];mag=v['magazines'][0]
  p['materials'][slot]=[dict(response=r['default response'],potential=r['potential response'],flags=r['potential flags'],skip=r['potential skip fraction'],angle_min=r['potential between'][0],angle_max=r['potential between'][1],velocity_min=r['potential and'][0]*SCALE,velocity_max=r['potential and'][1]*SCALE,initial=r['initial friction'],parallel=r['parallel friction'],perpendicular=r['perpendicular friction'],angular_noise=r['angular noise'],velocity_noise=r['velocity noise']*SCALE) for r in pv['projectile material response']]
  name=source['path'].split('\\')[-1];w['name']='halo_'+name.replace(' ','_');w['fire_kind']=KINDS[slot];w['headshot_mode']=2 if slot=='WP_RAILGUN' else 1 if slot=='WP_BFG' else 0
  converted={};assumptions=[]
  for tag in [source,projectile,source['melee'],source['animations']]:
   if tag:p['provenance']['source_tag_hashes'].append({'path':tag['path'],'class':tag['class'],'root_sha256':tag['root_sha256']})
  if mag['rounds loaded maximum']:
   w['magazine']=mag['rounds loaded maximum'];w['ammo_initial']=mag['rounds total initial'];w['ammo_max']=mag['rounds loaded maximum']+mag['rounds reserved maximum'];w['reload_rounds']=mag['rounds reloaded']
   if mag['reload time']>0:w['reload_ms']=ms(mag['reload time'])
   else:
    animation=next(a for a in source['animations']['values']['animations'] if a['name']=='first-person reload-empty')
    w['reload_ms']=round(animation['frame count']*1000/30);assumptions.append('Full/empty reload intervals derive from extracted animation frame counts at 30 frames/s; event/chamber/cancellation scheduling needs retail-2276 validation.')
  else:
   # HUD ammunition is a derived count; authoritative battery keeps micro-age units.
   age=t['age generated per round'];w['magazine']=round(1/age) if age>0 else 100;w['ammo_initial']=w['ammo_max']=w['magazine'];w['reload_rounds']=0
   assumptions.append('Battery consumption uses millionth-age units, including charged cost; HUD ammo is derived from remaining energy and normal-shot cost. Cooling age retains permille precision.')
  if slot in ('WP_PLASMAGUN','WP_LIGHTNING'):
   w.update(heat_per_shot=round(t['heat generated per round']*10000),heat_loss_per_second=round(v['heat loss rate']*10000),heat_recovery=round(v['heat recovery threshold']*10000),heat_overheat=round(v['overheated threshold']*10000),heat_age_penalty=round(v['age heat recovery penalty']*1000))
  if slot=='WP_LIGHTNING':
   ct=v['triggers'][1];cp=source['projectiles'][1];cd=cp['damage_effects']['impact damage'];cv=cp['values'];dvcharge=cd['values']
   if dvcharge['damage side effect']!=3:raise ValueError('Charged plasma damage must have the supported EMP side effect')
   w.update(charge_ms=ms(t['charging time']),charged_ammo=round(ct['age generated per round']/t['age generated per round']),charged_heat=round(ct['heat generated per round']*10000),charged_damage=round(dvcharge['damage upper bound'][0]),charged_speed=cv['initial velocity']*30*SCALE,charged_range=cv['maximum range']*SCALE,charged_tracking_radians=cv['guided angular velocity'],charged_health_multiplier=dvcharge['cyborg armor'],charged_shield_multiplier=dvcharge['cyborg energy shield'])
   for tag in [cp,cd]:p['provenance']['source_tag_hashes'].append({'path':tag['path'],'class':tag['class'],'root_sha256':tag['root_sha256']})
   assumptions.append('Release before 600 ms fires primary bolt; full charge fires secondary bolt with EMP shield depletion. Behavior follows build-2342 reference. Acquisition cone, cancellation, media behavior and build differences await retail validation.')
  animations={a['name']:a for a in source['animations']['values']['animations']}
  melee=animations['first-person melee'];ready=animations['first-person ready']
  w.update(melee_ms=round(melee['frame count']*1000/30),melee_impact_ms=max(1,round(melee['key frame index']*1000/30)),ready_ms=ms(v['ready time']) if v['ready time'] else round(ready['frame count']*1000/30))
  full=animations.get('first-person reload-full');empty=animations.get('first-person reload-empty')
  # Animation duration controls the visible magazine reload; shell reload retains its scalar cycle.
  if mag['rounds reloaded']>1 and full:w['reload_ms']=round(full['frame count']*1000/30)
  w['reload_empty_ms']=round(empty['frame count']*1000/30) if empty else w['reload_ms']
  w.update(reload_enter_ms=0,reload_exit_ms=0,reload_exit_empty_ms=0)
  if w['reload_rounds']==1:
   for field,clip in [('reload_enter_ms','enter'),('reload_exit_ms','exit-full'),('reload_exit_empty_ms','exit-empty')]:
    track=animations.get('first-person '+clip)
    if track:w[field]=round(track['frame count']*1000/(25 if track['flags']&4 else 30))
  hot=animations.get('first-person overheating');w['overheat_ms']=round(hot['frame count']*1000/30) if hot else 0
  w.update(rate_min=t['maximum rate of fire'][0],rate_max=t['maximum rate of fire'][1],rate_grow=round(t['firing acceleration rate']*30*10000),rate_recover=round(t['firing deceleration rate']*30*10000),battery_cost=round(t['age generated per round']*1000000),charged_battery_cost=round(v['triggers'][1]['age generated per round']*1000000) if slot=='WP_LIGHTNING' else 0,zoom_levels=v['zoom levels'],zoom_min=v['zoom magnification range'][0],zoom_max=v['zoom magnification range'][1],scoped_error=int(bool(t['flags']&1024)))
  rate=t['maximum rate of fire'][1]
  w['fire_ms']=round(math.ceil(30/rate-1e-6)*1000/30) if rate>0 else round(1000/30)
  assumptions.append('Trigger rate ramps use cached per-tick rates converted to fixed-point per-second rates.')
  # Quake spread takes ray offsets at 8192 units, not radians.
  w['spread']=math.tan(t['error angle'][0])*8192;w['spread_max']=math.tan(t['error angle'][1])*8192;w['spread_grow']=round(t['error acceleration rate']*30*10000);w['spread_recover']=round(t['error deceleration rate']*30*10000);w['pellets']=t['projectiles per shot']
  assumptions.append('Angular error endpoints and growth/recovery imported; 7-bit shot spread and Quake random distributions remain adaptations.')
  w['projectile_range']=pv['maximum range']*SCALE;w['projectile_speed']=pv['initial velocity']*30*SCALE;w['projectile_final_speed']=pv['final velocity']*30*SCALE;w['falloff_start']=pv['air damage range'][0]*SCALE;w['falloff_end']=pv['air damage range'][1]*SCALE;w['splash_damage']=0;w['splash_radius']=0
  effect=projectile.get('damage_effects',{}).get('impact damage')
  if slot=='WP_GRENADE_LAUNCHER':
   effect=projectile['damage_effects']['attached detonation damage']
   superdamage=next(d for d in projectile['effects']['super detonation']['damage_effects'] if d['values']['damage upper bound'][1]>0)
   w.update(tracking_radians=pv['guided angular velocity'],projectile_range=pv['maximum range']*SCALE,attachment_ms=ms(pv['timer'][0]),combine_count=7,combine_damage=round(superdamage['values']['damage upper bound'][0]),combine_radius=superdamage['values']['radius'][1]*SCALE)
   p['provenance']['source_tag_hashes'].append({'path':superdamage['path'],'class':superdamage['class'],'root_sha256':superdamage['root_sha256']})
   assumptions.append('Seven-needle threshold follows halo-ce-universal build 2342 reference, commit 76b1898ee14e6fb58e0412acc183da509c10e001; uploaded tags are build 2276. Acquisition cone and grouped explosion scheduling await retail validation; grazing ricochet uses extracted material bounds.')
  if slot=='WP_ROCKET_LAUNCHER':effect=next(d for d in projectile['effects']['effect']['damage_effects'] if d['values']['damage upper bound'][1]>0)
  dv=effect['values'];w['damage']=round(dv['damage upper bound'][0]);w['shield_multiplier']=dv['cyborg energy shield'];w['health_multiplier']=dv['cyborg armor']
  w.update(damage_minimum=dv['damage lower bound'],damage_maximum=dv['damage upper bound'][1],splash_inner=dv['radius'][0]*SCALE if slot=='WP_ROCKET_LAUNCHER' else 0,projectile_gravity=pv['air gravity scale'],water_gravity=pv['water gravity scale'],water_falloff_start=pv['water damage range'][0]*SCALE,water_falloff_end=pv['water damage range'][1]*SCALE)
  if slot=='WP_ROCKET_LAUNCHER':w['splash_damage']=w['damage'];w['splash_radius']=dv['radius'][1]*SCALE
  assumptions.append('Damage minimum and random full-strength bounds imported separately; final vitality remains integer-valued in Quake.')
  p['provenance']['source_tag_hashes'].append({'path':effect['path'],'class':effect['class'],'root_sha256':effect['root_sha256']})
  w['melee_damage']=round(source['melee']['values']['damage upper bound'][0])
  converted={k:w[k] for k in ['name','magazine','ammo_initial','ammo_max','reload_rounds','reload_ms','fire_ms','damage','shield_multiplier','health_multiplier','headshot_mode','spread','pellets','projectile_speed','projectile_range','tracking_radians','attachment_ms','combine_count','combine_damage','combine_radius','heat_per_shot','heat_loss_per_second','heat_recovery','heat_overheat','heat_age_penalty','charge_ms','charged_ammo','charged_heat','charged_damage','charged_speed','charged_range','charged_tracking_radians','charged_health_multiplier','charged_shield_multiplier','spread_max','spread_grow','spread_recover','projectile_final_speed','falloff_start','falloff_end']}
  converted=copy.deepcopy(w)
  audit['weapons'][slot]={'tag':source['path'],'root_sha256':source['root_sha256'],'imported':converted,'source_magazine':mag,'source_trigger':t,'source_damage':{k:dv[k] for k in ['damage lower bound','damage upper bound','cyborg armor','cyborg energy shield']},'assumptions':assumptions}
 collision=report['player']['collision'];c=collision['values'];p['player']={'health':round(c['maximum body vitality']),'shield':round(c['maximum shield vitality']),'shield_delay_ms':ms(c['stun time']),'shield_recharge_ms':ms(c['recharge time'])}
 p['provenance']['source_tag_hashes'].append({'path':collision['path'],'class':collision['class'],'root_sha256':collision['root_sha256']})
 audit['player']={'tag':report['player']['path'],'collision_tag':collision['path'],'imported':p['player'],'movement_source':report['globals']['values']['player information'][0],'biped_source':{k:report['player']['values'][k] for k in ['jump velocity','standing camera height','crouching camera height','standing collision height','crouching collision height','collision radius']},'note':'Directional speeds, absolute acceleration, jump, gravity, hull radius, camera transition and slope modifiers are applied. Acceleration conversion follows reference bipeds.c; arena BSP collision remains an adaptation.'}
 b=report['player']['values'];m=report['globals']['values']['player information'][0]
 p['movement']={k:m[source]*SCALE for k,source in [('forward','run forward'),('backward','run backward'),('sideways','run sideways'),('crouch_forward','sneak forward'),('crouch_backward','sneak backward'),('crouch_sideways','sneak sideways')]}
 p['movement'].update(acceleration=m['run acceleration']*30*SCALE,crouch_acceleration=m['sneak acceleration']*30*SCALE,air_acceleration=m['airborne acceleration']*30*SCALE,jump=b['jump velocity']*30*SCALE,gravity=9.78/3.048*SCALE,radius=b['collision radius']*SCALE,standing_height=b['standing collision height']*SCALE,crouch_height=b['crouching collision height']*SCALE,standing_view=b['standing camera height']*SCALE,crouch_view=b['crouching camera height']*SCALE,crouch_ms=ms(b['crouch transition time']),slope_falloff=b['uphill falloff angle'],slope_cutoff=b['maximum slope angle'],uphill_scale=b['uphill velocity scale'],downhill_scale=b['downhill velocity scale'],grenade_up=m['grenade origin'][1]*SCALE)
 audit['player']['movement_imported']=p['movement']
 # Grenade splash values can be imported independently of bounce/throw behavior.
 for i,g in enumerate(report['grenades']):
  effects=g['projectile']['effects']['effect']['damage_effects'];d=next(x for x in effects if x['values']['damage upper bound'][1]>0);v=d['values']
  responses=g['projectile']['values']['projectile material response'];default=responses[2];metal=responses[7]
  p['grenades'][i].update(throw_speed=report['player']['values']['grenade velocity']*SCALE,damage_minimum=v['damage lower bound'],damage_maximum=v['damage upper bound'][1],splash_inner=v['radius'][0]*SCALE,gravity=g['projectile']['values']['air gravity scale'],bounce_parallel=default['parallel friction'],bounce_perpendicular=default['perpendicular friction'],metal_parallel=metal['parallel friction'],metal_perpendicular=metal['perpendicular friction'])
  p['grenades'][i].update(fuse_ms=ms(g['projectile']['values']['timer'][0]),timer_start=g['projectile']['values']['detonation timer starts'],max_flight_ms=10000,max_count=g['globals']['maximum count'],splash_damage=round(v['damage upper bound'][0]),splash_radius=v['radius'][1]*SCALE,shield_multiplier=v['cyborg energy shield'],health_multiplier=v['cyborg armor'])
  p['provenance']['source_tag_hashes'].append({'path':d['path'],'class':d['class'],'root_sha256':d['root_sha256']})
 for tag in [report['globals'],report['player']]+[g['projectile'] for g in report['grenades']]:
  p['provenance']['source_tag_hashes'].append({'path':tag['path'],'class':tag['class'],'root_sha256':tag['root_sha256']})
 audit['grenades']=copy.deepcopy(p['grenades'])
 audit['material_mapping']={'world':'stone, index 2','SURF_METALSTEPS':'metal thick, index 7','unshielded player':'cyborg armor, index 21','shielded player':'cyborg energy shield, index 22','note':'Weapon responses preserve all 33 extracted entries; Quake BSP classification selects this subset.'}
 return p,audit
def profile_json(profile):
 # Keep each material row on one line so numeric-table changes stay reviewable.
 materials=profile['materials'];body=copy.deepcopy(profile);body.pop('materials')
 lines=json.dumps(body,indent=2).splitlines();lines[-2]+=',';lines[-1]='  "materials": {'
 slots=list(materials)
 for i,slot in enumerate(slots):
  lines.append('    '+json.dumps(slot)+': [')
  for j,row in enumerate(materials[slot]):lines.append('      '+json.dumps(row,separators=(', ', ': '))+(',' if j<len(materials[slot])-1 else ''))
  lines.append('    ]'+(',' if i<len(slots)-1 else ''))
 lines+=['  }','}'];return '\n'.join(lines)+'\n'
def main():
 a=argparse.ArgumentParser();a.add_argument('report',type=Path);a.add_argument('--ui-index',type=Path);args=a.parse_args()
 p,audit=import_profile(json.loads(args.report.read_text()),json.loads((ROOT/'data/gameplay-profile.json').read_text()))
 if args.ui_index:
  ui=json.loads(args.ui_index.read_text())['map']
  if ui['build']!=audit['source_map']['build'] or ui['version']!=5 or ui['name']!='ui':raise ValueError('UI cache does not match the reference build/type')
  audit['ui_map']=ui
 (ROOT/'data/gameplay-profile.json').write_text(profile_json(p));(ROOT/'data/halo-import-report.json').write_text(json.dumps(audit,indent=2)+'\n')
 print('Imported supported Xbox scalars; status remains mixed, with explicit parity gaps.')
if __name__=='__main__':main()
