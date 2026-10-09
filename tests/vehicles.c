#include "g_local.h"
#include "bg_qce_vehicle.h"
#include "bg_qce_vehicle_impulse.h"
#include <assert.h>
#include <stdio.h>
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
gclient_t clients[2];
vmCvar_t g_cheats,g_gravity;float qceVariantValues[GV_COUNT];
void PM_UpdateViewAngles(playerState_t *ps,const usercmd_t *cmd){int i;for(i=0;i<3;i++)ps->viewangles[i]=SHORT2ANGLE(cmd->angles[i]+ps->delta_angles[i]);}
static float rampSlope,crestX=1000000;
static int blocked,linked,shots,riderCollision;static vec3_t lastMuzzle,lastEnd;static gentity_t eventEntity;
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 memset(tr,0,sizeof(*tr));tr->fraction=1;tr->entityNum=ENTITYNUM_NONE;VectorCopy(end,tr->endpos);tr->startsolid=blocked;
 if(riderCollision && (mask&CONTENTS_BODY)){tr->startsolid=1;tr->fraction=0;}
 if(!blocked && !mins && mask==MASK_SOLID && start[0]<crestX && end[0]<crestX){float a=start[2]-rampSlope*start[0],b=end[2]-rampSlope*end[0];
  if(a>0 && b<=0){vec3_t delta;tr->fraction=a/(a-b);VectorSubtract(end,start,delta);VectorMA(start,tr->fraction,delta,tr->endpos);VectorSet(tr->plane.normal,-rampSlope,0,1);VectorNormalize(tr->plane.normal);}}
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
 assert(QCE_HogSteer(0,30)==1 && QCE_HogSteer(0,-30)==-1 && QCE_HogSteer(179,-179)>0);
 assert(QCE_HogThrottle(127,0,0)==1 && QCE_HogThrottle(-127,0,0)==-1);
 assert(QCE_HogThrottle(127,-127,0)==1 && QCE_HogThrottle(127,0,127)==0);
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
  {vec3_t hull={10,179,12},view={-10,-170,0},aim,forward;
   QCE_HogTurretAim(hull,view,aim);QCE_HogTurretTransform(base,hull,aim[YAW],aim[PITCH],gun,muzzle,axis);
   AngleVectors(view,forward,NULL,NULL);assert(DotProduct(forward,axis[0])>.999f);
  }
  QCE_HogTurretTransform(base,angles,90,-35,gun,muzzle,axis);assert(axis[0][1]>.8f && axis[0][2]>.5f && fabs(axis[0][0])<.001f);
 }
 memset(clients,0,sizeof(clients));memset(v,0,sizeof(*v));v->s.number=MAX_CLIENTS;v->inuse=1;v->s.origin[2]=2;SP_qce_warthog(v);
 p->health=100;p->client->sess.sessionTeam=TEAM_FREE;VectorSet(p->client->ps.origin,-110,0,30);assert(G_QceVehicleUse(p));assert(p->client->ps.qceVehicleSeat==2);
 qceVariantValues[GV_GRAVITY]=1;
 {usercmd_t cmd={0};cmd.buttons=BUTTON_ATTACK;cmd.angles[PITCH]=ANGLE2SHORT(-60);cmd.angles[YAW]=ANGLE2SHORT(90);G_QceVehicleInput(p,&cmd);}
 riderCollision=1; /* Standing rider bounds must not block muzzle clearance. */
 for(i=0;i<20;i++){level.time+=50;v->think(v);}assert(shots==0); /* cannot fire during entry */
 for(i=0;i<40;i++){level.time+=50;v->think(v);}assert(shots>10 && v->qceTurretSpin>.99f);assert(v->qceTurretAngles[PITCH]==-35 && fabs(v->qceTurretAngles[YAW]-90)<2);
 {int before=shots;
  for(i=0;i<20;i++){level.time+=50;v->think(v);}assert(shots-before>=14 && shots-before<=16);
  before=shots;for(i=0;i<50;i++){level.time+=20;v->think(v);}assert(shots-before>=14 && shots-before<=16);
 }
 assert(lastMuzzle[2]>70 && lastEnd[1]>lastMuzzle[1]);
 riderCollision=0;
 {int previous=shots;G_QceVehicleRelease(p,qfalse);assert(p->client->ps.qceVehicleSeat==5 && v->qceRiders[2]);
  for(i=0;i<10;i++){level.time+=50;v->think(v);}assert(shots==previous);assert(v->qceRiders[2]);
  blocked=1;level.time+=400;G_QceVehicleRelease(p,qfalse);assert(v->qceRiders[2] && p->client->ps.qceVehicleSeat==2 && !p->client->qceVehicleExitTime);
  blocked=0;G_QceVehicleRelease(p,qfalse);assert(p->client->ps.qceVehicleSeat==5);G_QceVehicleRelease(p,qtrue);assert(!v->qceRiders[2]);
  for(i=0;i<80;i++){level.time+=50;v->think(v);}assert(v->qceTurretSpin<.001f);
 }
 riderCollision=0;
 /* Airborne with zero angular velocity must preserve orientation; the rotation
    helper requires a nonzero axis. Test nonzero world position and yaw too. */
 memset(v,0,sizeof(*v));v->s.number=MAX_CLIENTS;v->inuse=1;VectorSet(v->s.origin,2300,-7200,200);v->s.angles[YAW]=-48.6f;SP_qce_warthog(v);
 for(i=0;i<10;i++){level.time+=50;v->think(v);}assert(fabs(v->r.currentOrigin[0]-2300)<.1f && fabs(v->r.currentOrigin[1]+7200)<.1f);
 assert(v->r.currentOrigin[2]>160 && v->r.currentOrigin[2]<180 && fabs(v->r.currentAngles[YAW]-311.4f)<.01f);
 /* Source blast torque changes the physical hull orientation, not just visuals. */
 {vec3_t direction={0,1,0};float yaw=v->r.currentAngles[YAW];
  QCE_HogBlastImpulse(direction,96,v->qceVehicleVelocity,v->qceVehicleAngularVelocity);
  for(i=0;i<4;i++){level.time+=50;v->think(v);}
  assert(fabs(AngleSubtract(v->r.currentAngles[ROLL],0))+fabs(AngleSubtract(v->r.currentAngles[PITCH],0))>20);
  assert(fabs(AngleSubtract(v->r.currentAngles[YAW],yaw))<180 && v->qceVehicleVelocity[1]>50);
 }
 /* A seam may stop a wide box, but cannot stop retail mass-point sweeps.
    A force-supported chassis settles, then leaves an uphill crest ballistically. */
 memset(v,0,sizeof(*v));v->s.number=MAX_CLIENTS;v->inuse=1;v->s.origin[2]=3;SP_qce_warthog(v);
 for(i=0;i<600;i++){level.time+=8;v->think(v);}assert(fabs(v->qceVehicleVelocity[2])<1 && v->r.currentOrigin[2]>-5 && v->r.currentOrigin[2]<15);
 p->client->ps.qceVehicle=0;VectorSet(p->client->ps.origin,0,60,30);assert(G_QceVehicleUse(p));assert(p->client->ps.qceVehicleSeat==0);
 {usercmd_t cmd={0};cmd.forwardmove=127;cmd.rightmove=-127;G_QceVehicleInput(p,&cmd);} /* A no longer brakes. */
 for(i=0;i<1200;i++){level.time+=8;v->think(v);}assert(v->r.currentOrigin[0]>1000 && v->qceVehicleVelocity[0]>300);
 G_QceVehicleRelease(p,qtrue);rampSlope=.15f;crestX=200;VectorSet(v->r.currentOrigin,-200,0,-27);VectorSet(v->r.currentAngles,-RAD2DEG(atan(rampSlope)),0,0);VectorSet(v->qceVehicleVelocity,550,0,82.5f);VectorClear(v->qceVehicleAngularVelocity);v->qceVehicleSpeed=550;
 {int airborne=0;float z=0,vz=0;
  for(i=0;i<300;i++){level.time+=8;v->think(v);if(v->r.currentOrigin[0]>crestX+100 && !v->s.angles2[2]){airborne=1;z=v->r.currentOrigin[2];vz=v->qceVehicleVelocity[2];break;}}
  assert(airborne);for(i=0;i<10;i++){level.time+=8;v->think(v);}assert(v->qceVehicleVelocity[2]<vz-18 && v->qceVehicleVelocity[2]>vz-21 && v->qceVehicleVelocity[0]>100);assert(v->r.currentOrigin[2]>z-100);
 }
 assert(linked>0);puts("PASS: Warthog speed/braking frame independence, seat ownership, blocked exit, forced release, full/dead/spectator entry, timed exits, turret control/spin/aim, force-supported ground motion and ballistic crest departure");return 0;
}
