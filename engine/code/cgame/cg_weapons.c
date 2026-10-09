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
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "cg_qce_visual.h"
#define QCE_DEFINE_PARTICLE_DATA
#include "../game/bg_qce_presentation.generated.h"
#include "cg_halo_present.h"

/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void CG_MachineGunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	float			waterScale = 1.0f;
	vec3_t			v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void CG_ShotgunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	vec3_t			v[3];
	int				i;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	for ( i = 0; i < 2; i++ ) {
		float	waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if ( i == 0 ) {
			velocity[1] = 40 + 10 * crandom();
		} else {
			velocity[1] = -40 + 10 * crandom();
		}
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + cg_brassTime.integer*3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis( cent->lerpAngles, v );

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		VectorAdd( cent->lerpOrigin, xoffset, re->origin );
		VectorCopy( re->origin, le->pos.trBase );
		if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
			waterScale = 0.10f;
		}

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		VectorScale( xvelocity, waterScale, le->pos.trDelta );

		AxisCopy( axisDefault, re->axis );
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->leFlags = LEF_TUMBLE;
		le->leBounceSoundType = LEBS_BRASS;
		le->leMarkType = LEMT_NONE;
	}
}


#ifdef MISSIONPACK
/*
==========================
CG_NailgunEjectBrass
==========================
*/
static void CG_NailgunEjectBrass( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	vec3_t			v[3];
	vec3_t			offset;
	vec3_t			xoffset;
	vec3_t			up;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 0;
	offset[1] = -12;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, origin );

	VectorSet( up, 0, 0, 64 );

	smoke = CG_SmokePuff( origin, up, 32, 1, 1, 1, 0.33f, 700, cg.time, 0, 0, cgs.media.smokePuffShader );
	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}
#endif


/*
==========================
CG_RailTrail
==========================
*/
void CG_RailTrail (clientInfo_t *ci, vec3_t start, vec3_t end) {
	vec3_t axis[36], move, move2, vec, temp;
	float  len;
	int    i, j, skip;
 
	localEntity_t *le;
	refEntity_t   *re;
 
#define RADIUS   4
#define ROTATION 1
#define SPACING  5
 
 if(cg.predictedPlayerState.stats[STAT_QCE_COMBAT]) {
  le=CG_AllocLocalEntity();re=&le->refEntity;
  le->leType=LE_FADE_RGB;le->startTime=cg.time;le->endTime=cg.time+1500;le->lifeRate=1.0f/1500;
  re->reType=RT_RAIL_CORE;re->customShader=cg_qceWorld.sniperSmokeShader;
  VectorCopy(start,re->origin);VectorCopy(end,re->oldorigin);
  le->color[0]=le->color[1]=le->color[2]=0.7f;le->color[3]=0.65f;
  re->shaderRGBA[0]=re->shaderRGBA[1]=re->shaderRGBA[2]=180;re->shaderRGBA[3]=166;
  return;
 }
	start[2] -= 4;
 
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
 
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
 
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;
 
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
 
	re->shaderRGBA[0] = ci->color1[0] * 255;
	re->shaderRGBA[1] = ci->color1[1] * 255;
	re->shaderRGBA[2] = ci->color1[2] * 255;
	re->shaderRGBA[3] = 255;

	le->color[0] = ci->color1[0] * 0.75;
	le->color[1] = ci->color1[1] * 0.75;
	le->color[2] = ci->color1[2] * 0.75;
	le->color[3] = 1.0f;

	AxisClear( re->axis );
 
	if (cg_oldRail.integer)
	{
		// nudge down a bit so it isn't exactly in center
		re->origin[2] -= 8;
		re->oldorigin[2] -= 8;
		return;
	}

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	PerpendicularVector(temp, vec);
	for (i = 0 ; i < 36; i++)
	{
		RotatePointAroundVector(axis[i], vec, temp, i * 10);//banshee 2.4 was 10
	}

	VectorMA(move, 20, vec, move);
	VectorScale (vec, SPACING, vec);

	skip = -1;
 
	j = 18;
	for (i = 0; i < len; i += SPACING)
	{
		if (i != skip)
		{
			skip = i + SPACING;
			le = CG_AllocLocalEntity();
			re = &le->refEntity;
			le->leFlags = LEF_PUFF_DONT_SCALE;
			le->leType = LE_MOVE_SCALE_FADE;
			le->startTime = cg.time;
			le->endTime = cg.time + (i>>1) + 600;
			le->lifeRate = 1.0 / (le->endTime - le->startTime);

			re->shaderTime = cg.time / 1000.0f;
			re->reType = RT_SPRITE;
			re->radius = 1.1f;
			re->customShader = cgs.media.railRingsShader;

			re->shaderRGBA[0] = ci->color2[0] * 255;
			re->shaderRGBA[1] = ci->color2[1] * 255;
			re->shaderRGBA[2] = ci->color2[2] * 255;
			re->shaderRGBA[3] = 255;

			le->color[0] = ci->color2[0] * 0.75;
			le->color[1] = ci->color2[1] * 0.75;
			le->color[2] = ci->color2[2] * 0.75;
			le->color[3] = 1.0f;

			le->pos.trType = TR_LINEAR;
			le->pos.trTime = cg.time;

			VectorCopy( move, move2);
			VectorMA(move2, RADIUS , axis[j], move2);
			VectorCopy(move2, le->pos.trBase);

			le->pos.trDelta[0] = axis[j][0]*6;
			le->pos.trDelta[1] = axis[j][1]*6;
			le->pos.trDelta[2] = axis[j][2]*6;
		}

		VectorAdd (move, vec, move);

		j = (j + ROTATION) % 36;
	}
}

/*
==========================
CG_RocketTrail
==========================
*/
static void CG_RocketTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 
					  wi->trailRadius, 
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime, 
					  t,
					  0,
					  0, 
					  cgs.media.smokePuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}

#ifdef MISSIONPACK
/*
==========================
CG_NailTrail
==========================
*/
static void CG_NailTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 
					  wi->trailRadius, 
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime, 
					  t,
					  0,
					  0, 
					  cgs.media.nailPuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}
#endif

/*
==========================
CG_PlasmaTrail
==========================
*/
static void CG_PlasmaTrail( centity_t *cent, const weaponInfo_t *wi ) {
	localEntity_t	*le;
	refEntity_t		*re;
	entityState_t	*es;
	vec3_t			velocity, xvelocity, origin;
	vec3_t			offset, xoffset;
	vec3_t			v[3];

	float	waterScale = 1.0f;

	if ( cg_noProjectileTrail.integer || cg_oldPlasma.integer ) {
		return;
	}

	es = &cent->currentState;

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 60 - 120 * crandom();
	velocity[1] = 40 - 80 * crandom();
	velocity[2] = 100 - 200 * crandom();

	le->leType = LE_MOVE_SCALE_FADE;
	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_NONE;
	le->leMarkType = LEMT_NONE;

	le->startTime = cg.time;
	le->endTime = le->startTime + 600;

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 2;
	offset[1] = 2;
	offset[2] = 2;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

	VectorAdd( origin, xoffset, re->origin );
	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_SPRITE;
	re->radius = 0.25f;
	re->customShader = cgs.media.railRingsShader;
	le->bounceFactor = 0.3f;

	re->shaderRGBA[0] = wi->flashDlightColor[0] * 63;
	re->shaderRGBA[1] = wi->flashDlightColor[1] * 63;
	re->shaderRGBA[2] = wi->flashDlightColor[2] * 63;
	re->shaderRGBA[3] = 63;

	le->color[0] = wi->flashDlightColor[0] * 0.2;
	le->color[1] = wi->flashDlightColor[1] * 0.2;
	le->color[2] = wi->flashDlightColor[2] * 0.2;
	le->color[3] = 0.25f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 1;
	le->angles.trDelta[1] = 0.5;
	le->angles.trDelta[2] = 0;

}
/*
==========================
CG_GrappleTrail
==========================
*/
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi ) {
	vec3_t	origin;
	entityState_t	*es;
	vec3_t			forward, up;
	refEntity_t		beam;

	es = &ent->currentState;

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	ent->trailTime = cg.time;

	memset( &beam, 0, sizeof( beam ) );
	//FIXME adjust for muzzle position
	VectorCopy ( cg_entities[ ent->currentState.otherEntityNum ].lerpOrigin, beam.origin );
	beam.origin[2] += 26;
	AngleVectors( cg_entities[ ent->currentState.otherEntityNum ].lerpAngles, forward, NULL, up );
	VectorMA( beam.origin, -6, up, beam.origin );
	VectorCopy( origin, beam.oldorigin );

	if (Distance( beam.origin, beam.oldorigin ) < 64 )
		return; // Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;

	AxisClear( beam.axis );
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene( &beam );
}

