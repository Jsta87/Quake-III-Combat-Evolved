/* Include the real handler to initialize its private firing context without test hooks. */
#include "../engine/code/game/g_weapon.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];
static float distance;
static int calls,amount,projectiles;
void G_QceFireBullet(gentity_t *owner,vec3_t start,vec3_t end,int weapon,int mod,int quad) {vec3_t d;(void)owner;assert(weapon==WP_SHOTGUN && mod==MOD_SHOTGUN && quad==1);VectorSubtract(end,start,d);assert(fabs(VectorLength(d)-3200)<0.1);projectiles++;}
static gentity_t event;
gentity_t *G_TempEntity(vec3_t origin,int type) {assert(type==EV_SHOTGUN);memset(&event,0,sizeof(event));VectorCopy(origin,event.s.pos.trBase);return &event;}
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int n,const char *fmt,...) {(void)n;(void)fmt;abort();}
qboolean OnSameTeam(gentity_t *a,gentity_t *b) {(void)a;(void)b;return qfalse;}
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 (void)start;(void)mins;(void)maxs;(void)end;(void)pass;(void)mask;memset(tr,0,sizeof(*tr));tr->entityNum=1;tr->fraction=0.5;VectorSet(tr->endpos,distance,0,0);
}
void G_Damage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,vec3_t dir,vec3_t point,int damage,int flags,int mod) {
 (void)target;(void)inflictor;(void)attacker;(void)dir;(void)point;(void)flags;assert(mod==MOD_SHOTGUN);calls++;amount=damage;
}
int main(void) {
 gclient_t shooter={0},victim={0};vec3_t start={0,0,0},end={3200,0,0};gentity_t *ent=&g_entities[0];
 ent->client=&shooter;ent->s.weapon=WP_SHOTGUN;shooter.ps.stats[STAT_QCE_COMBAT]=1;
 g_entities[1].client=&victim;g_entities[1].takedamage=qtrue;victim.ps.stats[STAT_HEALTH]=75;s_quadFactor=1;
 distance=120;assert(ShotgunPellet(start,end,ent));assert(calls==1 && amount>=18 && amount<=25);
 distance=180;assert(ShotgunPellet(start,end,ent));assert(calls==2 && amount>=14 && amount<=18);
 distance=240;assert(ShotgunPellet(start,end,ent));assert(calls==3 && amount==8);
 shooter.ps.stats[STAT_QCE_COMBAT]=0;distance=1000;assert(ShotgunPellet(start,end,ent));assert(calls==4 && amount==10);
 shooter.ps.stats[STAT_QCE_COMBAT]=1;distance=120;s_qceError=127;s_qceSpread=BG_QceSpread(WP_SHOTGUN,127);VectorSet(forward,1,0,0);VectorClear(muzzle);
 weapon_supershotgun_fire(ent);assert(event.s.generic1==WP_SHOTGUN && event.s.time2==127 && calls==4 && projectiles==15);
 puts("PASS: legacy pellet helper minimum/random attenuation, stock damage and production shotgun finite-projectile dispatch");return 0;
}
