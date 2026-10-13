#include "g_local.h"
#include <assert.h>
#include <stdio.h>
level_locals_t level;gentity_t g_entities[MAX_GENTITIES];static gclient_t clients[2];static int walls,noGround,notices;
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask){
 vec3_t delta;float bottom=mins?mins[2]:0;memset(tr,0,sizeof(*tr));tr->fraction=1;VectorCopy(end,tr->endpos);
 if(walls){tr->startsolid=tr->allsolid=1;return;}
 if(!noGround && start[2]+bottom>0 && end[2]+bottom<=0){tr->fraction=(start[2]+bottom)/(start[2]-end[2]);VectorSubtract(end,start,delta);VectorMA(start,tr->fraction,delta,tr->endpos);tr->plane.normal[2]=1;}
}
void trap_SendServerCommand(int client,const char *s){notices++;}
int main(void){
 gentity_t *ent=&g_entities[0],*other=&g_entities[1];vec3_t origin={0,0,33};
 level.maxclients=2;level.time=100;ent->client=&clients[0];ent->s.number=0;ent->inuse=1;
 other->client=&clients[1];other->inuse=1;other->health=100;other->r.linked=1;VectorCopy(origin,other->r.currentOrigin);
 assert(!G_QcePrepareSpawn(ent,origin));assert(clients[0].qceSpawnWaitUntil==1100 && notices==1);
 level.time=1099;other->r.linked=0;assert(!G_QcePrepareSpawn(ent,origin));
 level.time=1100;assert(G_QcePrepareSpawn(ent,origin));assert(clients[0].qceSpawnWaitUntil==0);
 other->r.linked=1;assert(!G_QcePrepareSpawn(ent,origin));level.time+=1000;
 assert(G_QcePrepareSpawn(ent,origin));assert(origin[0]*origin[0]+origin[1]*origin[1]>=64*64-1 && origin[2]==25);
 VectorSet(origin,0,0,33);VectorSet(other->r.currentOrigin,0,0,90);assert(!G_QcePrepareSpawn(ent,origin)); /* no spawning on heads */
 level.time+=1000;noGround=1;assert(!G_QcePrepareSpawn(ent,origin));assert(clients[0].qceSpawnWaitUntil);
 noGround=0;walls=1;assert(!G_QcePrepareSpawn(ent,origin));walls=0;assert(G_QcePrepareSpawn(ent,origin));
 puts("PASS: one-second spawn queue, early-space wait, safe ground offsets, horizontal separation, stacked-player rejection, world clearance and continued queue when no safe ground exists");return 0;
}