/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	CG_RocketTrail( ent, wi );
}


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
static void CG_RegisterHaloView( weaponInfo_t *weapon, const char *name ) {
 char path[MAX_QPATH], text[16384], *cursor, *token;
 fileHandle_t file;
 int length, i, j, eventVersion, eventCount;
 qceViewClip_t clips[QCE_VIEW_CLIPS];
 memset(clips,0,sizeof(clips));
 for(i=0;i<QCE_VIEW_CLIPS;i++)clips[i].fps=30;
 Com_sprintf(path,sizeof(path),"models/qce/halo/view/%s.cfg",name);
 length = trap_FS_FOpenFile(path,&file,FS_READ);
 if (length < 0) return;
 if (length <= 0 || length >= sizeof(text)) { if(file)trap_FS_FCloseFile(file); return; }
 trap_FS_Read(text,length,file); trap_FS_FCloseFile(file); text[length] = 0; cursor = text;
 for(i=0;i<QCE_VIEW_CLIPS;i++) {
  int fields[4];
  for(j=0;j<4;j++) { token=COM_Parse(&cursor); if(!token[0])break; fields[j]=atoi(token); }
  if(j==0 && i>=QCE_VIEW_RELOAD_EXIT+1)break;
  if(j!=4)return;
  clips[i].first=fields[0]; clips[i].count=fields[1]; clips[i].fps=fields[2]; clips[i].loop=fields[3];
  if(fields[0]<0 || fields[1]<0 || fields[1]>1024 || fields[0]>1024-fields[1] || fields[2]<1 || fields[2]>100 || fields[3]<0 || (fields[1]>0 && fields[3]>=fields[1]))return;
 }
 if(clips[QCE_VIEW_IDLE].count<=0)return;
 Com_sprintf(path,sizeof(path),"models/qce/halo/view/%s.iqm",name);
 if(trap_FS_FOpenFile(path,NULL,FS_READ)<=0)return;
 weapon->haloViewModel=trap_R_RegisterModel(path);
 if(!weapon->haloViewModel)return;
 memcpy(weapon->haloClips,clips,sizeof(clips));Q_strncpyz(weapon->haloName,name,sizeof(weapon->haloName));
 Com_sprintf(path,sizeof(path),"models/qce/halo/view/%s_60.skin",name);
 weapon->haloAmmoCounter=trap_FS_FOpenFile(path,NULL,FS_READ)>0;
 Com_sprintf(path,sizeof(path),"models/qce/halo/view/%s.bob",name);
 length=trap_FS_FOpenFile(path,&file,FS_READ);
 if(length>0 && length<sizeof(text)) {
  trap_FS_Read(text,length,file);trap_FS_FCloseFile(file);text[length]=0;cursor=text;
  if(atoi(COM_Parse(&cursor))==1)weapon->haloMovingJoints=atoi(COM_Parse(&cursor));
  if(weapon->haloMovingJoints<1 || weapon->haloMovingJoints>64)weapon->haloMovingJoints=0;
 } else if(file)trap_FS_FCloseFile(file);
 weapon->haloStopSound=trap_S_RegisterSound("sound/qce/halo/events/stop.wav",qfalse);
 Com_sprintf(path,sizeof(path),"models/qce/halo/view/%s.events",name);
 length=trap_FS_FOpenFile(path,&file,FS_READ);
 if(length<=0 || length>=sizeof(text)) {if(file)trap_FS_FCloseFile(file);return;}
 trap_FS_Read(text,length,file);trap_FS_FCloseFile(file);text[length]=0;cursor=text;
 eventVersion=atoi(COM_Parse(&cursor));eventCount=atoi(COM_Parse(&cursor));
 if((eventVersion!=1 && eventVersion!=2) || eventCount<1 || eventCount>QCE_VIEW_CLIPS)return;
 for(i=0;i<eventCount;i++) {
  qceViewSound_t *event=&weapon->haloSounds[i];
  event->frame=atoi(COM_Parse(&cursor));event->count=atoi(COM_Parse(&cursor));
  event->loop=eventVersion==2?atoi(COM_Parse(&cursor)):0;
  if(event->loop<0 || event->loop>1 || event->count<0 || event->count>4 || event->frame<0 || (event->count && event->frame>=clips[i].count)) {memset(weapon->haloSounds,0,sizeof(weapon->haloSounds));return;}
  for(j=0;j<event->count;j++) {
   token=COM_Parse(&cursor);
   if(!token[0] || strlen(token)>=MAX_QPATH) {memset(weapon->haloSounds,0,sizeof(weapon->haloSounds));return;}
   event->sounds[j]=trap_S_RegisterSound(token,qfalse);
  }
 }
}

void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model[0] );

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	}

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_flash.md3" );
	weaponInfo->flashModel = trap_R_RegisterModel( path );

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_barrel.md3" );
	weaponInfo->barrelModel = trap_R_RegisterModel( path );

	COM_StripExtension( item->world_model[0], path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_hand.md3" );
	weaponInfo->handsModel = trap_R_RegisterModel( path );

	if ( !weaponInfo->handsModel ) {
		weaponInfo->handsModel = trap_R_RegisterModel( "models/weapons2/shotgun/shotgun_hand.md3" );
	}

	switch ( weaponNum ) {
	case WP_GAUNTLET:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/melee/fstrun.wav", qfalse );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav", qfalse );
		break;

	case WP_LIGHTNING:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/melee/fsthum.wav", qfalse );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/lightning/lg_hum.wav", qfalse );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/lightning/lg_fire.wav", qfalse );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel( "models/weaphits/crackle.md3" );
		cgs.media.sfx_lghit1 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit.wav", qfalse );
		cgs.media.sfx_lghit2 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit2.wav", qfalse );
		cgs.media.sfx_lghit3 = trap_S_RegisterSound( "sound/weapons/lightning/lg_hit3.wav", qfalse );

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/rocket/rocket.md3" );
		weaponInfo->missileTrailFunc = CG_GrappleTrail;
		weaponInfo->missileDlight = 200;
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/melee/fsthum.wav", qfalse );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/melee/fstrun.wav", qfalse );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		break;

#ifdef MISSIONPACK
	case WP_CHAINGUN:
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/vulcan/wvulfire.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound( "sound/weapons/vulcan/vulcanf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;
#endif

	case WP_MACHINEGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;

	case WP_SHOTGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/shotgun/sshotf1b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/rocket/rocket.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		cgs.media.rocketExplosionShader = trap_R_RegisterShader( "rocketExplosion" );
		break;

#ifdef MISSIONPACK
	case WP_PROX_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/proxmine.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/proxmine/wstbfire.wav", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;
#endif

	case WP_GRENADE_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/grenade1.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/grenade/grenlf1a.wav", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;

#ifdef MISSIONPACK
	case WP_NAILGUN:
		weaponInfo->ejectBrassFunc = CG_NailgunEjectBrass;
		weaponInfo->missileTrailFunc = CG_NailTrail;
//		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/nailgun/wnalflit.wav", qfalse );
		weaponInfo->trailRadius = 16;
		weaponInfo->wiTrailTime = 250;
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/nail.md3" );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/nailgun/wnalfire.wav", qfalse );
		break;
#endif

	case WP_PLASMAGUN:
