#include "g_local.h"
#include "bg_qce_presentation.generated.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
level_locals_t level;
static gentity_t grenade;
static int releases;
static vec3_t launch;
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int n,const char *fmt,...) {(void)n;(void)fmt;abort();}
gentity_t *fire_grenade(gentity_t *owner,vec3_t start,vec3_t dir) {
 (void)owner;(void)dir;memset(&grenade,0,sizeof(grenade));VectorCopy(start,launch);releases++;return &grenade;
}
void trap_Trace(trace_t *r,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 (void)start;(void)mins;(void)maxs;(void)pass;(void)mask;memset(r,0,sizeof(*r));r->fraction=1;VectorCopy(end,r->endpos);
}
int main(void) {
 gentity_t player={0};gclient_t client={0};player.client=&client;player.health=100;client.ps.stats[STAT_QCE_COMBAT]=1;client.ps.viewheight=52;
 level.time=100;G_QceBeginGrenadeThrow(&player,0);assert(client.qceGrenadeReleaseTime==367);
 G_QceBeginGrenadeThrow(&player,1);assert(client.qceGrenadeReleaseType==0);
 level.time=366;G_QceUpdateGrenadeRelease(&player);assert(releases==0);
 /* Release uses the current aim/origin, not the input-start transform. */
 VectorSet(client.ps.origin,10,20,30);client.ps.viewangles[YAW]=90;
 level.time=367;G_QceUpdateGrenadeRelease(&player);assert(releases==1 && !client.qceGrenadeReleaseTime);
 assert(fabs(launch[0]-6)<0.01 && fabs(launch[1]-20)<0.01 && fabs(launch[2]-82)<0.01);
 assert(grenade.s.pos.trDelta[1]==800 && grenade.qceGrenadeType==1 && !grenade.qceFuseArmed);
 G_QceUpdateGrenadeRelease(&player);assert(releases==1);
 level.time=500;G_QceBeginGrenadeThrow(&player,1);level.time=550;player.health=0;G_QceUpdateGrenadeRelease(&player);
 assert(releases==2 && grenade.qceGrenadeType==2 && grenade.s.pos.trDelta[1]>48 && grenade.s.pos.trDelta[1]<300);
 player.health=100;level.time=900;G_QceBeginGrenadeThrow(&player,0);client.sess.sessionTeam=TEAM_SPECTATOR;
 level.time=1200;G_QceUpdateGrenadeRelease(&player);assert(releases==2 && !client.qceGrenadeReleaseTime);
 puts("PASS: animation-keyframe grenade release, no duplicate throws, current camera/aim and lateral offset, premature death release and spectator cancellation");
 return 0;
}
