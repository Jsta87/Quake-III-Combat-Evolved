/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "g_local.h"
static gentity_t *QceNeedleOwner(gentity_t *ent);

#define	MISSILE_PRESTEP_TIME	50

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, trace->endpos );
			ent->s.time = level.time / 4;
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
static qboolean G_QceMissileSplash(gentity_t *ent,gentity_t *ignore) {
 if(ent->qceGrenadeType) {
  const qce_grenadedef_t *g=BG_QceGrenadeDef(ent->qceGrenadeType-1);
  return G_QceRadiusDamage(ent->r.currentOrigin,ent->parent,ent->splashDamage,g->damage_minimum,g->damage_maximum,g->splash_inner,ent->splashRadius,ignore,ent->splashMethodOfDeath);
 }
 if(ent->qceProjectileWeapon==WP_ROCKET_LAUNCHER) {
  const qce_weapondef_t *w=BG_QceWeaponDef(ent->qceProjectileWeapon);
  return G_QceRadiusDamage(ent->r.currentOrigin,ent->parent,ent->splashDamage,w->damage_minimum,w->damage_maximum,w->splash_inner,ent->splashRadius,ignore,ent->splashMethodOfDeath);
 }
 return G_RadiusDamage(ent->r.currentOrigin,ent->parent,ent->splashDamage,ent->splashRadius,ignore,ent->splashMethodOfDeath);
}
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

 if(ent->qceProjectileWeapon || ent->qceGrenadeType)ent->parent=QceNeedleOwner(ent);
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	// splash damage
	if ( ent->splashDamage ) {
		if( G_QceMissileSplash(ent,ent) ) {
			if(ent->r.ownerNum>=0 && ent->r.ownerNum<level.maxclients && g_entities[ent->r.ownerNum].client &&
               (!(ent->qceNeedle || ent->qceProjectileWeapon || ent->qceGrenadeType) || g_entities[ent->r.ownerNum].qceEntitySerial==ent->qceOwnerSerial))
                g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}
	}

	trap_LinkEntity( ent );
}


#ifdef MISSIONPACK
/*
================
ProximityMine_Explode
================
*/
static void ProximityMine_Explode( gentity_t *mine ) {
	G_ExplodeMissile( mine );
	// if the prox mine has a trigger free it
	if (mine->activator) {
		G_FreeEntity(mine->activator);
		mine->activator = NULL;
	}
}