//		weaponInfo->missileModel = cgs.media.invulnerabilityPowerupModel;
		weaponInfo->missileTrailFunc = CG_PlasmaTrail;
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/plasma/lasfly.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/plasma/hyprbf1a.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		break;

	case WP_RAILGUN:
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/railgun/rg_hum.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.5f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/railgun/railgf1a.wav", qfalse );
		cgs.media.railExplosionShader = trap_R_RegisterShader( "railExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		cgs.media.railCoreShader = trap_R_RegisterShader( "railCore" );
		break;

	case WP_BFG:
		weaponInfo->readySound = trap_S_RegisterSound( "sound/weapons/bfg/bfg_hum.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.7f, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/bfg/bfg_fire.wav", qfalse );
		cgs.media.bfgExplosionShader = trap_R_RegisterShader( "bfgExplosion" );
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weaphits/bfg.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		break;

	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		break;
	}
	/* Locally converted Halo audio is optional. Keep arena sounds if absent. */
	{
		const char *haloName = NULL;
		char soundPath[MAX_QPATH];
		int variation;
		switch ( weaponNum ) {
		case WP_MACHINEGUN: haloName = "machinegun"; break;
		case WP_SHOTGUN: haloName = "shotgun"; break;
		case WP_ROCKET_LAUNCHER: haloName = "rocket"; break;
		case WP_RAILGUN: haloName = "railgun"; break;
		case WP_PLASMAGUN: haloName = "plasma"; break;
		case WP_LIGHTNING: haloName = "lightning"; break;
		case WP_BFG: haloName = "bfg"; break;
		case WP_GRENADE_LAUNCHER: haloName = "grenade"; break;
		default: break;
		}
		if ( haloName ) {
			CG_RegisterHaloView( weaponInfo, haloName );
   Com_sprintf(soundPath,sizeof(soundPath),"sound/qce/halo/pickup/%s1.wav",haloName);
   if(trap_FS_FOpenFile(soundPath,NULL,FS_READ)>0)weaponInfo->haloPickupSound=trap_S_RegisterSound(soundPath,qfalse);
   Com_sprintf(soundPath,sizeof(soundPath),"models/qce/halo/world/%s.md3",haloName);
   if(trap_FS_FOpenFile(soundPath,NULL,FS_READ)>0) {
    int axis;weaponInfo->weaponModel=trap_R_RegisterModel(soundPath);weaponInfo->barrelModel=0;
    trap_R_ModelBounds(weaponInfo->weaponModel,mins,maxs);
    for(axis=0;axis<3;axis++)weaponInfo->weaponMidpoint[axis]=(mins[axis]+maxs[axis])*0.5f;
   }
			Com_sprintf( soundPath, sizeof(soundPath), "sound/qce/halo/fire/%s1.wav", haloName );
			if ( trap_FS_FOpenFile( soundPath, NULL, FS_READ ) > 0 ) {
				memset( weaponInfo->flashSound, 0, sizeof(weaponInfo->flashSound) );
				for ( variation = 0; variation < 4; variation++ ) {
					Com_sprintf( soundPath, sizeof(soundPath), "sound/qce/halo/fire/%s%d.wav", haloName, variation + 1 );
					if ( trap_FS_FOpenFile( soundPath, NULL, FS_READ ) <= 0 ) break;
					weaponInfo->flashSound[variation] = trap_S_RegisterSound( soundPath, qfalse );
				}
				/* Quake's continuous hum loops do not accompany Halo shots. */
				weaponInfo->firingSound = 0;
				weaponInfo->readySound = 0;
			}
		}
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );

	itemInfo->icon = trap_R_RegisterShader( item->icon );

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame ) {

	// change weapon
	if ( frame >= ci->animations[TORSO_DROP].firstFrame 
		&& frame < ci->animations[TORSO_DROP].firstFrame + 9 ) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if ( frame >= ci->animations[TORSO_ATTACK].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[TORSO_ATTACK2].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK2].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}
	
	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 * 
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
	trace_t  trace;
	refEntity_t  beam;
	vec3_t   forward;
	vec3_t   muzzlePoint, endPoint;
	int      anim;

	if (cent->currentState.weapon != WP_LIGHTNING) {
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

	// CPMA  "true" lightning
	if ((cent->currentState.number == cg.predictedPlayerState.clientNum) && (cg_trueLightning.value != 0)) {
		vec3_t angle;
		int i;

		for (i = 0; i < 3; i++) {
			float a = cent->lerpAngles[i] - cg.refdefViewAngles[i];
			if (a > 180) {
				a -= 360;
			}
			if (a < -180) {
				a += 360;
			}

			angle[i] = cg.refdefViewAngles[i] + a * (1.0 - cg_trueLightning.value);
			if (angle[i] < 0) {
				angle[i] += 360;
			}
			if (angle[i] > 360) {
				angle[i] -= 360;
			}
		}

		AngleVectors(angle, forward, NULL, NULL );
		VectorCopy(cent->lerpOrigin, muzzlePoint );
//		VectorCopy(cg.refdef.vieworg, muzzlePoint );
	} else {
		// !CPMA
		AngleVectors( cent->lerpAngles, forward, NULL, NULL );
		VectorCopy(cent->lerpOrigin, muzzlePoint );
	}

	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzlePoint[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	VectorMA( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );

	// see if it hit a wall
	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, 
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene( &beam );

	// add the impact flare if it hit something
	if ( trace.fraction < 1.0 ) {
		vec3_t	angles;
		vec3_t	dir;

		VectorSubtract( beam.oldorigin, beam.origin, dir );
		VectorNormalize( dir );

		memset( &beam, 0, sizeof( beam ) );
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA( trace.endpos, -16, dir, beam.origin );

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis( angles, beam.axis );
		trap_R_AddRefEntityToScene( &beam );
	}
}
/*

static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
	trace_t		trace;
	refEntity_t		beam;
	vec3_t			forward;
	vec3_t			muzzlePoint, endPoint;

	if ( cent->currentState.weapon != WP_LIGHTNING ) {
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

	// find muzzle point for this frame
	VectorCopy( cent->lerpOrigin, muzzlePoint );
	AngleVectors( cent->lerpAngles, forward, NULL, NULL );

	// FIXME: crouch
	muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	VectorMA( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );

	// see if it hit a wall
	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, 
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene( &beam );

	// add the impact flare if it hit something
	if ( trace.fraction < 1.0 ) {
		vec3_t	angles;
		vec3_t	dir;

		VectorSubtract( beam.oldorigin, beam.origin, dir );
		VectorNormalize( dir );

		memset( &beam, 0, sizeof( beam ) );
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA( trace.endpos, -16, dir, beam.origin );

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis( angles, beam.axis );
		trap_R_AddRefEntityToScene( &beam );
	}
}
*/


/*
======================
CG_MachinegunSpinAngle
======================
*/
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
#ifdef MISSIONPACK
		if ( cent->currentState.weapon == WP_CHAINGUN && !cent->pe.barrelSpinning ) {
			trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, trap_S_RegisterSound( "sound/weapons/vulcan/wvulwind.wav", qfalse ) );
		}
#endif
	}

	return angle;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	if ( powerups & ( 1 << PW_INVIS ) ) {
		gun->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( gun );
	} else {
		trap_R_AddRefEntityToScene( gun );

		if ( powerups & ( 1 << PW_BATTLESUIT ) ) {
			gun->customShader = cgs.media.battleWeaponShader;
			trap_R_AddRefEntityToScene( gun );
		}
		if ( powerups & ( 1 << PW_QUAD ) ) {
			gun->customShader = cgs.media.quadWeaponShader;
			trap_R_AddRefEntityToScene( gun );
		}
	}
}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
static void CG_HaloMuzzle(centity_t *cent,int weapon,const vec3_t origin,int renderfx) {
 refEntity_t flash;int frame,count=cg_qceWorld.muzzleCounts[weapon],elapsed=cg.time-cent->muzzleFlashTime;
 if(!count || elapsed<0 || elapsed>=67)return;
 frame=elapsed*count/67;if(frame>=count)frame=count-1;
 memset(&flash,0,sizeof(flash));flash.reType=RT_SPRITE;flash.customShader=cg_qceWorld.muzzleShaders[weapon][frame];
 flash.radius=cg_qceWorld.muzzleRadius[weapon];flash.rotation=(cent->qceFireSequence*73)%360;
 flash.renderfx=renderfx;VectorCopy(origin,flash.origin);memset(flash.shaderRGBA,255,4);
 trap_R_AddRefEntityToScene(&flash);
 trap_R_AddLightToScene(origin,100,cg_qceWorld.muzzleColors[weapon][0],cg_qceWorld.muzzleColors[weapon][1],cg_qceWorld.muzzleColors[weapon][2]);
}

void CG_HaloWorldMuzzle(refEntity_t *gun,centity_t *cent) {
 vec3_t origin;int weapon=cent->currentState.weapon;
 if(weapon<=WP_GAUNTLET || weapon>=WP_NUM_WEAPONS || cent->muzzleFlashTime<=0)return;
 VectorMA(gun->origin,12,gun->axis[0],origin);
 CG_HaloMuzzle(cent,weapon,origin,gun->renderfx);
}

