#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];
level_locals_t level;
vmCvar_t g_gametype;
gentity_t *G_Spawn(void) {gentity_t *ent=&g_entities[13];memset(ent,0,sizeof(*ent));ent->inuse=qtrue;ent->s.number=13;return ent;}
qboolean OnSameTeam(gentity_t *a,gentity_t *b) {(void)a;(void)b;return qfalse;}
void G_ExplodeMissile(gentity_t *ent);
static int damage_calls,damage_total,splash_calls,splash_total,blocked;
static gentity_t *last_attacker;
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int n,const char *fmt,...) {(void)n;(void)fmt;abort();}
void G_AddEvent(gentity_t *ent,int event,int parm) {(void)ent;(void)parm;assert(event==EV_MISSILE_MISS);}
void G_SetOrigin(gentity_t *ent,vec3_t origin) {VectorCopy(origin,ent->r.currentOrigin);VectorCopy(origin,ent->s.pos.trBase);ent->s.pos.trType=TR_STATIONARY;}
void trap_LinkEntity(gentity_t *ent) {(void)ent;}
void G_Damage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,vec3_t dir,vec3_t point,int damage,int flags,int mod) {
 (void)target;(void)inflictor;(void)dir;(void)point;(void)flags;assert(mod==MOD_GRENADE);
 damage_calls++;damage_total+=damage;last_attacker=attacker;
}
qboolean G_RadiusDamage(vec3_t origin,gentity_t *attacker,float damage,float radius,gentity_t *ignore,int mod) {
 (void)origin;(void)attacker;(void)ignore;assert(radius==80 && mod==MOD_GRENADE_SPLASH);splash_calls++;splash_total+=(int)damage;return qtrue;
}
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 (void)start;(void)mins;(void)maxs;(void)pass;(void)mask;memset(tr,0,sizeof(*tr));tr->fraction=blocked?0.5:1;tr->entityNum=ENTITYNUM_WORLD;VectorCopy(end,tr->endpos);
}
static gentity_t *needle(int index) {
 gentity_t *ent=&g_entities[index];memset(ent,0,sizeof(*ent));ent->inuse=qtrue;ent->s.number=index;ent->qceNeedle=1;
 ent->damage=10;ent->methodOfDeath=MOD_GRENADE;ent->splashMethodOfDeath=MOD_GRENADE_SPLASH;
 ent->r.ownerNum=0;ent->qceOwnerSerial=10;ent->parent=&g_entities[0];ent->s.eType=ET_MISSILE;return ent;
}
int main(void) {
 gclient_t shooter={0},victim={0};gentity_t *target=&g_entities[1],*ent;trace_t hit={0};int i;
 level.num_entities=20;level.maxclients=2;level.time=100;
 g_entities[0].inuse=qtrue;g_entities[0].client=&shooter;g_entities[0].qceEntitySerial=10;
 target->inuse=qtrue;target->client=&victim;target->qceEntitySerial=20;target->takedamage=qtrue;target->health=75;target->s.number=1;
 victim.ps.persistant[PERS_SPAWN_COUNT]=3;VectorSet(target->r.currentOrigin,0,100,0);
 hit.entityNum=1;VectorSet(hit.endpos,2,100,0);
 ent=needle(2);assert(G_QceNeedleImpact(ent,&hit));assert(damage_calls==0 && ent->nextthink==850 && ent->qceStuck);
 level.time=850;G_QceNeedleThink(ent);assert(damage_calls==1 && damage_total==10 && ent->freeAfterEvent && splash_calls==0);
 memset(ent,0,sizeof(*ent));damage_calls=damage_total=0;level.time=100;
 for(i=2;i<9;i++){ent=needle(i);G_QceNeedleImpact(ent,&hit);if(i<8)assert(ent->nextthink==850 && !ent->qceNeedleSpent);}
 assert(ent->nextthink==101 && ent->splashDamage==60 && ent->splashRadius==80 && ent->damage==0);
 level.time=101;for(i=2;i<9;i++){assert(g_entities[i].qceNeedleSpent);G_QceNeedleThink(&g_entities[i]);}
 assert(splash_calls==1 && splash_total==60 && damage_total==60);
 /* A fresh needle cannot reuse the already-combined attachments. */
 ent=needle(9);level.time=102;G_QceNeedleImpact(ent,&hit);assert(!ent->qceNeedleSpent && ent->nextthink==852);
 victim.ps.persistant[PERS_SPAWN_COUNT]++;G_QceNeedleThink(ent);assert(damage_total==60);
 ent=needle(10);G_QceNeedleImpact(ent,&hit);g_entities[0].qceEntitySerial++;
 G_QceNeedleThink(ent);assert(last_attacker==&g_entities[ENTITYNUM_WORLD]);
 /* A 100 ms frame turns at most nine degrees, preserving speed. */
 ent=needle(11);ent->qceTrackEntity=1;ent->qceTrackSerial=20;ent->qceTrackSpawn=victim.ps.persistant[PERS_SPAWN_COUNT];
 ent->qceTrackTime=100;level.previousTime=100;level.time=200;VectorSet(ent->s.pos.trDelta,320,0,0);
 G_QceTrackNeedle(ent);assert(fabs(VectorLength(ent->s.pos.trDelta)-320)<0.01);
 assert(fabs(atan2(ent->s.pos.trDelta[1],ent->s.pos.trDelta[0])-M_PI/20)<0.0001);
 blocked=1;ent->qceTrackTime=200;level.time=300;G_QceTrackNeedle(ent);assert(fabs(atan2(ent->s.pos.trDelta[1],ent->s.pos.trDelta[0])-M_PI/20)<0.0001);
 victim.ps.persistant[PERS_SPAWN_COUNT]++;blocked=0;level.time=400;G_QceTrackNeedle(ent);assert(ent->qceTrackEntity==ENTITYNUM_NONE);
 ent=needle(12);hit.entityNum=ENTITYNUM_WORLD;G_QceNeedleImpact(ent,&hit);assert(ent->nextthink==1150);G_QceNeedleThink(ent);assert(splash_calls==1);
 ent->qceNeedle=0;assert(!G_QceNeedleImpact(ent,&hit));
 {
  vec3_t start={0,0,0},dir={0,1,0};
  shooter.ps.stats[STAT_QCE_COMBAT]=1;shooter.ps.weapon=WP_GRENADE_LAUNCHER;
  g_entities[0].s.number=0;g_entities[0].qceEntitySerial=10;
  ent=fire_plasma(&g_entities[0],start,dir);
  assert(ent->qceNeedle && ent->qceTrackEntity==1 && ent->qceTrackSerial==20);
  assert(ent->nextthink>=level.time+4999 && ent->nextthink<=level.time+5000 && ent->think==G_QceNeedleThink);
  blocked=1;ent=fire_plasma(&g_entities[0],start,dir);assert(ent->qceTrackEntity==ENTITYNUM_NONE);blocked=0;
  VectorSet(target->r.currentOrigin,100,0,0);ent=fire_plasma(&g_entities[0],start,dir);assert(ent->qceTrackEntity==ENTITYNUM_NONE);
  shooter.ps.weapon=WP_LIGHTNING;VectorSet(target->r.currentOrigin,0,100,0);
  ent=fire_qce_overcharge(&g_entities[0],start,dir);
  assert(ent->qceCharged && !ent->qceNeedle && ent->qceTrackEntity==1 && ent->methodOfDeath==MOD_QCE_OVERCHARGE && ent->damage==70 && !ent->splashDamage);
  assert(ent->qceOwnerSerial==10 && fabs(VectorLength(ent->s.pos.trDelta)-1200)<0.01);
  assert(ent->nextthink>=level.time+2666 && ent->nextthink<=level.time+2667);
  ent->qceTrackTime=level.time;level.previousTime=level.time;level.time+=100;
  VectorSet(target->r.currentOrigin,100,0,0);G_QceTrackNeedle(ent);
  assert(fabs(atan2(ent->s.pos.trDelta[0],ent->s.pos.trDelta[1])-BG_QceWeaponDef(WP_LIGHTNING)->charged_tracking_radians/10)<0.0001);
  shooter.ps.weapon=WP_PLASMAGUN;ent=fire_plasma(&g_entities[0],start,dir);
  assert(!ent->qceNeedle && ent->think==G_ExplodeMissile && ent->nextthink>level.time);
 }
 puts("PASS: delayed needle damage, seven-needle combine, consumed groups, world contacts, respawn/owner reuse safety, bounded homing and blocked sightlines");return 0;
}