/*
================
ProximityMine_Die
================
*/
static void ProximityMine_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
ProximityMine_Trigger
================
*/
void ProximityMine_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace ) {
	vec3_t		v;
	gentity_t	*mine;

	if( !other->client ) {
		return;
	}

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract( trigger->s.pos.trBase, other->s.pos.trBase, v );
	if( VectorLength( v ) > trigger->parent->splashRadius ) {
		return;
	}


	if ( g_gametype.integer >= GT_TEAM ) {
		// don't trigger same team mines
		if (trigger->parent->s.generic1 == other->client->sess.sessionTeam) {
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered through walls, closed doors, etc...
	if( !CanDamage( other, trigger->s.pos.trBase ) ) {
		return;
	}

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent( mine, EV_PROXIMITY_MINE_TRIGGER, 0 );
	mine->nextthink = level.time + 500;

	G_FreeEntity( trigger );
}

/*
================
ProximityMine_Activate
================
*/
static void ProximityMine_Activate( gentity_t *ent ) {
	gentity_t	*trigger;
	float		r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer;

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

	ent->s.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav" );

	// build the proximity trigger
	trigger = G_Spawn ();

	trigger->classname = "proxmine_trigger";

	r = ent->splashRadius;
	VectorSet( trigger->r.mins, -r, -r, -r );
	VectorSet( trigger->r.maxs, r, r, r );

	G_SetOrigin( trigger, ent->s.pos.trBase );

	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity (trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

/*
================
ProximityMine_ExplodeOnPlayer
================
*/
static void ProximityMine_ExplodeOnPlayer( gentity_t *mine ) {
	gentity_t	*player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	if ( player->client->invulnerabilityTime > level.time ) {
		G_Damage( player, mine->parent, mine->parent, vec3_origin, mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK, MOD_JUICED );
		player->client->invulnerabilityTime = 0;
		G_TempEntity( player->client->ps.origin, EV_JUICED );
	}
	else {
		G_SetOrigin( mine, player->s.pos.trBase );
		// make sure the explosion gets to the client
		mine->r.svFlags &= ~SVF_NOCLIENT;
		mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
		G_ExplodeMissile( mine );
	}
}

/*
================
ProximityMine_Player
================
*/
static void ProximityMine_Player( gentity_t *mine, gentity_t *player ) {
	if( mine->s.eFlags & EF_NODRAW ) {
		return;
	}

	G_AddEvent( mine, EV_PROXIMITY_MINE_STICK, 0 );

	if( player->s.eFlags & EF_TICKING ) {
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear( mine->s.pos.trDelta );

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	if ( player->client->invulnerabilityTime > level.time ) {
		mine->nextthink = level.time + 2 * 1000;
	}
	else {
		mine->nextthink = level.time + 10 * 1000;
	}
}
#endif

/*
================
G_MissileImpact
================
*/
#include "g_qce_needle.h"

static int G_QceMaterialResponse(gentity_t *ent,trace_t *trace,gentity_t *other) {
 int material=other->client?(other->client->ps.stats[STAT_QCE_SHIELD]>0?22:21):((trace->surfaceFlags&SURF_METALSTEPS)?7:2);
 const qce_materialdef_t *m=BG_QceMaterialDef(ent->qceProjectileWeapon,material);
 vec3_t direction;
 float speed=VectorNormalize2(ent->s.pos.trDelta,direction),dot=-DotProduct(direction,trace->plane.normal);
 float angle,velocity;
 if(dot<0)dot=0;
 if(dot>1)dot=1;
 angle=atan2(dot,sqrt(1-dot*dot))+crandom()*m->angular_noise;
 velocity=speed*dot-random()*m->velocity_noise;
 if(m->potential && (!m->angle_max || (angle>=m->angle_min && angle<=m->angle_max)) &&
   (!m->velocity_max || (velocity>=m->velocity_min && velocity<=m->velocity_max)) &&
   (!(m->flags&1) || other->client) && (!(m->flags&2) || !other->client) && random()>=m->skip)return m->potential;
 return m->response;
}
static qboolean G_QceContinueProjectile(gentity_t *ent,trace_t *trace,gentity_t *other,int response) {
 int material=other->client?(other->client->ps.stats[STAT_QCE_SHIELD]>0?22:21):((trace->surfaceFlags&SURF_METALSTEPS)?7:2),i;
 const qce_materialdef_t *m=BG_QceMaterialDef(ent->qceProjectileWeapon,material);
 float dot;
 if(response==3 && other->client) {
  ent->target_ent=other;VectorScale(ent->s.pos.trDelta,1-m->initial,ent->s.pos.trDelta);
  VectorCopy(trace->endpos,ent->r.currentOrigin);return qtrue;
 }
 if(response==2) {
  dot=DotProduct(ent->s.pos.trDelta,trace->plane.normal);
  for(i=0;i<3;i++)ent->s.pos.trDelta[i]=(ent->s.pos.trDelta[i]-dot*trace->plane.normal[i])*(1-m->perpendicular)-dot*trace->plane.normal[i]*(1-m->parallel);
  VectorMA(trace->endpos,0.08f,trace->plane.normal,ent->r.currentOrigin);
  return qtrue;
 }
 return qfalse;
}

static void G_QceBounceGrenade(gentity_t *ent,trace_t *trace) {
 const qce_grenadedef_t *g=BG_QceGrenadeDef(ent->qceGrenadeType-1);
 float dot=DotProduct(ent->s.pos.trDelta,trace->plane.normal);
 float parallel=(trace->surfaceFlags&SURF_METALSTEPS)?g->metal_parallel:g->bounce_parallel;
 float perpendicular=(trace->surfaceFlags&SURF_METALSTEPS)?g->metal_perpendicular:g->bounce_perpendicular;
 int i;
 for(i=0;i<3;i++)ent->s.pos.trDelta[i]=(ent->s.pos.trDelta[i]-dot*trace->plane.normal[i])*(1-perpendicular)-dot*trace->plane.normal[i]*(1-parallel);
 if(trace->plane.normal[2]>0.7f && VectorLength(ent->s.pos.trDelta)<8)G_SetOrigin(ent,trace->endpos);
 else {
  VectorMA(trace->endpos,0.08f,trace->plane.normal,ent->r.currentOrigin);
  VectorCopy(ent->r.currentOrigin,ent->s.pos.trBase);ent->s.pos.trTime=level.time;ent->s.pos.trType=TR_LINEAR;
 }
}
qboolean G_QceGrenadeImpact(gentity_t *ent,trace_t *trace) {
 gentity_t *other=&g_entities[trace->entityNum];
 /* Hand frags bounce off bodies too; plasma arms its fuse on first contact. */
 if(ent->qceGrenadeType>0 && (!BG_QceGrenadeDef(ent->qceGrenadeType-1)->sticky || !other->client)) {
  const qce_grenadedef_t *def=BG_QceGrenadeDef(ent->qceGrenadeType-1);
  G_QceBounceGrenade(ent,trace);
  if(!ent->qceFuseArmed && (def->timer_start==1 || (def->timer_start==2 && ent->s.pos.trType==TR_STATIONARY))) {
   ent->qceFuseArmed=1;ent->nextthink=level.time+def->fuse_ms;
  }
  G_AddEvent(ent,EV_GRENADE_BOUNCE,0);return qtrue;
 }
 if(ent->qceGrenadeType>0 && BG_QceGrenadeDef(ent->qceGrenadeType-1)->sticky && !ent->qceStuck) {
  vec3_t offset,angles,forward,right,up;
  ent->qceStuck=1;ent->qceAttachEntity=trace->entityNum;
  ent->qceAttachSerial=other->qceEntitySerial;
  ent->qceAttachSpawn=other->client?other->client->ps.persistant[PERS_SPAWN_COUNT]:0;
  G_SetOrigin(ent,trace->endpos);
  if(!ent->qceFuseArmed) {ent->qceFuseArmed=1;ent->nextthink=level.time+BG_QceGrenadeDef(ent->qceGrenadeType-1)->fuse_ms;}
  VectorSubtract(trace->endpos,other->r.currentOrigin,offset);
  VectorCopy(other->r.currentAngles,angles);
  if(other->client)VectorSet(angles,0,other->client->ps.viewangles[YAW],0);
  AngleVectors(angles,forward,right,up);
  ent->qceAttachOffset[0]=DotProduct(offset,forward);
  ent->qceAttachOffset[1]=DotProduct(offset,right);
  ent->qceAttachOffset[2]=DotProduct(offset,up);
  trap_LinkEntity(ent);return qtrue;
 }
 return qfalse;
}

qboolean G_QceRunStuckGrenade(gentity_t *ent) {
 vec3_t origin;
 if(ent->qceStuck) {
  if(ent->qceAttachEntity>=0 && ent->qceAttachEntity<ENTITYNUM_WORLD) {
   gentity_t *target=&g_entities[ent->qceAttachEntity];
   if(target->inuse && target->qceEntitySerial==ent->qceAttachSerial &&
      (!target->client || target->client->ps.persistant[PERS_SPAWN_COUNT]==ent->qceAttachSpawn)) {
    vec3_t angles,forward,right,up;
    VectorCopy(target->r.currentAngles,angles);
    if(target->client)VectorSet(angles,0,target->client->ps.viewangles[YAW],0);
    AngleVectors(angles,forward,right,up);VectorCopy(target->r.currentOrigin,origin);
    VectorMA(origin,ent->qceAttachOffset[0],forward,origin);
    VectorMA(origin,ent->qceAttachOffset[1],right,origin);
    VectorMA(origin,ent->qceAttachOffset[2],up,origin);
    G_SetOrigin(ent,origin);
   } else ent->qceAttachEntity=ENTITYNUM_WORLD;
  }
  trap_LinkEntity(ent);G_RunThink(ent);return qtrue;
 }
 return qfalse;
}

void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
#ifdef MISSIONPACK
	vec3_t			forward, impactpoint, bouncedir;
	int				eFlags;
#endif
	other = &g_entities[trace->entityNum];
 if(ent->qceProjectileWeapon || ent->qceGrenadeType)ent->parent=QceNeedleOwner(ent);

 if(ent->qceNeedle && G_QceContinueProjectile(ent,trace,other,G_QceMaterialResponse(ent,trace,other)))return;
 if(G_QceNeedleImpact(ent,trace))return;
 if(G_QceGrenadeImpact(ent,trace))return;

 if(ent->qceBullet) {
  gentity_t *owner=QceNeedleOwner(ent);
  gentity_t *event;
  vec3_t damagePoint;
  int amount=(int)ceil(BG_QceDamage(ent->qceProjectileWeapon,ent->qceDamageScale,random())*ent->count);
  VectorCopy(trace->endpos,damagePoint);
  if(BG_QceWeaponDef(ent->qceProjectileWeapon)->headshot_mode)G_QceResolveHeadPoint(other,trace->endpos,ent->s.pos.trDelta,damagePoint);
  if(other->takedamage && owner) {
   if(owner->client && LogAccuracyHit(other,owner))owner->client->accuracy_hits++;
   G_Damage(other,ent,owner,ent->s.pos.trDelta,damagePoint,amount,0,ent->methodOfDeath);
  }
  if(ent->qceProjectileWeapon==WP_RAILGUN) {
   event=G_TempEntity(trace->endpos,EV_RAILTRAIL);VectorCopy(ent->s.origin2,event->s.origin2);event->s.clientNum=ent->r.ownerNum;
  } else {
   event=G_TempEntity(trace->endpos,other->client?EV_BULLET_HIT_FLESH:EV_BULLET_HIT_WALL);
   event->s.eventParm=other->client?other->s.number:DirToByte(trace->plane.normal);event->s.otherEntityNum=ent->r.ownerNum;
  }
  if(G_QceContinueProjectile(ent,trace,other,G_QceMaterialResponse(ent,trace,other))) {
   VectorCopy(trace->endpos,ent->s.origin2);return;
  }
  G_FreeEntity(ent);return;
 }
	// check for bounce
	if ( !other->takedamage &&
		( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		return;
	}

#ifdef MISSIONPACK
	if ( other->takedamage ) {
		if ( ent->s.weapon != WP_PROX_LAUNCHER ) {
			if ( other->client && other->client->invulnerabilityTime > level.time ) {
				//
				VectorCopy( ent->s.pos.trDelta, forward );
				VectorNormalize( forward );
				if (G_InvulnerabilityEffect( other, forward, ent->s.pos.trBase, impactpoint, bouncedir )) {
					VectorCopy( bouncedir, trace->plane.normal );
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile( ent, trace );
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
		}
	}
#endif
	// impact damage
	if (other->takedamage) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;

			if( ent->parent && ent->parent->client && LogAccuracyHit( other, ent->parent ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}
   if(ent->qceProjectileWeapon && !ent->qceNeedle && !ent->qceCharged)
    ent->damage=(int)ceil(BG_QceDamage(ent->qceProjectileWeapon,ent->qceDamageScale,random()));
			G_Damage (other, ent, ent->parent, velocity,
				trace->endpos, ent->damage,
				0, ent->methodOfDeath);
		}
	}

#ifdef MISSIONPACK
	if( ent->s.weapon == WP_PROX_LAUNCHER ) {
		if( ent->s.pos.trType != TR_GRAVITY ) {
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if( other->s.eType == ET_PLAYER && other->health > 0 ) {
			ProximityMine_Player( ent, other );
			return;
		}

		SnapVectorTowards( trace->endpos, ent->s.pos.trBase );
		G_SetOrigin( ent, trace->endpos );
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		G_AddEvent( ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags );

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles( trace->plane.normal, ent->s.angles );
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		VectorSet(ent->r.mins, -4, -4, -4);
		VectorSet(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}
#endif

	if (!strcmp(ent->classname, "hook")) {
		gentity_t *nent;
		vec3_t v;

		nent = G_Spawn();
		if ( other->takedamage && other->client ) {

			G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
		} else {
			VectorCopy(trace->endpos, v);
			G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
			ent->enemy = NULL;
		}

		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin( ent, v );
		G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity( ent );
		trap_LinkEntity( nent );

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if( G_QceMissileSplash(ent,other) ) {
			if( !hitClient && ent->parent && ent->parent->client ) {
				ent->parent->client->accuracy_hits++;
			}
		}
	}

	trap_LinkEntity( ent );
}

/*
================
G_RunMissile
================
*/
#include "g_qce_projectile.h"

void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			passent;

 if(G_QceRunStuckGrenade(ent))return;
 G_QceTrackNeedle(ent);
 if(ent->qceProjectileWeapon || ent->qceGrenadeType) {G_QceRunProjectile(ent);return;}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
#ifdef MISSIONPACK
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if (ent->s.weapon == WP_PROX_LAUNCHER && ent->count) {
		passent = ENTITYNUM_NONE;
	}
#endif
	else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	trap_LinkEntity( ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}
			G_FreeEntity( ent );
			return;
		}
		G_MissileImpact( ent, &tr );
		if ( ent->s.eType != ET_MISSILE ) {
			return;		// exploded
		}
	}
#ifdef MISSIONPACK
	// if the prox mine wasn't yet outside the player body
	if (ent->s.weapon == WP_PROX_LAUNCHER && !ent->count) {
		// check if the prox mine is outside the owner bbox
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
		if (!tr.startsolid || tr.entityNum != ent->r.ownerNum) {
			ent->count = 1;
		}
	}
#endif
	// check think function after bouncing
	G_RunThink( ent );
}


//=============================================================================

/*
=================
fire_plasma

=================
*/
static int QceProjectileMod(int weapon,qboolean splash) {
 switch(weapon) {
 case WP_MACHINEGUN:return MOD_MACHINEGUN;
 case WP_SHOTGUN:return MOD_SHOTGUN;
 case WP_GRENADE_LAUNCHER:return splash?MOD_GRENADE_SPLASH:MOD_GRENADE;
 case WP_ROCKET_LAUNCHER:return splash?MOD_ROCKET_SPLASH:MOD_ROCKET;
 case WP_LIGHTNING:return MOD_LIGHTNING;
 case WP_RAILGUN:return MOD_RAILGUN;
 case WP_PLASMAGUN:return splash?MOD_PLASMA_SPLASH:MOD_PLASMA;
 case WP_BFG:return splash?MOD_BFG_SPLASH:MOD_BFG;
 default:return MOD_UNKNOWN;
 }
}

gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashDamage = 15;
	bolt->splashRadius = 20;
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

 if(self->client && self->client->ps.stats[STAT_QCE_COMBAT]) {
  const qce_weapondef_t *def=BG_QceWeaponDef(self->client->ps.weapon);
  G_QceInitProjectile(bolt,self,self->client->ps.weapon);
  bolt->methodOfDeath=QceProjectileMod(self->client->ps.weapon,qfalse);
  bolt->splashMethodOfDeath=QceProjectileMod(self->client->ps.weapon,qtrue);
  bolt->damage=def->damage;bolt->splashDamage=def->splash_damage;bolt->splashRadius=def->splash_radius;
  bolt->nextthink=level.time+60000;
  bolt->nextthink=level.time+60000;
  VectorScale(dir,def->projectile_speed,bolt->s.pos.trDelta);SnapVector(bolt->s.pos.trDelta);
  if(def->attachment_ms>0) {
   bolt->qceNeedle=1;bolt->qceOwnerSerial=self->qceEntitySerial;
   bolt->think=G_QceNeedleThink;bolt->s.weapon=WP_PLASMAGUN;
   bolt->nextthink=level.time+60000;
   QceAcquireNeedle(bolt,dir,def->projectile_range);
  }
 }

	return bolt;
}	

//=============================================================================


/*
=================
fire_grenade
=================
*/
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

 if(self->client && self->client->ps.stats[STAT_QCE_COMBAT]) {
  const qce_weapondef_t *def=BG_QceWeaponDef(self->client->ps.weapon);
  G_QceInitProjectile(bolt,self,self->client->ps.weapon);
  bolt->methodOfDeath=QceProjectileMod(self->client->ps.weapon,qfalse);
  bolt->splashMethodOfDeath=QceProjectileMod(self->client->ps.weapon,qtrue);
  bolt->damage=def->damage;bolt->splashDamage=def->splash_damage;bolt->splashRadius=def->splash_radius;
  bolt->nextthink=level.time+60000;
  VectorScale(dir,def->projectile_speed,bolt->s.pos.trDelta);SnapVector(bolt->s.pos.trDelta);
 }

	return bolt;
}

//=============================================================================


/*
=================
fire_bfg
=================
*/
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_rocket
=================
*/
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 900, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

 if(self->client && self->client->ps.stats[STAT_QCE_COMBAT]) {
  const qce_weapondef_t *def=BG_QceWeaponDef(self->client->ps.weapon);
  G_QceInitProjectile(bolt,self,self->client->ps.weapon);
  bolt->methodOfDeath=QceProjectileMod(self->client->ps.weapon,qfalse);
  bolt->splashMethodOfDeath=QceProjectileMod(self->client->ps.weapon,qtrue);
  bolt->damage=def->damage;bolt->splashDamage=def->splash_damage;bolt->splashRadius=def->splash_radius;
  bolt->nextthink=level.time+60000;
  VectorScale(dir,def->projectile_speed,bolt->s.pos.trDelta);SnapVector(bolt->s.pos.trDelta);
 }

	return bolt;
}

/*
=================
fire_grapple
=================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*hook;

	VectorNormalize (dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number; // use to match beam in client
	VectorCopy( start, hook->s.pos.trBase );
	VectorScale( dir, 800, hook->s.pos.trDelta );
	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	self->client->hook = hook;

	return hook;
}


#ifdef MISSIONPACK
/*
=================
fire_nail
=================
*/
#define NAILGUN_SPREAD	500

gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up ) {
	gentity_t	*bolt;
	vec3_t		dir;
	vec3_t		end;
	float		r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NAILGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	VectorCopy( start, bolt->s.pos.trBase );

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * NAILGUN_SPREAD * 16;
	r = cos(r) * crandom() * NAILGUN_SPREAD * 16;
	VectorMA( start, 8192 * 16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);
	VectorSubtract( end, start, dir );
	VectorNormalize( dir );

	scale = 555 + random() * 1800;
	VectorScale( dir, scale, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );

	VectorCopy( start, bolt->r.currentOrigin );

	return bolt;
}	


/*
=================
fire_prox
=================
*/
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t dir ) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_PROXIMITY_MINE;
	bolt->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	bolt->count = 0;

	//FIXME: we prolly wanna abuse another field
	bolt->s.generic1 = self->client->sess.sessionTeam;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}
#endif

gentity_t *fire_qce_overcharge(gentity_t *self,vec3_t start,vec3_t dir) {
 const qce_weapondef_t *def=BG_QceWeaponDef(WP_LIGHTNING);
 gentity_t *bolt=fire_plasma(self,start,dir);
 bolt->qceCharged=1;bolt->qceOwnerSerial=self->qceEntitySerial;bolt->damage=def->charged_damage;
 bolt->methodOfDeath=MOD_QCE_OVERCHARGE;bolt->splashDamage=0;
 VectorScale(dir,def->charged_speed,bolt->s.pos.trDelta);
 bolt->nextthink=level.time+60000;
 QceAcquireNeedle(bolt,dir,def->charged_range);
 return bolt;
}