void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	vec3_t		angles;
	weapon_t	weaponNum;
	weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
	orientation_t	lerped;

	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if( weaponNum == WP_RAILGUN ) {
		clientInfo_t *ci = &cgs.clientinfo[cent->currentState.clientNum];
		if( cent->pe.railFireTime + 1500 > cg.time ) {
			int scale = 255 * ( cg.time - cent->pe.railFireTime ) / 1500;
			gun.shaderRGBA[0] = ( ci->c1RGBA[0] * scale ) >> 8;
			gun.shaderRGBA[1] = ( ci->c1RGBA[1] * scale ) >> 8;
			gun.shaderRGBA[2] = ( ci->c1RGBA[2] * scale ) >> 8;
			gun.shaderRGBA[3] = 255;
		}
		else {
			Byte4Copy( ci->c1RGBA, gun.shaderRGBA );
		}
	}

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel) {
		return;
	}

	if ( !ps ) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
			cent->pe.lightningFiring = qtrue;
		} else if ( weapon->readySound ) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
		}
	}

	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, "tag_weapon");
	VectorCopy(parent->origin, gun.origin);

	VectorMA(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	// Make weapon appear left-handed for 2 and centered for 3
	if(ps && cg_drawGun.integer == 2)
		VectorMA(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
	else if(!ps || cg_drawGun.integer != 3)
	       	VectorMA(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);

	VectorMA(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);

	MatrixMultiply(lerped.axis, ((refEntity_t *)parent)->axis, gun.axis);
	gun.backlerp = parent->backlerp;

	CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups );

	// add the spinning barrel
	if ( weapon->barrelModel ) {
		memset( &barrel, 0, sizeof( barrel ) );
		VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle( cent );
		AnglesToAxis( angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &gun, weapon->weaponModel, "tag_barrel" );

		CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.number];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on the single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}

	// add the flash
	if ( ( weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_GRAPPLING_HOOK )
		&& ( nonPredictedCent->currentState.eFlags & EF_FIRING ) ) 
	{
		// continuous flash
	} else {
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > (cg.predictedPlayerState.stats[STAT_QCE_COMBAT]?67:MUZZLE_FLASH_TIME) ) {
			return;
		}
	}

	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

 if(cg.predictedPlayerState.stats[STAT_QCE_COMBAT]) {
  CG_PositionEntityOnTag(&flash,&gun,weapon->weaponModel,"tag_flash");
  CG_HaloMuzzle(cent,weaponNum,flash.origin,flash.renderfx);return;
 }
	flash.hModel = weapon->flashModel;
	if (!flash.hModel) {
		return;
	}
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

	// colorize the railgun blast
	if ( weaponNum == WP_RAILGUN ) {
		clientInfo_t	*ci;

		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		flash.shaderRGBA[0] = 255 * ci->color1[0];
		flash.shaderRGBA[1] = 255 * ci->color1[1];
		flash.shaderRGBA[2] = 255 * ci->color1[2];
	}

	CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash");
	trap_R_AddRefEntityToScene( &flash );

	if ( ps || cg.renderingThirdPerson ||
		cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		// add lightning bolt
		CG_LightningBolt( nonPredictedCent, flash.origin );

		if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
			trap_R_AddLightToScene( flash.origin, 300 + (rand()&31), weapon->flashDlightColor[0],
				weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
/* Resolve attachment orientation in world space from the interpolated skeleton. */
static qboolean CG_HaloAttachment( refEntity_t *gun, const char *name, orientation_t *world ) {
 orientation_t local;
 int i;
 if(!trap_R_LerpTagRef(&local,gun,name))return qfalse;
 VectorCopy(gun->origin,world->origin);
 for(i=0;i<3;i++)VectorMA(world->origin,local.origin[i],gun->axis[i],world->origin);
 MatrixMultiply(local.axis,gun->axis,world->axis);
 return qtrue;
}

static void CG_HaloBrass( orientation_t *port, int weapon ) {
 localEntity_t *le;
 refEntity_t *re;
 float waterScale;
 if(cg_brassTime.integer<=0 || (weapon!=WP_MACHINEGUN && weapon!=WP_SHOTGUN && weapon!=WP_BFG && weapon!=WP_RAILGUN))return;
 le=CG_AllocLocalEntity();re=&le->refEntity;
 le->leType=LE_FRAGMENT;le->startTime=cg.time;le->endTime=cg.time+cg_brassTime.integer;
 le->pos.trType=TR_GRAVITY;le->pos.trTime=cg.time;
 VectorCopy(port->origin,le->pos.trBase);VectorCopy(port->origin,re->origin);
 waterScale=(CG_PointContents(port->origin,-1)&CONTENTS_WATER)?0.1f:1.0f;
 VectorScale(port->axis[0],(60+40*random())*waterScale,le->pos.trDelta);
 VectorMA(le->pos.trDelta,(30+20*random())*waterScale,port->axis[2],le->pos.trDelta);
 VectorAdd(le->pos.trDelta,cg.predictedPlayerState.velocity,le->pos.trDelta);
 AxisCopy(port->axis,re->axis);
 re->hModel=weapon==WP_SHOTGUN?cgs.media.shotgunBrassModel:cgs.media.machinegunBrassModel;
 le->bounceFactor=0.3f*waterScale;le->leFlags=LEF_TUMBLE;le->leBounceSoundType=LEBS_BRASS;le->leMarkType=LEMT_NONE;
 le->angles.trType=TR_LINEAR;le->angles.trTime=cg.time;
 vectoangles(port->axis[0],le->angles.trBase);VectorSet(le->angles.trDelta,90,180,90);
}

static void CG_HaloEffects( refEntity_t *gun, weaponInfo_t *weapon, centity_t *cent ) {
 orientation_t marker;
 qceViewPlayback_t *view=&cg.haloView;
 if(cent->qceFireWeapon==cg.predictedPlayerState.weapon && cent->muzzleFlashTime>0 && cg.time-cent->muzzleFlashTime<67 && CG_HaloAttachment(gun,"tag_flash",&marker)) {
  CG_HaloMuzzle(cent,cg.predictedPlayerState.weapon,marker.origin,gun->renderfx);
 }
 /* The fire sequence identifies a shot even when several render frames share its time. */
 if(view->ejectSequence!=cent->qceFireSequence) {
  view->ejectSequence=cent->qceFireSequence;
  if(cent->qceFireWeapon==cg.predictedPlayerState.weapon && cent->muzzleFlashTime>0 && cg.time-cent->muzzleFlashTime<200 && CG_HaloAttachment(gun,"tag_eject",&marker))CG_HaloBrass(&marker,cg.predictedPlayerState.weapon);
 }
 if(cg_qceFlashlight.integer && CG_HaloAttachment(gun,"tag_light",&marker)) {
  vec3_t end;
  trace_t hit;
  VectorMA(marker.origin,768,marker.axis[0],end);
  CG_Trace(&hit,marker.origin,NULL,NULL,end,cg.predictedPlayerState.clientNum,MASK_SOLID);
  VectorMA(hit.endpos,-8,marker.axis[0],end);
  trap_R_AddLightToScene(end,160,1.0f,0.95f,0.8f);
 }
}

/* Halo clips include both weapon geometry and shared Spartan arms. */
static qboolean CG_AddHaloViewWeapon( playerState_t *ps, weaponInfo_t *weapon ) {
 centity_t *cent=&cg.predictedPlayerEntity;
 const qce_weapondef_t *def=BG_QceWeaponDef(ps->weapon);
 qceViewPlayback_t *view=&cg.haloView;
 qceViewInput_t input;
 qceViewClip_t *anim;
 int clip,previous,elapsed,phaseMs=0;
 refEntity_t gun;
 if(!weapon->haloViewModel || !ps->stats[STAT_QCE_COMBAT])return qfalse;
 memset(&input,0,sizeof(input));input.weapon=ps->weapon;input.state=ps->weaponstate;input.now=cg.time;
 input.magazine=ps->weaponstate==WEAPON_RELOADING?(ps->qceReloadEmpty?0:1):BG_QceMagazine(ps,ps->weapon);input.weaponTime=ps->weaponTime;
 input.hot=BG_QceOverheated(ps,ps->weapon);input.charge=ps->qceChargeMs;input.chargeMs=def->charge_ms;
 input.fireTime=cent->qceFireWeapon==ps->weapon?cent->muzzleFlashTime:0;input.chargedFire=cent->qceChargedFire;
 input.grenadeTime=cent->qceGrenadeTime;input.meleeTime=cent->qceMeleeTime;input.reloadRounds=def->reload_rounds;
 switch(ps->weaponstate) {
 case WEAPON_RAISING:phaseMs=def->ready_ms;break;
 case WEAPON_DROPPING:phaseMs=233;break;
 case WEAPON_RELOAD_ENTER:phaseMs=def->reload_enter_ms;break;
 case WEAPON_RELOAD_EXIT:phaseMs=def->reload_exit_ms;break;
 case WEAPON_RELOAD_EXIT_EMPTY:phaseMs=def->reload_exit_empty_ms;break;
 case WEAPON_RELOADING:
  phaseMs=def->reload_rounds==1 || input.magazine>0?def->reload_ms:def->reload_empty_ms;break;
 case WEAPON_MELEEING:phaseMs=def->melee_ms;break;
 default:break;
 }
 {
  usercmd_t movement;
  qboolean available=!(ps->pm_flags&PMF_FOLLOW) && !cg.demoPlayback && trap_GetUserCmd(trap_GetCurrentCmdNumber(),&movement);
  input.moving=ps->groundEntityNum!=ENTITYNUM_NONE && (available?
   movement.forwardmove*movement.forwardmove+movement.rightmove*movement.rightmove>161:
   ps->velocity[0]*ps->velocity[0]+ps->velocity[1]*ps->velocity[1]>4);
 }
 input.phaseMs=phaseMs;previous=view->clip;
 /* Cut embedded reload audio when a switch cancels the corresponding action. */
 if(weapon->haloStopSound && (
    ((previous==QCE_VIEW_RELOAD_FULL || previous==QCE_VIEW_RELOAD_EMPTY || previous==QCE_VIEW_RELOAD_ENTER || previous==QCE_VIEW_CHARGE_ENTER || previous==QCE_VIEW_CHARGE) &&
     (view->weapon!=ps->weapon || ps->weaponstate==WEAPON_DROPPING || ps->weaponstate==WEAPON_MELEEING || ps->weaponstate==WEAPON_RAISING || input.grenadeTime>view->time)) ||
    ((previous==QCE_VIEW_CHARGE_ENTER || previous==QCE_VIEW_CHARGE) && input.charge<=0 && input.fireTime<=view->time)))
  trap_S_StartSound(NULL,ps->clientNum,CHAN_WEAPON,weapon->haloStopSound);
 clip=QCE_ViewSelect(view,&input,weapon->haloClips);
 anim=&weapon->haloClips[clip];elapsed=cg.time-view->start;
 if(clip==QCE_VIEW_IDLE || clip==QCE_VIEW_MOVING)phaseMs=0;
 if(clip==QCE_VIEW_CHARGE_ENTER)phaseMs=def->charge_ms;
 if(clip==QCE_VIEW_RELOAD_FULL || clip==QCE_VIEW_RELOAD_EMPTY)phaseMs=clip==QCE_VIEW_RELOAD_EMPTY?def->reload_empty_ms:def->reload_ms;
 if(previous!=clip && cg_debugAnim.integer)CG_Printf("QCE view weapon=%d clip=%d first=%d count=%d fps=%d\n",ps->weapon,clip,anim->first,anim->count,anim->fps);
 if(QCE_ViewSoundDue(view,elapsed,anim,&weapon->haloSounds[clip],phaseMs) && clip!=QCE_VIEW_CHARGED_FIRE && clip!=QCE_VIEW_MELEE && (clip!=QCE_VIEW_FIRE || weapon->haloSounds[clip].frame>0)) {
  qceViewSound_t *event=&weapon->haloSounds[clip];
  trap_S_StartSound(NULL,ps->clientNum,CHAN_WEAPON,event->sounds[rand()%event->count]);
  if(cg_debugAnim.integer)CG_Printf("QCE animation sound weapon=%d clip=%d source_frame=%d\n",ps->weapon,clip,event->frame);
 }
 if(input.charge>=input.chargeMs && input.chargeMs>0 && weapon->haloSounds[QCE_VIEW_CHARGE].loop && weapon->haloSounds[QCE_VIEW_CHARGE].count>0)
  trap_S_AddLoopingSound(ps->clientNum,cg.refdef.vieworg,vec3_origin,weapon->haloSounds[QCE_VIEW_CHARGE].sounds[0]);
 /* Audio and state continue while the model is hidden or tested. */
 if(!cg_drawGun.integer || cg.testGun || cg.renderingThirdPerson)return qtrue;
 memset(&gun,0,sizeof(gun));gun.hModel=weapon->haloViewModel;
 QCE_ViewFrames(anim,clip,elapsed,phaseMs,&gun.oldframe,&gun.frame,&gun.backlerp);
 if(weapon->haloMovingJoints && weapon->haloClips[QCE_VIEW_MOVING].count>0) {
  qceViewClip_t *moving=&weapon->haloClips[QCE_VIEW_MOVING];
  QCE_ViewMovement(view,cg.time,input.moving,ps->weaponstate);
  gun.qceOverlayWeight=view->moveWeight;gun.qceOverlayJoints=weapon->haloMovingJoints;
  QCE_ViewFrames(moving,QCE_VIEW_MOVING,view->moveElapsed,0,&gun.qceOverlayOldFrame,&gun.qceOverlayFrame,&gun.qceOverlayBacklerp);
 }
 VectorCopy(cg.refdef.vieworg,gun.origin);VectorCopy(gun.origin,gun.lightingOrigin);AxisCopy(cg.refdef.viewaxis,gun.axis);
 QCE_ViewSway(view,cg.time,cg.refdefViewAngles);
 VectorMA(gun.origin,view->swayOffset[YAW],gun.axis[1],gun.origin);
 VectorMA(gun.origin,-view->swayOffset[PITCH],gun.axis[2],gun.origin);
 VectorMA(gun.origin,cg_gun_x.value,gun.axis[0],gun.origin);
 VectorMA(gun.origin,cg_gun_y.value,gun.axis[1],gun.origin);
 VectorMA(gun.origin,cg_gun_z.value,gun.axis[2],gun.origin);
 if(ps->weaponstate==WEAPON_DROPPING && weapon->haloClips[QCE_VIEW_PUTAWAY].count<=0) {
  float fraction=(233-ps->weaponTime)/233.0f;
  if(fraction<0)fraction=0;
  if(fraction>1)fraction=1;
  VectorMA(gun.origin,-32*fraction,gun.axis[2],gun.origin);
 }
 {int axis;float scale=Com_Clamp(0.5f,1.5f,cg_qceWeaponScale.value);for(axis=0;axis<3;axis++)VectorScale(gun.axis[axis],scale,gun.axis[axis]);gun.nonNormalizedAxes=qtrue;}
 gun.renderfx=RF_DEPTHHACK|RF_FIRST_PERSON|RF_MINLIGHT;
 if(weapon->haloAmmoCounter) {
  int ammo=input.magazine;
  char path[MAX_QPATH];
  if(ammo<0)ammo=0;
  if(ammo>60)ammo=60;
  if(!weapon->haloAmmoSkins[ammo]) {
   Com_sprintf(path,sizeof(path),"models/qce/halo/view/%s_%d.skin",weapon->haloName,ammo);
   weapon->haloAmmoSkins[ammo]=trap_R_RegisterSkin(path);
  }
  gun.customSkin=weapon->haloAmmoSkins[ammo];
 }
 CG_AddWeaponWithPowerups(&gun,cent->currentState.powerups);CG_HaloEffects(&gun,weapon,cent);
 return qtrue;
}

void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

 if(ps->stats[STAT_HEALTH]<=0) {
  if(cg.haloView.weapon>WP_NONE && cg.haloView.weapon<WP_NUM_WEAPONS && cg_weapons[cg.haloView.weapon].haloStopSound)
   trap_S_StartSound(NULL,ps->clientNum,CHAN_WEAPON,cg_weapons[cg.haloView.weapon].haloStopSound);
  memset(&cg.haloView,0,sizeof(cg.haloView));return;
 }
	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

 CG_RegisterWeapon(ps->weapon);
 weapon=&cg_weapons[ps->weapon];
 if(CG_AddHaloViewWeapon(ps,weapon))return;

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}


	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cg_fov.integer > 90 ) {
		fovOffset = -0.2 * ( cg_fov.integer - 90 );
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {
		// get clientinfo for animation map
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM] );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int		i;
	int		bits;
	int		count;
	int		x, y, w;
	const char	*name;
	float	*color;

	if(cg.predictedPlayerState.stats[STAT_QCE_COMBAT] && cg.predictedPlayerState.qceMaxHeldWeapons!=9)return;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[ STAT_WEAPONS ];
	count = 0;
	for ( i = 1 ; i < MAX_WEAPONS ; i++ ) {
		if ( bits & ( 1 << i ) ) {
			count++;
		}
	}

	x = 320 - count * 20;
	y = 380;

	for ( i = 1 ; i < MAX_WEAPONS ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		CG_RegisterWeapon( i );

		// draw weapon icon
		CG_DrawPic( x, y, 32, 32, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			CG_DrawPic( x-4, y-4, 40, 40, cgs.media.selectShader );
		}

		// no ammo cross on top
		if ( !cg.snap->ps.ammo[ i ] ) {
			CG_DrawPic( x, y, 32, 32, cgs.media.noammoShader );
		}

		x += 40;
	}

	// draw the selected name
	if ( cg_weapons[ cg.weaponSelect ].item ) {
		name = cg.snap->ps.stats[STAT_QCE_COMBAT]?BG_QceWeaponName(cg.weaponSelect):cg_weapons[ cg.weaponSelect ].item->pickup_name;
		if ( name ) {
			w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
			x = ( SCREEN_WIDTH - w ) / 2;
			CG_DrawBigStringColor(x, y - 22, name, color);
		}
	}

	trap_R_SetColor( NULL );
}


