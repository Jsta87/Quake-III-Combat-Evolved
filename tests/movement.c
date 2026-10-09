#include "q_shared.h"
#include "bg_public.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
void QDECL Com_Printf(const char *fmt, ...) { (void)fmt; }
void QDECL Com_Error(int level, const char *fmt, ...) { (void)level; (void)fmt; abort(); }
void trap_SnapVector(float *v) { int i; for(i=0;i<3;i++)v[i]=roundf(v[i]); }
static float meleeTarget=10000,meleeWall=10000;
static int contents(const vec3_t p, int pass) { (void)p; (void)pass; return 0; }
static void trace(trace_t *t, const vec3_t start, const vec3_t mins,
 const vec3_t maxs, const vec3_t end, int pass, int mask) {
 float a=start[2]+(mins?mins[2]:0), b=end[2]+(mins?mins[2]:0);
 (void)maxs; (void)pass; (void)mask;
 memset(t,0,sizeof(*t));t->fraction=1;VectorCopy(end,t->endpos);
 t->entityNum=ENTITYNUM_NONE;
 if(!mins && mask==MASK_SHOT && end[0]>start[0]) {
  float nearest=meleeTarget<meleeWall?meleeTarget:meleeWall;
  if(nearest>=start[0] && nearest<=end[0]) {
   t->fraction=(nearest-start[0])/(end[0]-start[0]);
   {int i;for(i=0;i<3;i++)t->endpos[i]=start[i]+t->fraction*(end[i]-start[i]);}
   t->entityNum=meleeWall<meleeTarget?ENTITYNUM_WORLD:1;
   t->contents=meleeWall<meleeTarget?CONTENTS_SOLID:CONTENTS_BODY;
  }
  return;
 }

 if (b < 0 && b < a) {
  t->fraction=a/(a-b);if(t->fraction<0)t->fraction=0;
  { int i; for(i=0;i<3;i++) t->endpos[i]=start[i]+t->fraction*(end[i]-start[i]); }
  t->plane.normal[2]=1;t->entityNum=ENTITYNUM_WORLD;
 }
}
static void init(playerState_t *ps, int profile) {
 memset(ps,0,sizeof(*ps));ps->pm_type=PM_NORMAL;ps->origin[2]=24;
 ps->gravity=800;ps->speed=200;ps->stats[STAT_HEALTH]=100;
 ps->stats[STAT_QCE_MOVEMENT]=profile;ps->weapon=WP_MACHINEGUN;
 ps->stats[STAT_WEAPONS]=1<<WP_MACHINEGUN;ps->ammo[WP_MACHINEGUN]=100;
}
static void step(playerState_t *ps, int forward, int side, int up, int yaw) {
 pmove_t pm; memset(&pm,0,sizeof(pm));pm.ps=ps;pm.trace=trace;
 pm.pointcontents=contents;pm.tracemask=MASK_PLAYERSOLID;
 pm.cmd.serverTime=ps->commandTime+8;pm.cmd.forwardmove=forward;
 pm.cmd.rightmove=side;pm.cmd.upmove=up;pm.cmd.angles[YAW]=ANGLE2SHORT(yaw);
 pm.cmd.weapon=WP_MACHINEGUN;Pmove(&pm);
}
#ifndef QCE_WEAPON_TEST
int main(void) {
 playerState_t server,client;int i;float speed,apex=0;
 init(&server,1);init(&client,1);
 for(i=0;i<250;i++) {step(&server,127,0,0,0);step(&client,127,0,0,0);}
 assert(!memcmp(&server,&client,sizeof(server)));
 speed=sqrtf(server.velocity[0]*server.velocity[0]+server.velocity[1]*server.velocity[1]);
 assert(speed>179 && speed<181);
 for(i=0;i<250;i++) step(&server,127,127,0,i%360);
 speed=sqrtf(server.velocity[0]*server.velocity[0]+server.velocity[1]*server.velocity[1]);assert(speed<=181.0f);
 init(&server,1);for(i=0;i<250;i++)step(&server,127,0,-127,0);
 assert(server.velocity[0]>71 && server.velocity[0]<73);
 init(&server,1);step(&server,0,0,127,0);assert(server.velocity[2]>164 && server.velocity[2]<168);
 for(i=0;i<200;i++){step(&server,0,0,127,0);if(server.origin[2]>apex)apex=server.origin[2];}
 fprintf(stderr,"Measured apex: %f\n",apex);assert(apex>77 && apex<80);assert(server.groundEntityNum==ENTITYNUM_WORLD);
 init(&server,1);init(&client,0);server.origin[2]=client.origin[2]=1000;
 step(&server,127,0,0,0);step(&client,127,0,0,0);
 assert(server.velocity[0]>0 && server.velocity[0]<client.velocity[0]);
 assert(server.velocity[2]>client.velocity[2]);
 init(&server,1);server.qceVariantScale[0]=501;for(i=0;i<250;i++)step(&server,127,0,0,0);
 assert(server.velocity[0]>89 && server.velocity[0]<91);
 server.qceVariantScale[0]=1;step(&server,127,0,0,0);assert(server.velocity[0]==0 && server.velocity[1]==0);
 init(&server,1);server.qceVariantScale[2]=1;server.origin[2]=1000;server.velocity[2]=50;for(i=0;i<50;i++)step(&server,0,0,0,0);assert(server.velocity[2]==50);
 init(&server,1);server.qceVariantScale[1]=2001;step(&server,0,0,127,0);assert(server.velocity[2]>230 && server.velocity[2]<240);
 init(&server,0);step(&server,0,0,127,0);assert(server.velocity[2]>260 && server.velocity[2]<270);
 printf("PASS: speed cap, turning, crouch speed, jump arc, held-jump landing, deterministic replay, reduced air control, stock jump fallback\n");
 return 0;
}

#endif
