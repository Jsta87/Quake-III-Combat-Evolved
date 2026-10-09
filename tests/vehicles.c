#include "g_local.h"
#include "bg_qce_vehicle.h"
#include <assert.h>
#include <stdio.h>
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
gclient_t clients[2];
vmCvar_t g_cheats,g_gravity;
static int blocked,linked,shots;static vec3_t lastMuzzle,lastEnd;static gentity_t eventEntity;
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 memset(tr,0,sizeof(*tr));tr->fraction=1;tr->entityNum=ENTITYNUM_NONE;VectorCopy(end,tr->endpos);tr->startsolid=blocked;
 if(!blocked && !mins && mask==MASK_SOLID && start[2]>0 && end[2]<0){tr->fraction=start[2]/(start[2]-end[2]);{vec3_t delta;VectorSubtract(end,start,delta);VectorMA(start,tr->fraction,delta,tr->endpos);}VectorSet(tr->plane.normal,0,0,1);}
}
void SetClientViewAngle(gentity_t *p,vec3_t a){VectorCopy(a,p->client->ps.viewangles);}
void trap_LinkEntity(gentity_t *e){linked++;}
void trap_SendServerCommand(int n,const char *msg){}
int G_ModelIndex(char *name){return 1;}
gentity_t *G_TempEntity(vec3_t origin,int event){return &eventEntity;}
void G_Damage(gentity_t *t,gentity_t *i,gentity_t *a,vec3_t d,vec3_t p,int damage,int flags,int mod){}
void BG_PlayerStateToEntityState(playerState_t *ps,entityState_t *es,qboolean snap){VectorCopy(ps->origin,es->pos.trBase);}
gentity_t *G_QceFireBullet(gentity_t *owner,vec3_t start,vec3_t end,int weapon,int mod,int quad){assert(weapon==QCE_HOG_TURRET_WEAPON);shots++;VectorCopy(start,lastMuzzle);VectorCopy(end,lastEnd);return &eventEntity;}
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
 {vec3_t axis[3]={{1,0,0},{0,1,0},{0,0,1}},normal;float height,h[4]={-5.04f,5.36f,-5.04f,5.36f};
  assert(QCE_HogSteer(0,30)==1 && QCE_HogSteer(0,-30)==-1 && QCE_HogSteer(179,-179)>0);
  assert(QCE_HogThrottle(127,0,0)==1 && QCE_HogThrottle(-127,0,0)==-1);
  assert(QCE_HogThrottle(127,-127,0)==0 && QCE_HogThrottle(127,0,127)==0);
  QCE_HogContactPlane(h,axis,normal,&height);assert(fabs(normal[0]+.1f)<.001f && fabs(height)<.001f);
  h[0]=h[1]=-6.08f;h[2]=h[3]=6.08f;QCE_HogContactPlane(h,axis,normal,&height);assert(fabs(normal[1]+.2f)<.001f);
 }
 level.num_entities=MAX_CLIENTS+1;level.maxclients=2;p->client=&clients[0];p->inuse=1;p->health=100;p->client->ps.stats[STAT_HEALTH]=100;
 v->s.number=MAX_CLIENTS;v->inuse=1;v->qceVehicle=1;v->health=500;
 assert(G_QceVehicleUse(p));assert(p->client->ps.qceVehicle==MAX_CLIENTS);assert(v->qceRiders[0]==1);assert(p->r.contents==CONTENTS_BODY);assert(p->client->ps.pm_type==PM_FREEZE);
 blocked=1;G_QceVehicleRelease(p,qfalse);assert(p->client->ps.qceVehicle==MAX_CLIENTS);
 blocked=0;G_QceVehicleRelease(p,qfalse);assert(p->client->ps.qceVehicleSeat==3 && v->qceRiders[0]==1);
 G_QceVehicleRelease(p,qfalse);assert(p->client->ps.qceVehicle);level.time=QCE_HogExitMS(0);G_QceVehicleRelease(p,qfalse);assert(!p->client->ps.qceVehicle);assert(!v->qceRiders[0]);assert(p->r.contents==CONTENTS_BODY);
 VectorSet(p->client->ps.origin,0,-60,30);v->qceRiders[0]=2;assert(G_QceVehicleUse(p));assert(p->client->ps.qceVehicleSeat==1);
 blocked=1;G_QceVehicleRelease(p,qtrue);assert(!p->client->ps.qceVehicle);assert(!v->qceRiders[1]);
 blocked=0;VectorSet(p->client->ps.origin,-110,0,30);assert(G_QceVehicleUse(p));assert(p->client->ps.qceVehicleSeat==2);G_QceVehicleRelease(p,qtrue);
 blocked=0;v->qceRiders[0]=v->qceRiders[1]=v->qceRiders[2]=2;assert(!G_QceVehicleUse(p));
 v->qceRiders[0]=v->qceRiders[1]=v->qceRiders[2]=0;p->health=0;assert(!G_QceVehicleUse(p));
 p->health=100;p->client->sess.sessionTeam=TEAM_SPECTATOR;assert(!G_QceVehicleUse(p));
 p->client->ps.qceVehicle=MAX_CLIENTS;p->client->ps.qceVehicleSeat=0;v->qceRiders[0]=2;G_QceVehicleRelease(p,qtrue);assert(!p->client->ps.qceVehicle && v->qceRiders[0]==2);
 {
  vec3_t base={0,0,0},angles={0,0,0},gun,muzzle,axis[3];float ramp=0;
  for(i=0;i<30;i++)ramp=QCE_HogRamp(ramp,qtrue,1,4,1.0f/30);assert(fabs(ramp-1)<.001f);
  for(i=0;i<120;i++)ramp=QCE_HogRamp(ramp,qfalse,1,4,1.0f/30);assert(ramp<.001f);
  assert(fabs(QCE_HogSuspensionAngle(0,-3.209f))<.01f);assert(fabs(QCE_HogSuspensionAngle(1,-2.815f))<.01f);
  assert(QCE_HogSuspensionAngle(0,-100)<-29 && QCE_HogSuspensionAngle(1,-100)>29);
  QCE_HogTurretTransform(base,angles,90,-35,gun,muzzle,axis);assert(axis[0][1]>.8f && axis[0][2]>.5f && fabs(axis[0][0])<.001f);
 }
 memset(clients,0,sizeof(clients));memset(v,0,sizeof(*v));v->s.number=MAX_CLIENTS;v->inuse=1;v->s.origin[2]=2;SP_qce_warthog(v);
 p->health=100;p->client->sess.sessionTeam=TEAM_FREE;VectorSet(p->client->ps.origin,-110,0,30);assert(G_QceVehicleUse(p));assert(p->client->ps.qceVehicleSeat==2);
 p->client->pers.cmd.buttons=BUTTON_ATTACK;VectorSet(p->client->ps.viewangles,-60,90,0);
 for(i=0;i<20;i++){level.time+=50;v->think(v);}assert(shots==0); /* cannot fire during entry */
 for(i=0;i<40;i++){level.time+=50;v->think(v);}assert(shots>10 && v->qceTurretSpin>.99f);assert(v->qceTurretAngles[PITCH]==-35 && fabs(v->qceTurretAngles[YAW]-90)<.01f);
 {int before=shots;
  for(i=0;i<20;i++){level.time+=50;v->think(v);}assert(shots-before>=14 && shots-before<=16);
  before=shots;for(i=0;i<50;i++){level.time+=20;v->think(v);}assert(shots-before>=14 && shots-before<=16);
 }
 assert(lastMuzzle[2]>70 && lastEnd[1]>lastMuzzle[1]);
 {int previous=shots;G_QceVehicleRelease(p,qfalse);assert(p->client->ps.qceVehicleSeat==5 && v->qceRiders[2]);
  for(i=0;i<10;i++){level.time+=50;v->think(v);}assert(shots==previous);assert(v->qceRiders[2]);
  blocked=1;level.time+=400;G_QceVehicleRelease(p,qfalse);assert(v->qceRiders[2] && p->client->ps.qceVehicleSeat==2 && !p->client->qceVehicleExitTime);
  blocked=0;G_QceVehicleRelease(p,qfalse);assert(p->client->ps.qceVehicleSeat==5);G_QceVehicleRelease(p,qtrue);assert(!v->qceRiders[2]);
  for(i=0;i<80;i++){level.time+=50;v->think(v);}assert(v->qceTurretSpin<.001f);
 }
 assert(linked>0);puts("PASS: Warthog speed/braking frame independence, seat ownership, blocked exit, forced release, full/dead/spectator entry, timed exits, turret control/spin/aim and suspension bounds");return 0;
}