/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
	if ( cg.snap->ps.stats[STAT_QCE_COMBAT] && i == WP_GAUNTLET ) return qfalse;
	if ( !cg.snap->ps.stats[STAT_QCE_COMBAT] && !cg.snap->ps.ammo[i] ) {
		return qfalse;
	}
	if ( ! (cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
		cg.weaponSelect++;
		if ( cg.weaponSelect == MAX_WEAPONS ) {
			cg.weaponSelect = 0;
		}
		if ( cg.weaponSelect == WP_GAUNTLET ) {
			continue;		// never cycle to gauntlet
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == MAX_WEAPONS ) {
		cg.weaponSelect = original;
	}
 if(cg.weaponSelect!=original)cg.weaponManualSelectTime=cg.time;
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
		cg.weaponSelect--;
		if ( cg.weaponSelect == -1 ) {
			cg.weaponSelect = MAX_WEAPONS - 1;
		}
		if ( cg.weaponSelect == WP_GAUNTLET ) {
			continue;		// never cycle to gauntlet
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == MAX_WEAPONS ) {
		cg.weaponSelect = original;
	}
 if(cg.weaponSelect!=original)cg.weaponManualSelectTime=cg.time;
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > MAX_WEAPONS-1 || (cg.snap->ps.stats[STAT_QCE_COMBAT] && num==WP_GAUNTLET) ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) ) {
		return;		// don't have the weapon
	}

	if(cg.weaponSelect!=num)cg.weaponManualSelectTime=cg.time;
	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int		i;
 if(cg.snap && cg.snap->ps.stats[STAT_QCE_COMBAT])return;

	cg.weaponSelectTime = cg.time;

	for ( i = MAX_WEAPONS-1 ; i > 0 ; i-- ) {
		if ( CG_WeaponSelectable( i ) ) {
			cg.weaponSelect = i;
			break;
		}
	}
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	CG_RegisterWeapon(ent->weapon);
 weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;
	cent->qceChargedFire = ent->eventParm & 1;
 cent->qceFireSequence++;
 cent->qceFireWeapon=ent->weapon;

	// lightning gun only does this this on initial press
	if ( ent->weapon == WP_LIGHTNING && !cg.predictedPlayerState.stats[STAT_QCE_COMBAT] ) {
		if ( cent->pe.lightningFiring ) {
			return;
		}
	}

	if( ent->weapon == WP_RAILGUN ) {
		cent->pe.railFireTime = cg.time;
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
		trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
	}

 /* The graph's fire sound is mechanical (pump/bolt/recoil), not the
    firing-effect gunshot. Play both; never substitute one for the other. */
 if(cg.predictedPlayerState.stats[STAT_QCE_COMBAT] && cent->qceChargedFire && weap->haloSounds[QCE_VIEW_CHARGED_FIRE].count>0) {
  qceViewSound_t *shot=&weap->haloSounds[QCE_VIEW_CHARGED_FIRE];
  trap_S_StartSound(NULL,ent->number,CHAN_AUTO,shot->sounds[rand()%shot->count]);
 } else {
  for(c=0;c<4 && weap->flashSound[c];c++);
  if(c>0)trap_S_StartSound(NULL,ent->number,CHAN_AUTO,weap->flashSound[rand()%c]);
  if(cg.predictedPlayerState.stats[STAT_QCE_COMBAT] && weap->haloSounds[QCE_VIEW_FIRE].count>0 &&
     (weap->haloSounds[QCE_VIEW_FIRE].frame==0 || ent->number!=cg.predictedPlayerState.clientNum)) {
   qceViewSound_t *mechanical=&weap->haloSounds[QCE_VIEW_FIRE];
   trap_S_StartSound(NULL,ent->number,CHAN_WEAPON,mechanical->sounds[rand()%mechanical->count]);
  }
 }

 if(cg_debugAnim.integer)CG_Printf("QCE fire weapon=%d effect_sound=%d mechanical_sound=%d charged=%d\n",ent->weapon,weap->flashSound[0],weap->haloSounds[QCE_VIEW_FIRE].count?weap->haloSounds[QCE_VIEW_FIRE].sounds[0]:0,cent->qceChargedFire);
	// Local Halo brass uses the current skeleton's ejection marker when drawn.
	if ( weap->ejectBrassFunc && cg_brassTime.integer > 0 &&
      !(ent->number==cg.predictedPlayerState.clientNum && cg.predictedPlayerState.stats[STAT_QCE_COMBAT] &&
        weap->haloViewModel && cg_drawGun.integer && !cg.renderingThirdPerson && !cg.testGun) ) {
		weap->ejectBrassFunc( cent );
	}
}


