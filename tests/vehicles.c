#include "g_local.h"
#include "bg_qce_vehicle.h"
#include <assert.h>
#include <stdio.h>
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
gclient_t clients[2];
vmCvar_t g_cheats,g_gravity;
static int blocked,linked;
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 memset(tr,0,sizeof(*tr));tr->fraction=1;tr->entityNum=ENTITYNUM_NONE;VectorCopy(end,tr->endpos);tr->startsolid=blocked;
}
void trap_LinkEntity(gentity_t *e){linked++;}
void trap_SendServerCommand(int n,const char *msg){}
int main(void){
 gentity_t *p=&g_entities[0],*v=&g_entities[MAX_CLIENTS];int i;float s=0,s2=0;
 for(i=0;i<300;i++)s=QCE_HogSpeed(s,1,1.0f/30);assert(fabs(s-QCE_HOG_FORWARD)<.01f);
 for(i=0;i<600;i++)s2=QCE_HogSpeed(s2,1,1.0f/60);assert(fabs(s-s2)<.01f);
 for(i=0;i<300;i++)s=QCE_HogSpeed(s,-1,1.0f/30);assert(fabs(s+QCE_HOG_REVERSE)<.01f);
 for(i=0;i<100;i++)s=QCE_HogSpeed(s,0,1.0f/30);assert(s==0);
 {vec3_t forward={1,0,0},normal={-0.4472136f,0,0.8944272f},velocity;
  QCE_HogGroundVelocity(forward,100,normal,0,velocity);
  assert(velocity[2]>39 && fabs(DotProduct(velocity,normal))<.001f); /* climb, do not erase slope velocity */
  normal[0]=-normal[0];QCE_HogGroundVelocity(forward,100,normal,0,velocity);assert(velocity[2]< -39);
  VectorSet(normal,0,0,1);QCE_HogGroundVelocity(forward,100,normal,0,velocity);assert(velocity[0]==100 && velocity[2]==0);
 }
 level.num_entities=MAX_CLIENTS+1;level.maxclients=2;p->client=&clients[0];p->inuse=1;p->health=100;p->client->ps.stats[STAT_HEALTH]=100;
 v->s.number=MAX_CLIENTS;v->inuse=1;v->qceVehicle=1;v->health=500;
 assert(G_QceVehicleUse(p));assert(p->client->ps.qceVehicle==MAX_CLIENTS);assert(v->qceRiders[0]==1);assert(p->r.contents==CONTENTS_BODY);assert(p->client->ps.pm_type==PM_FREEZE);
 blocked=1;G_QceVehicleRelease(p,qfalse);assert(p->client->ps.qceVehicle==MAX_CLIENTS);
 blocked=0;G_QceVehicleRelease(p,qfalse);assert(!p->client->ps.qceVehicle);assert(!v->qceRiders[0]);assert(p->r.contents==CONTENTS_BODY);
 v->qceRiders[0]=2;assert(G_QceVehicleUse(p));assert(p->client->ps.qceVehicleSeat==1);
 blocked=1;G_QceVehicleRelease(p,qtrue);assert(!p->client->ps.qceVehicle);assert(!v->qceRiders[1]);
 blocked=0;v->qceRiders[0]=v->qceRiders[1]=v->qceRiders[2]=2;assert(!G_QceVehicleUse(p));
 v->qceRiders[0]=v->qceRiders[1]=v->qceRiders[2]=0;p->health=0;assert(!G_QceVehicleUse(p));
 p->health=100;p->client->sess.sessionTeam=TEAM_SPECTATOR;assert(!G_QceVehicleUse(p));
 p->client->ps.qceVehicle=MAX_CLIENTS;p->client->ps.qceVehicleSeat=0;v->qceRiders[0]=2;G_QceVehicleRelease(p,qtrue);assert(!p->client->ps.qceVehicle && v->qceRiders[0]==2);
 assert(linked>0);puts("PASS: Warthog speed/braking frame independence, seat ownership, blocked exit, forced release, full/dead/spectator entry");return 0;
}
