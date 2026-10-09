#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];
level_locals_t level;
static int bounces,links,thinks;
static gentity_t bounce;
gentity_t *G_TempEntity(vec3_t origin,int event) {(void)origin;assert(event==EV_GRENADE_BOUNCE);bounces++;memset(&bounce,0,sizeof(bounce));return &bounce;}
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int n,const char *fmt,...) {(void)n;(void)fmt;abort();}
void G_AddEvent(gentity_t *ent,int event,int parm) {(void)ent;(void)parm;assert(event==EV_GRENADE_BOUNCE);bounces++;}
void G_SetOrigin(gentity_t *ent,vec3_t origin) {VectorCopy(origin,ent->r.currentOrigin);VectorCopy(origin,ent->s.pos.trBase);ent->s.pos.trType=TR_STATIONARY;}
void trap_LinkEntity(gentity_t *ent) {(void)ent;links++;}
void G_RunThink(gentity_t *ent) {(void)ent;thinks++;}
int main(void) {
 gentity_t grenade={0},*target=&g_entities[1];gclient_t client={0};trace_t hit={0};
 hit.entityNum=1;VectorSet(hit.endpos,12,20,30);
 target->inuse=qtrue;target->takedamage=qtrue;target->client=&client;target->qceEntitySerial=42;
 client.ps.persistant[PERS_SPAWN_COUNT]=3;VectorSet(target->r.currentOrigin,10,20,30);level.time=100;
 grenade.qceGrenadeType=1;assert(G_QceGrenadeImpact(&grenade,&hit));assert(bounces==1 && !grenade.qceStuck);
 assert(!grenade.qceFuseArmed); /* Body/wall contact must not arm a frag. */
 hit.entityNum=ENTITYNUM_WORLD;VectorSet(hit.plane.normal,0,0,-1);
 G_QceGrenadeImpact(&grenade,&hit);assert(!grenade.qceFuseArmed);
 VectorSet(hit.plane.normal,0.8,0,0.6);G_QceGrenadeImpact(&grenade,&hit);
 assert(grenade.qceFuseArmed && grenade.nextthink==800 && bounce.s.generic1==1);
 level.time=200;G_QceGrenadeImpact(&grenade,&hit);assert(grenade.nextthink==800);
 memset(&grenade,0,sizeof(grenade));level.time=100;
 grenade.qceGrenadeType=2;hit.entityNum=1;assert(G_QceGrenadeImpact(&grenade,&hit));
 assert(grenade.qceStuck && grenade.nextthink==2100 && grenade.s.pos.trType==TR_STATIONARY);
 target->r.currentOrigin[0]=20;assert(G_QceRunStuckGrenade(&grenade));assert(fabs(grenade.r.currentOrigin[0]-22)<0.01);
 client.ps.viewangles[YAW]=90;G_QceRunStuckGrenade(&grenade);
 assert(fabs(grenade.r.currentOrigin[0]-20)<0.01 && fabs(grenade.r.currentOrigin[1]-22)<0.01);
 client.ps.persistant[PERS_SPAWN_COUNT]++;target->r.currentOrigin[0]=100;
 G_QceRunStuckGrenade(&grenade);assert(grenade.qceAttachEntity==ENTITYNUM_WORLD && grenade.r.currentOrigin[0]<30);
 memset(&grenade,0,sizeof(grenade));grenade.qceGrenadeType=2;G_QceGrenadeImpact(&grenade,&hit);
 target->qceEntitySerial++;G_QceRunStuckGrenade(&grenade);assert(grenade.qceAttachEntity==ENTITYNUM_WORLD);
 memset(&grenade,0,sizeof(grenade));grenade.qceGrenadeType=2;hit.entityNum=ENTITYNUM_WORLD;
 VectorSet(hit.plane.normal,0,0,1);VectorSet(grenade.s.pos.trDelta,100,0,-100);
 G_QceGrenadeImpact(&grenade,&hit);assert(!grenade.qceStuck && !grenade.qceFuseArmed && grenade.s.pos.trType==TR_LINEAR);
 memset(&grenade,0,sizeof(grenade));grenade.qceGrenadeType=2;G_QceGrenadeImpact(&grenade,&hit);assert(grenade.qceFuseArmed && grenade.nextthink==2100);
 memset(&grenade,0,sizeof(grenade));assert(!G_QceGrenadeImpact(&grenade,&hit) && !G_QceRunStuckGrenade(&grenade));
 memset(&grenade,0,sizeof(grenade));grenade.qceGrenadeType=1;VectorSet(grenade.s.pos.trDelta,100,0,-100);VectorSet(hit.plane.normal,0,0,1);
 G_QceGrenadeImpact(&grenade,&hit);assert(fabs(grenade.s.pos.trDelta[0]-20)<0.01 && fabs(grenade.s.pos.trDelta[2]-30)<0.01);
 assert(links>0 && thinks>0);puts("PASS: hand-frag body bounce, sticky plasma arming, moving/rotating attachment, world contact, respawn/entity-reuse detachment and stock fallback");
 return 0;
}