/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType ) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	int				r;
	qboolean		alphaFade;
	qboolean		isSprite;
	int				duration;
	vec3_t			sprOrg;
	vec3_t			sprVel;

 if(cg.predictedPlayerState.stats[STAT_QCE_COMBAT] && weapon==WP_GAUNTLET) {
  CG_RegisterWeapon(WP_GAUNTLET);
  trap_S_StartSound(origin,ENTITYNUM_WORLD,CHAN_AUTO,cg_weapons[WP_GAUNTLET].flashSound[0]);
  return;
 }
 if(cg.predictedPlayerState.stats[STAT_QCE_COMBAT] && (weapon==WP_PLASMAGUN || weapon==WP_LIGHTNING || weapon==WP_GRENADE_LAUNCHER || weapon==WP_ROCKET_LAUNCHER)) {
  if(weapon==WP_ROCKET_LAUNCHER)CG_HaloGrenadeExplosion(1,origin);
  else {
   le=CG_MakeExplosion(origin,dir,0,cg_qceWorld.plasmaExplosionShader,300,qtrue);
   le->color[0]=weapon==WP_GRENADE_LAUNCHER?0.8f:weapon==WP_LIGHTNING?0.3f:0.3f;
   le->color[1]=weapon==WP_GRENADE_LAUNCHER?0.3f:weapon==WP_LIGHTNING?1:0.65f;
   le->color[2]=weapon==WP_LIGHTNING?0.3f:1;
   trap_S_StartSound(origin,ENTITYNUM_WORLD,CHAN_AUTO,weapon==WP_GRENADE_LAUNCHER?cg_qceWorld.needleImpact:cg_qceWorld.plasmaImpact);
  }
  return;
 }

	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch ( weapon ) {
	default:
#ifdef MISSIONPACK
	case WP_NAILGUN:
		if( soundType == IMPACTSOUND_FLESH ) {
			sfx = cgs.media.sfx_nghitflesh;
		} else if( soundType == IMPACTSOUND_METAL ) {
			sfx = cgs.media.sfx_nghitmetal;
		} else {
			sfx = cgs.media.sfx_nghit;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
#endif
	case WP_LIGHTNING:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if ( r < 2 ) {
			sfx = cgs.media.sfx_lghit2;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
#ifdef MISSIONPACK
	case WP_PROX_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_proxexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
#endif
	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		if (cg_oldRocket.integer == 0) {
			// explosion sprite animation
			VectorMA( origin, 24, dir, sprOrg );
			VectorScale( dir, 64, sprVel );

			CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 20, 30 );
		}
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		//sfx = cgs.media.sfx_railg;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;

#ifdef MISSIONPACK
	case WP_CHAINGUN:
		mod = cgs.media.bulletFlashModel;
		if( soundType == IMPACTSOUND_FLESH ) {
			sfx = cgs.media.sfx_chghitflesh;
		} else if( soundType == IMPACTSOUND_METAL ) {
			sfx = cgs.media.sfx_chghitmetal;
		} else {
			sfx = cgs.media.sfx_chghit;
		}
		mark = cgs.media.bulletMarkShader;

		radius = 8;
		break;
#endif

	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if ( r == 0 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
	}

	if ( sfx && soundType!=IMPACTSOUND_SILENT ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, 
							   mod,	shader,
							   duration, isSprite );
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
		if ( weapon == WP_RAILGUN ) {
			// colorize with client color
			VectorCopy( cgs.clientinfo[clientNum].color1, le->color );
			le->refEntity.shaderRGBA[0] = le->color[0] * 0xff;
			le->refEntity.shaderRGBA[1] = le->color[1] * 0xff;
			le->refEntity.shaderRGBA[2] = le->color[2] * 0xff;
			le->refEntity.shaderRGBA[3] = 0xff;
		}
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	if ( weapon == WP_RAILGUN ) {
		float	*color;

		// colorize with client color
		color = cgs.clientinfo[clientNum].color1;
		CG_ImpactMark( mark, origin, dir, random()*360, color[0],color[1], color[2],1, alphaFade, radius, qfalse );
	} else {
		CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse );
	}
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum ) {
	if(cg.predictedPlayerState.stats[STAT_QCE_COMBAT] && (weapon==WP_PLASMAGUN || weapon==WP_LIGHTNING || weapon==WP_GRENADE_LAUNCHER || weapon==WP_ROCKET_LAUNCHER)) {
  CG_MissileHitWall(weapon,0,origin,dir,IMPACTSOUND_FLESH);return;
 }
 CG_Bleed( origin, entityNum );

 // some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
#ifdef MISSIONPACK
	case WP_NAILGUN:
	case WP_CHAINGUN:
	case WP_PROX_LAUNCHER:
#endif
		CG_MissileHitWall( weapon, 0, origin, dir, IMPACTSOUND_FLESH );
		break;
	default:
		break;
	}
}



/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet( vec3_t start, vec3_t end, int skipNum, int qceWeapon ) {
	trace_t		tr;
	int sourceContentType, destContentType;

	CG_Trace( &tr, start, NULL, NULL, end, skipNum, MASK_SHOT );
 if(qceWeapon && tr.fraction==1)return;

	sourceContentType = CG_PointContents( start, 0 );
	destContentType = CG_PointContents( tr.endpos, 0 );

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if ( sourceContentType == destContentType ) {
		if ( sourceContentType & CONTENTS_WATER ) {
			CG_BubbleTrail( start, tr.endpos, 32 );
		}
	} else if ( sourceContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( start, trace.endpos, 32 );
	} else if ( destContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( tr.endpos, trace.endpos, 32 );
	}

	if (  tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	if ( cg_entities[tr.entityNum].currentState.eType == ET_PLAYER ) {
		CG_MissileHitPlayer( WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum );
	} else {
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		}
		if ( tr.surfaceFlags & SURF_METALSTEPS ) {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL );
		} else {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT );
		}
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void CG_ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, int otherEntNum, int qceWeapon,int fraction ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;

	qceWeapon&=15;
	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0 ; i < (qceWeapon?BG_QceWeaponDef(qceWeapon)->pellets:DEFAULT_SHOTGUN_COUNT) ; i++ ) {
		r = Q_crandom( &seed ) * (qceWeapon?BG_QceSpread(qceWeapon,fraction):DEFAULT_SHOTGUN_SPREAD) * 16;
		u = Q_crandom( &seed ) * (qceWeapon?BG_QceSpread(qceWeapon,fraction):DEFAULT_SHOTGUN_SPREAD) * 16;
		VectorMA( origin, 8192 * 16, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);
  if(qceWeapon)BG_QceRayEnd(origin,end,qceWeapon);

		if(!qceWeapon)CG_ShotgunPellet( origin, end, otherEntNum, qceWeapon );
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire( entityState_t *es ) {
	vec3_t	v;
	int		contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t			up;

		contents = CG_PointContents( es->pos.trBase, 0 );
		if ( !( contents & CONTENTS_WATER ) ) {
			VectorSet( up, 0, 0, 8 );
			CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
		}
	}
	CG_ShotgunPattern( es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum, es->generic1, es->time2 );
}

