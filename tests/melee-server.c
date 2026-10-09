#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];
static gclient_t clients[2];
static gentity_t impact;
static float targetDistance=40,wallDistance=1000;
static int expectedFlags;
static int damageCalls,traceCalls,impactCalls,invalidTrace;
void QDECL Com_Printf(const char *fmt, ...) {(void)fmt;}
void QDECL Com_Error(int level,const char *fmt,...) {(void)level;(void)fmt;abort();}
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 float distance=end[0]-start[0],nearest=targetDistance;
 traceCalls++;assert(!mins && !maxs && pass==0 && mask==MASK_SHOT);
 assert(fabsf(start[2]-50)<0.01f);
 assert(fabsf(VectorLength((float[3]){end[0]-start[0],end[1]-start[1],end[2]-start[2]})-QCE_MELEE_REACH)<0.01f);
 memset(tr,0,sizeof(*tr));tr->fraction=1;tr->entityNum=ENTITYNUM_NONE;VectorCopy(end,tr->endpos);
 if(distance<=0)return;
 if(wallDistance<nearest)nearest=wallDistance;
 if(nearest>distance)return;
 tr->fraction=nearest/distance;tr->endpos[0]=start[0]+nearest;tr->plane.normal[0]=-1;
 tr->entityNum=wallDistance<targetDistance?ENTITYNUM_WORLD:1;
 if(invalidTrace==1)tr->startsolid=qtrue;
 if(invalidTrace==2)tr->surfaceFlags=SURF_NOIMPACT;
}
void G_Damage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,vec3_t dir,vec3_t point,int damage,int flags,int mod) {
 damageCalls++;
 assert(target==&g_entities[1] && inflictor==&g_entities[0] && attacker==inflictor);
 assert(damage==QCE_MELEE_DAMAGE && flags==expectedFlags && mod==MOD_GAUNTLET);
 assert(dir[0]>0.99f && fabsf(point[0]-targetDistance)<0.01f);
}
gentity_t *G_TempEntity(vec3_t origin,int event) {
 (void)origin;assert(event==EV_MISSILE_MISS);impactCalls++;memset(&impact,0,sizeof(impact));return &impact;
}
int main(void) {
 gentity_t *attacker=&g_entities[0],*target=&g_entities[1];
 attacker->client=&clients[0];attacker->health=100;attacker->s.number=0;
 attacker->client->ps.stats[STAT_QCE_COMBAT]=1;
 attacker->client->ps.origin[2]=24;attacker->client->ps.viewheight=26;
 attacker->client->ps.pm_type=PM_NORMAL;
 target->client=&clients[1];target->health=100;target->takedamage=qtrue;target->s.number=1;
 assert(G_QceMelee(attacker) && damageCalls==1 && impactCalls==1);
 assert(impact.s.weapon==WP_GAUNTLET && impact.s.otherEntityNum==1);
 wallDistance=20;assert(!G_QceMelee(attacker) && damageCalls==1 && impactCalls==2);
 wallDistance=1000;targetDistance=80;assert(!G_QceMelee(attacker) && damageCalls==1 && impactCalls==2);
 targetDistance=QCE_MELEE_REACH;assert(G_QceMelee(attacker) && damageCalls==2);
 targetDistance=40;invalidTrace=1;assert(!G_QceMelee(attacker));
 invalidTrace=2;assert(!G_QceMelee(attacker));invalidTrace=0;
 target->takedamage=qfalse;assert(!G_QceMelee(attacker));target->takedamage=qtrue;
 target->health=0;assert(!G_QceMelee(attacker));target->health=100;
 attacker->client->ps.viewangles[YAW]=90;assert(!G_QceMelee(attacker));
 assert(damageCalls==2);

 attacker->client->ps.viewangles[YAW]=0;VectorSet(target->client->ps.origin,40,0,24);
 expectedFlags=DAMAGE_QCE_BACKSMACK;assert(G_QceMelee(attacker));
 target->client->ps.viewangles[YAW]=180;expectedFlags=0;assert(G_QceMelee(attacker));
 traceCalls=0;attacker->client->ps.stats[STAT_QCE_COMBAT]=0;assert(!G_QceMelee(attacker));
 attacker->client->ps.stats[STAT_QCE_COMBAT]=1;attacker->health=0;assert(!G_QceMelee(attacker));
 attacker->health=100;attacker->client->noclip=qtrue;assert(!G_QceMelee(attacker));
 attacker->client->noclip=qfalse;attacker->client->ps.pm_type=PM_SPECTATOR;assert(!G_QceMelee(attacker));
 assert(!G_QceMelee(NULL) && traceCalls==0);
 puts("PASS: server melee damage dispatch, eye trace/reach boundary, walls, misses, invalid hits, impact event backsmack orientation and eligibility guards");
 return 0;
}