/*
============================================================================

BULLETS

============================================================================
*/


/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest ) {
	vec3_t		forward, right;
	polyVert_t	verts[4];
	vec3_t		line;
	float		len, begin, end;
	vec3_t		start, finish;
	vec3_t		midpoint;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 ) {
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );

	midpoint[0] = ( start[0] + finish[0] ) * 0.5;
	midpoint[1] = ( start[1] + finish[1] ) * 0.5;
	midpoint[2] = ( start[2] + finish[2] ) * 0.5;

	// add the tracer sound
	trap_S_StartSound( midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound );

}


/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum, qboolean silent ) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t		start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && cg_tracerChance.value > 0 ) {
		if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
			sourceContentType = CG_PointContents( start, 0 );
			destContentType = CG_PointContents( end, 0 );

			// do a complete bubble trail if necessary
			if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
				CG_BubbleTrail( start, end, 32 );
			}
			// bubble trail from water into air
			else if ( ( sourceContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( start, trace.endpos, 32 );
			}
			// bubble trail from air into water
			else if ( ( destContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( trace.endpos, end, 32 );
			}

			// draw a tracer
			if ( random() < cg_tracerChance.value ) {
				CG_Tracer( start, end );
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
		CG_Bleed( end, fleshEntityNum );
	} else {
		CG_MissileHitWall( WP_MACHINEGUN, 0, end, normal, silent?IMPACTSOUND_SILENT:IMPACTSOUND_DEFAULT );
	}

}

/* Optional local media: absence keeps a source-only checkout playable. */
qceWorldMedia_t cg_qceWorld;
void CG_RegisterHaloWorld(void) {
 fileHandle_t file;int length,i,j;char buffer[4096],*text,*token,path[MAX_QPATH];
 memset(&cg_qceWorld,0,sizeof(cg_qceWorld));
 if(trap_FS_FOpenFile("models/qce/halo/player/body.skin",NULL,FS_READ)>0) {
  cg_qceWorld.bodySkin=trap_R_RegisterSkin("models/qce/halo/player/body.skin");cg_qceWorld.visorSkin=trap_R_RegisterSkin("models/qce/halo/player/visor.skin");
 }
 cg_qceWorld.shieldHitSound=trap_S_RegisterSound("sound/qce/halo/combat/shield-hit.wav",qfalse);
 cg_qceWorld.shieldBreakSound=trap_S_RegisterSound("sound/qce/halo/combat/shield-break.wav",qfalse);
 cg_qceWorld.plasmaImpact=trap_S_RegisterSound("sound/qce/halo/combat/plasma-impact.wav",qfalse);
 cg_qceWorld.needleImpact=trap_S_RegisterSound("sound/qce/halo/combat/needle-impact.wav",qfalse);
 cg_qceWorld.plasmaFlyby=trap_S_RegisterSound("sound/qce/halo/combat/plasma-flyby.wav",qfalse);
 cg_qceWorld.needleFlyby=trap_S_RegisterSound("sound/qce/halo/combat/needle-flyby.wav",qfalse);
 {char config[4096],*cursor;fileHandle_t handle;int length,weapon,count,j;
  length=trap_FS_FOpenFile("models/qce/halo/world/muzzle.cfg",&handle,FS_READ);
  if(length>0 && length<sizeof(config)) {
   trap_FS_Read(config,length,handle);config[length]=0;cursor=config;
   while(*cursor) {
    weapon=atoi(COM_Parse(&cursor));count=atoi(COM_Parse(&cursor));
    if(weapon<=WP_GAUNTLET || weapon>=WP_NUM_WEAPONS || count<1 || count>16)break;
    cg_qceWorld.muzzleCounts[weapon]=count;cg_qceWorld.muzzleRadius[weapon]=atof(COM_Parse(&cursor));
    for(j=0;j<3;j++)cg_qceWorld.muzzleColors[weapon][j]=atof(COM_Parse(&cursor));
    for(j=0;j<count;j++){Com_sprintf(path,sizeof(path),"qce/halo/muzzle-%d-%d",weapon,j);cg_qceWorld.muzzleShaders[weapon][j]=trap_R_RegisterShader(path);}
   }
  }
  if(handle)trap_FS_FCloseFile(handle);
 }
 cg_qceWorld.sniperSmokeShader=trap_R_RegisterShader("qce/halo/sniper-smoke");
 cg_qceWorld.shieldViewShader=trap_R_RegisterShader("qce/halo/shield-view");
 cg_qceWorld.shieldShader=trap_R_RegisterShader("qce/halo/shield-shell");
 cg_qceWorld.shieldBreakShader=trap_R_RegisterShader("qce/halo/shield-break");
 cg_qceWorld.plasmaShaders[0]=trap_R_RegisterShader("qce/halo/plasma-volume");
 cg_qceWorld.plasmaShaders[1]=cg_qceWorld.plasmaShaders[0];
 for(i=0;i<3;i++) {Com_sprintf(path,sizeof(path),"models/qce/halo/world/plasma-volume-%d.md3",i);if(trap_FS_FOpenFile(path,NULL,FS_READ)>0)cg_qceWorld.plasmaModels[i]=trap_R_RegisterModel(path);}
 {int n=trap_FS_FOpenFile("models/qce/halo/world/plasma-volumes.cfg",&file,FS_READ);
  if(n>0 && n<sizeof(buffer)){trap_FS_Read(buffer,n,file);buffer[n]=0;text=buffer;for(i=0;i<3;i++)for(j=0;j<14;j++)cg_qceWorld.plasmaVolumes[i][j]=atof(COM_Parse(&text));}
  if(n>=0)trap_FS_FCloseFile(file);
 }
 if(trap_FS_FOpenFile("models/qce/halo/world/needle-projectile.md3",NULL,FS_READ)>0)cg_qceWorld.projectileModels[0]=trap_R_RegisterModel("models/qce/halo/world/needle-projectile.md3");
 if(trap_FS_FOpenFile("models/qce/halo/world/rocket-projectile.md3",NULL,FS_READ)>0)cg_qceWorld.projectileModels[1]=trap_R_RegisterModel("models/qce/halo/world/rocket-projectile.md3");
 {char reticles[24000],*input;int n=trap_FS_FOpenFile("ui/qce/halo-crosshairs.cfg",&file,FS_READ);
  if(n>0 && n<sizeof(reticles)) {
   trap_FS_Read(reticles,n,file);reticles[n]=0;input=reticles;
   while(*(token=COM_Parse(&input))) {
    int w=atoi(token),index;float values[6];qhandle_t shader;
    if(w<0 || w>=WP_NUM_WEAPONS || cg_qceWorld.crosshairCount[w]>=32)break;
    shader=trap_R_RegisterShaderNoMip(COM_Parse(&input));
    for(j=0;j<6;j++)values[j]=atof(COM_Parse(&input));
    index=cg_qceWorld.crosshairCount[w]++;cg_qceWorld.crosshairs[w][index].shader=shader;
    cg_qceWorld.crosshairs[w][index].width=values[0];cg_qceWorld.crosshairs[w][index].height=values[1];
    cg_qceWorld.crosshairs[w][index].x=values[2];cg_qceWorld.crosshairs[w][index].y=values[3];
    cg_qceWorld.crosshairs[w][index].flags=(int)values[4];cg_qceWorld.crosshairs[w][index].frame=(int)values[5];
   }
  }
  if(n>=0)trap_FS_FCloseFile(file);
 }
 length=trap_FS_FOpenFile("models/qce/halo/player/spartan.cfg",&file,FS_READ);
 if(length>0 && length<(int)sizeof(buffer)) {
  trap_FS_Read(buffer,length,file);buffer[length]=0;text=buffer;
  for(i=0;i<65;i++) {
   int values[4];
   for(j=0;j<4;j++){token=COM_Parse(&text);values[j]=atoi(token);}
   if(values[0]<0 || values[1]<1 || values[0]+values[1]>4096 || values[2]<1 || values[2]>100)break;
   cg_qceWorld.playerClips[i].first=values[0];cg_qceWorld.playerClips[i].count=values[1];cg_qceWorld.playerClips[i].fps=values[2];cg_qceWorld.playerClips[i].loop=0;
  }
  if(i>=49)cg_qceWorld.playerModel=trap_R_RegisterModel("models/qce/halo/player/spartan.iqm");
 }
 if(file)trap_FS_FCloseFile(file);
 if(trap_FS_FOpenFile("models/qce/halo/player/head.md3",NULL,FS_READ)>0)cg_qceWorld.headModel=trap_R_RegisterModel("models/qce/halo/player/head.md3");
 cg_qceWorld.headBodySkin=trap_R_RegisterSkin("models/qce/halo/player/head-body.skin");
 cg_qceWorld.headVisorSkin=trap_R_RegisterSkin("models/qce/halo/player/head-visor.skin");
 for(i=0;i<2;i++) {
  Com_sprintf(path,sizeof(path),"models/qce/halo/world/%s-grenade.md3",i?"plasma":"frag");
  if(trap_FS_FOpenFile(path,NULL,FS_READ)>0)cg_qceWorld.grenadeModels[i]=trap_R_RegisterModel(path);
  Com_sprintf(path,sizeof(path),"sound/qce/halo/grenade/%s-explode1.wav",i?"plasma":"frag");
  if(trap_FS_FOpenFile(path,NULL,FS_READ)>0)cg_qceWorld.grenadeExplode[i]=trap_S_RegisterSound(path,qfalse);
  Com_sprintf(path,sizeof(path),"sound/qce/halo/grenade/%s1.wav",i?"bounce-metal":"bounce");
  if(trap_FS_FOpenFile(path,NULL,FS_READ)>0)cg_qceWorld.grenadeBounce[i]=trap_S_RegisterSound(path,qfalse);
 }
 if(trap_FS_FOpenFile("sound/qce/halo/grenade/throw1.wav",NULL,FS_READ)>0)cg_qceWorld.grenadeThrow=trap_S_RegisterSound("sound/qce/halo/grenade/throw1.wav",qfalse);
 if(trap_FS_FOpenFile("sound/qce/halo/grenade/plasma-loop1.wav",NULL,FS_READ)>0)cg_qceWorld.grenadeLoop=trap_S_RegisterSound("sound/qce/halo/grenade/plasma-loop1.wav",qfalse);
 {int row,n;const char *names[3]={"grenade-explosion","plasma-explosion","grenade-smoke"};
  for(row=0;row<3;row++)for(n=0;n<64;n++) {
   Com_sprintf(path,sizeof(path),"textures/qce/particles/%s/%02d.tga",names[row],n);
   {fileHandle_t spriteFile;byte header[18];int size;
    size=trap_FS_FOpenFile(path,&spriteFile,FS_READ);
    if(size<18){if(size>=0)trap_FS_FCloseFile(spriteFile);break;}
    trap_FS_Read(header,18,spriteFile);trap_FS_FCloseFile(spriteFile);
    cg_qceWorld.grenadeParticleWidths[row][n]=header[12]+256*header[13];
   }
   Com_sprintf(path,sizeof(path),n?"qce/halo/%s-%02d":"qce/halo/%s",names[row],n);
   cg_qceWorld.grenadeParticleShaders[row][n]=trap_R_RegisterShader(path);
   cg_qceWorld.grenadeParticleCounts[row]++;
  }
 }
 cg_qceWorld.grenadeShader=trap_R_RegisterShader("qce/halo/grenade-explosion");
 cg_qceWorld.plasmaExplosionShader=trap_R_RegisterShader("qce/halo/plasma-explosion");
 cg_qceWorld.grenadeSmokeShader=trap_R_RegisterShader("qce/halo/grenade-smoke");
 for(i=0;i<4;i++) {
  const char *types[]={"normal","metal","splash"};int rows[]={FOOTSTEP_BOOT,FOOTSTEP_METAL,FOOTSTEP_SPLASH};
  for(j=0;j<3;j++) {
   Com_sprintf(path,sizeof(path),"sound/qce/halo/footstep/%s%d.wav",types[j],i+1);
   if(trap_FS_FOpenFile(path,NULL,FS_READ)>0){
    sfxHandle_t sound=trap_S_RegisterSound(path,qfalse);int row;
    cgs.media.footsteps[rows[j]][i]=sound;
    if(j==0)for(row=FOOTSTEP_NORMAL;row<=FOOTSTEP_ENERGY;row++)cgs.media.footsteps[row][i]=sound;
   }
  }
 }
}
void CG_HaloGrenadeExplosion(int type,vec3_t origin) {
 int i,state,j,kind=type==2?1:0,extra,grounded;float birth,transition,rate;
 trace_t ground;vec3_t below;
 const qce_particle_type_t *def=&qce_grenade_particles[kind];
 birth=def->emitterDuration[0]+random()*(def->emitterDuration[1]-def->emitterDuration[0]);
 transition=def->emitterTransition[0]+random()*(def->emitterTransition[1]-def->emitterTransition[0]);
 rate=def->creationRate[0];extra=(int)(rate*(birth+transition*0.5f));
 VectorCopy(origin,below);below[2]-=8;CG_Trace(&ground,origin,NULL,NULL,below,ENTITYNUM_NONE,MASK_SOLID);
 grounded=ground.fraction<1 && ground.plane.normal[2]>0.7f;
 trap_S_StartSound(origin,ENTITYNUM_WORLD,CHAN_AUTO,cg_qceWorld.grenadeExplode[kind]);
 for(i=0;i<def->count+extra;i++) {
  localEntity_t *le=CG_AllocLocalEntity();int total=0,spawnMs=0,spriteSeed=rand();
  if(i>=def->count && rate>0) {
   float n=(i-def->count+1)/(float)rate,t;
   if(n<=birth)t=n;
   else t=birth+transition*(1-sqrt(1-2*(n-birth)/transition));
   spawnMs=(int)(ceil(t*30)*1000/30); /* Match the source emission tick. */
  }
  le->leType=LE_QCE_GRENADE_PARTICLE;le->qceParticleType=kind;le->startTime=cg.time+spawnMs;
  le->qceParticleEmitterStart=cg.time;le->qceParticleEmitterBirth=(int)(birth*1000);le->qceParticleEmitterTransition=(int)(transition*1000);
  memcpy(le->qceParticleTypeColor,def->typeColor,sizeof(le->qceParticleTypeColor));
  for(state=0;state<3;state++) {
   const qce_particle_state_t *ps=&def->states[state];float f=random();
   int row=kind?1:(state?2:0),count=cg_qceWorld.grenadeParticleCounts[row],sprite=count?spriteSeed%count:0;
   float width=count?cg_qceWorld.grenadeParticleWidths[row][sprite]:36;
   le->qceParticlePhysics[state][0]=ps->mass;le->qceParticlePhysics[state][1]=ps->buoyancy;
   le->qceParticlePhysics[state][2]=ps->friction;le->qceParticlePhysics[state][3]=ps->radius;
   for(j=0;j<4;j++)le->qceParticleColor[state][j]=ps->argbMin[j]+f*(ps->argbMax[j]-ps->argbMin[j]);
   /* Source scale multiplies sprite pixels, converted to our chosen world scale. */
   le->qceParticleScale[state]=(ps->scale[0]+random()*(ps->scale[1]-ps->scale[0]))*width*80.0f*0.5f;
   le->qceParticleRotation[state]=(ps->rotation[0]+random()*(ps->rotation[1]-ps->rotation[0]))*180.0f/M_PI;
   le->qceParticleShader[state]=count?cg_qceWorld.grenadeParticleShaders[row][sprite]:(kind?cg_qceWorld.plasmaExplosionShader:(state?cg_qceWorld.grenadeSmokeShader:cg_qceWorld.grenadeShader));
   if(state<2) {
    le->qceParticlePhaseMs[state*2]=(int)((ps->duration[0]+random()*(ps->duration[1]-ps->duration[0]))*1000);
    le->qceParticlePhaseMs[state*2+1]=(int)((ps->transition[0]+random()*(ps->transition[1]-ps->transition[0]))*1000);
    total+=le->qceParticlePhaseMs[state*2]+le->qceParticlePhaseMs[state*2+1];
   }
  }
  le->endTime=le->startTime+(total>0?total:1);
  if(le->endTime>cg.time+le->qceParticleEmitterBirth+le->qceParticleEmitterTransition)
   le->endTime=cg.time+le->qceParticleEmitterBirth+le->qceParticleEmitterTransition;
  le->pos.trType=TR_LINEAR;le->pos.trTime=le->startTime;
  {float z=crandom(),theta=random()*2*M_PI,r=sqrt(1-z*z);vec3_t offset;
   if(grounded)z=fabs(z);
   VectorSet(offset,r*cos(theta)*def->explosion[0]*80,r*sin(theta)*def->explosion[0]*80,z*def->explosion[1]*80);
   VectorAdd(origin,offset,le->pos.trBase);VectorScale(offset,def->explosion[2],le->pos.trDelta);
  }
  le->refEntity.reType=RT_SPRITE;le->refEntity.rotation=random()*360;
 }
}

void CG_HaloWorldFrames(const qceViewClip_t *clip,int oneshot,int elapsed,int *oldframe,int *frame,float *backlerp) {
 QCE_ViewFrames(clip,oneshot?QCE_VIEW_FIRE:QCE_VIEW_IDLE,elapsed,0,oldframe,frame,backlerp);
}
