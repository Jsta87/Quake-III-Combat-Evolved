/* SPDX-License-Identifier: GPL-3.0-only
 * Server-authoritative first Warthog prototype. No retail rigid-body simulation.
 */
#include "g_local.h"
#include "bg_qce_vehicle.h"
static const vec3_t seatOffsets[3]={{-1.904f,13.260f,31.260f},{-9.296f,-12.692f,46.243f},{-49.827f,0,60.445f}};
static gentity_t *Vehicle(gentity_t *player) {
 int n=player->client->ps.qceVehicle,seat=player->client->ps.qceVehicleSeat%3;
 if(n<=0 || n>=level.num_entities || seat<0 || seat>=3)return NULL;
 if(!g_entities[n].inuse || !g_entities[n].qceVehicle || g_entities[n].health<=0 || g_entities[n].qceRiders[seat]!=player->s.number+1)return NULL;
 return &g_entities[n];
}
static void SeatOrigin(gentity_t *v,int seat,vec3_t result) {
 vec3_t axis[3];int i;AnglesToAxis(v->r.currentAngles,axis);VectorCopy(v->r.currentOrigin,result);
 for(i=0;i<3;i++)VectorMA(result,seatOffsets[seat][i],axis[i],result);
}
void G_QceVehicleRelease(gentity_t *p,qboolean force) {
 gentity_t *v;trace_t tr;vec3_t dest,exitStart,mins={-15,-15,-24},maxs={15,15,32},axis[3];int seat,i,found=0;
 if(!p->client || !p->client->ps.qceVehicle)return;
 v=Vehicle(p);seat=p->client->ps.qceVehicleSeat%3;
 if(!force && p->client->qceVehicleExitTime && level.time<p->client->qceVehicleExitTime)return;
 if(v) {
  AnglesToAxis(v->r.currentAngles,axis);
  for(i=0;i<6;i++) {
   VectorCopy(v->r.currentOrigin,dest);dest[2]+=40;
   if(seat==2 && i<2)VectorMA(dest,i?-112:-170,axis[0],dest);
   else if(i<4)VectorMA(dest,(i&1?-1:1)*(seat==1?-1:1)*(i<2?112:170),axis[1],dest);
   else VectorMA(dest,(i&1?-1:1)*200,axis[0],dest);
   SeatOrigin(v,seat,exitStart);trap_Trace(&tr,exitStart,mins,maxs,dest,v->s.number,MASK_SOLID);
   if(tr.startsolid || tr.allsolid || tr.fraction<1)continue;
   trap_Trace(&tr,dest,mins,maxs,dest,p->s.number,MASK_PLAYERSOLID);
   if(!tr.startsolid && !tr.allsolid){found=1;break;}
  }
  if(!found && !force){
   p->client->qceVehicleExitTime=0;p->client->ps.qceVehicleSeat=seat;p->client->ps.qceVehicleTime=p->client->qceVehicleEnterTime;
   trap_SendServerCommand(p->s.number,"print \"Exit blocked. Move the Warthog to open ground.\n\"");return;
  }
  if(!force && !p->client->qceVehicleExitTime) {
   p->client->qceVehicleExitTime=level.time+QCE_HogExitMS(seat);
   p->client->ps.qceVehicleSeat=seat+3;p->client->ps.qceVehicleTime=level.time;return;
  }
  if(seat>=0 && seat<3 && v->qceRiders[seat]==p->s.number+1)v->qceRiders[seat]=0;
  if(found){VectorCopy(dest,p->client->ps.origin);VectorCopy(dest,p->r.currentOrigin);}
 }
 p->client->qceVehicleExitTime=0;p->client->ps.qceVehicle=p->client->ps.qceVehicleSeat=p->client->ps.qceVehicleTime=0;
 p->client->ps.pm_type=p->health>0?PM_NORMAL:PM_DEAD;
 VectorClear(p->client->ps.velocity);p->r.contents=CONTENTS_BODY;
 p->client->ps.eFlags^=EF_TELEPORT_BIT;trap_LinkEntity(p);
}
qboolean G_QceVehicleUse(gentity_t *p) {
 gentity_t *best=NULL,*v;float closest=150*150;int i,seat=-1;vec3_t delta;
 if(!p->client || p->health<=0 || p->client->sess.sessionTeam==TEAM_SPECTATOR)return qfalse;
 if(p->client->ps.qceVehicle){G_QceVehicleRelease(p,qfalse);p->client->qcePickupLatched=qtrue;return qtrue;}
 for(i=MAX_CLIENTS;i<level.num_entities;i++) {
  v=&g_entities[i];if(!v->inuse || !v->qceVehicle || v->health<=0)continue;
  VectorSubtract(v->r.currentOrigin,p->client->ps.origin,delta);
  if(VectorLengthSquared(delta)<closest && (!v->qceRiders[0]||!v->qceRiders[1]||!v->qceRiders[2])){closest=VectorLengthSquared(delta);best=v;}
 }
 if(!best)return qfalse;
 closest=150*150;
 for(i=0;i<3;i++)if(!best->qceRiders[i]) {
  vec3_t boarding,axis[3];AnglesToAxis(best->r.currentAngles,axis);VectorCopy(best->r.currentOrigin,boarding);
  if(i<2)VectorMA(boarding,i==0?48:-48,axis[1],boarding);else VectorMA(boarding,-95,axis[0],boarding);
  VectorSubtract(boarding,p->client->ps.origin,delta);
  if(VectorLengthSquared(delta)<closest){closest=VectorLengthSquared(delta);seat=i;}
 }
 if(seat<0)return qfalse;
 /* A wall between player and vehicle must block entry. */
 {trace_t tr;vec3_t target;SeatOrigin(best,seat,target);trap_Trace(&tr,p->client->ps.origin,NULL,NULL,target,p->s.number,MASK_SOLID);
  if(tr.startsolid || tr.allsolid || (tr.fraction<1 && tr.entityNum!=best->s.number))return qfalse;}
 if(seat==0){vec3_t aim;VectorCopy(p->client->ps.viewangles,aim);aim[YAW]=best->r.currentAngles[YAW];SetClientViewAngle(p,aim);}
 p->client->qceVehicleEnterTime=p->client->ps.qceVehicleTime=level.time;p->client->qceVehicleExitTime=0;best->qceRiders[seat]=p->s.number+1;p->client->ps.qceVehicle=best->s.number;p->client->ps.qceVehicleSeat=seat;
 p->client->ps.qceZoom=0;p->client->ps.pm_type=PM_FREEZE;p->client->ps.eFlags^=EF_TELEPORT_BIT;
 VectorClear(p->client->ps.velocity);SeatOrigin(best,seat,p->client->ps.origin);
 p->r.contents=CONTENTS_BODY;p->client->qcePickupLatched=qtrue;
 trap_SendServerCommand(p->s.number,va("print \"Warthog %s: E exits; %s.\n\"",seat==0?"driver":seat==1?"passenger":"gunner",seat==0?"mouse steers, W/S throttle, A or Space brakes":seat==1?"use your held weapon":"mouse aims, hold fire to spin up the turret"));
 return qtrue;
}
void G_QceVehicleInput(gentity_t *p,usercmd_t *cmd) {
 if(p->client->ps.qceVehicle) {
  if(!Vehicle(p) || p->client->noclip || p->health<=0)G_QceVehicleRelease(p,qtrue);
  else {
   p->client->ps.pm_type=PM_FREEZE;
   if(p->client->qceVehicleExitTime && level.time>=p->client->qceVehicleExitTime)G_QceVehicleRelease(p,qfalse);
  }
 }
 if((cmd->buttons&BUTTON_QCE_PICKUP) && !p->client->qcePickupLatched)G_QceVehicleUse(p);
}
static void VehicleDie(gentity_t *v,gentity_t *inflictor,gentity_t *attacker,int damage,int mod) {
 int i;gentity_t *p;vec3_t origin;VectorCopy(v->r.currentOrigin,origin);
 /* Release before health changes invalidate Vehicle(). G_Damage already subtracted it. */
 v->health=1;
 for(i=0;i<3;i++)if(v->qceRiders[i]){p=&g_entities[v->qceRiders[i]-1];G_QceVehicleRelease(p,qtrue);G_Damage(p,v,attacker,NULL,NULL,60,DAMAGE_NO_KNOCKBACK,MOD_ROCKET_SPLASH);}
 v->health=0;v->takedamage=qfalse;v->s.eFlags|=EF_NODRAW;v->r.contents=0;v->qceVehicleRespawn=level.time+15000;
 G_TempEntity(origin,EV_MISSILE_MISS)->s.weapon=WP_ROCKET_LAUNCHER;trap_LinkEntity(v);
}
static void TurretThink(gentity_t *v,float dt) {
 gentity_t *p=NULL,*bolt;vec3_t hull[3],view,localAim,gun,muzzle,gunAxis[3],dir,end;trace_t tr;
 qboolean firing=qfalse;float angle,radius,rate;int i,emitted=0;
 if(v->qceRiders[2]) {
  p=&g_entities[v->qceRiders[2]-1];
  if(p->inuse && p->client && p->health>0 && !p->client->qceVehicleExitTime) {
   AnglesToAxis(v->r.currentAngles,hull);AngleVectors(p->client->ps.viewangles,view,NULL,NULL);
   for(i=0;i<3;i++)localAim[i]=DotProduct(view,hull[i]);vectoangles(localAim,v->qceTurretAngles);
   v->qceTurretAngles[YAW]=AngleNormalize180(v->qceTurretAngles[YAW]);
   v->qceTurretAngles[PITCH]=Com_Clamp(-35,15,AngleNormalize180(v->qceTurretAngles[PITCH]));
   firing=(p->client->pers.cmd.buttons&BUTTON_ATTACK) && level.time-p->client->qceVehicleEnterTime>=1534;
  }
 }
 v->qceTurretSpin=QCE_HogRamp(v->qceTurretSpin,firing,1,4,dt);
 v->qceTurretError=QCE_HogRamp(v->qceTurretError,firing,3,1,dt);
 if(!firing){v->qceTurretNextShot=level.time;return;}
 rate=8+7*v->qceTurretSpin;
 if(level.time<v->qceTurretNextShot)return;
 QCE_HogTurretTransform(v->r.currentOrigin,v->r.currentAngles,v->qceTurretAngles[YAW],v->qceTurretAngles[PITCH],gun,muzzle,gunAxis);
 /* Do not spawn rounds beyond a wall or the vehicle body. */
 trap_Trace(&tr,gun,NULL,NULL,muzzle,p->s.number,MASK_SHOT);
 if(tr.startsolid || tr.allsolid || tr.fraction<1){v->qceTurretNextShot=level.time+1000/rate;return;}
 if(v->qceTurretNextShot<level.time-100)v->qceTurretNextShot=level.time-100;
 while(v->qceTurretNextShot<=level.time && emitted++<3) {
  v->qceTurretNextShot+=1000/rate;
 angle=random()*2*M_PI;radius=sqrt(random())*tan(DEG2RAD(1+v->qceTurretError));
 VectorCopy(gunAxis[0],dir);VectorMA(dir,cos(angle)*radius,gunAxis[1],dir);VectorMA(dir,sin(angle)*radius,gunAxis[2],dir);VectorNormalize(dir);
 VectorMA(muzzle,8000,dir,end);bolt=G_QceFireBullet(p,muzzle,end,QCE_HOG_TURRET_WEAPON,MOD_MACHINEGUN,1);
 bolt->s.generic1=(v->qceTurretShots%3)==0;
 p->client->accuracy_shots++;v->qceTurretLastShot=level.time;v->qceTurretShots++;
 }
}
static void VehicleThink(gentity_t *v) {
 float dt=Com_Clamp(0.001f,0.1f,(level.time-v->qceVehicleTime)*0.001f),throttle=0,steer=0;
 vec3_t forward,end,start,normal,flat,axis[3],hullMins={-61,-29,18},hullMaxs={61,29,65};trace_t tr;int i;gentity_t *p;float groundZ=-99999,heights[4];int contacts=0;
 v->qceVehicleTime=level.time;v->nextthink=level.time+1;
 if(v->health<=0) {
  if(level.time<v->qceVehicleRespawn)return;
  VectorCopy(v->pos1,v->r.currentOrigin);VectorCopy(v->pos2,v->r.currentAngles);VectorClear(v->qceVehicleVelocity);v->qceVehicleSpeed=0;
  v->qceTurretSpin=v->qceTurretError=v->qceVehicleRPM=0;v->qceTurretLastShot=v->qceTurretShots=0;VectorClear(v->qceTurretAngles);
  v->health=500;v->takedamage=qtrue;v->s.eFlags&=~EF_NODRAW;v->r.contents=CONTENTS_SOLID;
 }
 for(i=0;i<3;i++)if(v->qceRiders[i]) {
  p=&g_entities[v->qceRiders[i]-1];
  if(!p->inuse || !p->client || p->health<=0 || p->client->ps.qceVehicle!=v->s.number || p->client->ps.qceVehicleSeat%3!=i){v->qceRiders[i]=0;continue;}
  if(i==0 && !p->client->qceVehicleExitTime && level.time-p->client->qceVehicleEnterTime>=1667){throttle=QCE_HogThrottle(p->client->pers.cmd.forwardmove,p->client->pers.cmd.rightmove,p->client->pers.cmd.upmove);steer=QCE_HogSteer(v->r.currentAngles[YAW],p->client->ps.viewangles[YAW]);}
 }
 /* Four support probes align the hull to a gently filtered terrain normal. */
 VectorSet(flat,0,v->r.currentAngles[YAW],0);AnglesToAxis(flat,axis);VectorClear(normal);
 for(i=0;i<4;i++) {
  VectorCopy(v->r.currentOrigin,start);VectorMA(start,i&1?53.6f:-50.4f,axis[0],start);VectorMA(start,i&2?30.4f:-30.4f,axis[1],start);start[2]+=36;
  VectorCopy(start,end);end[2]-=100;
  trap_Trace(&tr,start,NULL,NULL,end,v->s.number,MASK_SOLID);
  if(tr.fraction<1 && !tr.startsolid && tr.plane.normal[2]>.25f){float height=tr.endpos[2]+(tr.plane.normal[0]*(tr.endpos[0]-v->r.currentOrigin[0])+tr.plane.normal[1]*(tr.endpos[1]-v->r.currentOrigin[1]))/tr.plane.normal[2];heights[i]=tr.endpos[2];contacts|=1<<i;VectorAdd(normal,tr.plane.normal,normal);if(height>groundZ)groundZ=height;}
 }
 if(contacts==15)QCE_HogContactPlane(heights,axis,normal,&groundZ);
 if(groundZ>-99999 && v->r.currentOrigin[2]-groundZ<45) {
  VectorNormalize(normal);v->qceVehicleSpeed=QCE_HogSpeed(v->qceVehicleSpeed,throttle,dt);
  v->r.currentAngles[YAW]+=steer*v->qceVehicleSpeed/120.0f*33.0797f*dt;
  AngleVectors(v->r.currentAngles,forward,NULL,NULL);
  v->r.currentAngles[PITCH]+=(RAD2DEG(atan2(DotProduct(normal,axis[0]),normal[2]))-v->r.currentAngles[PITCH])*Com_Clamp(0,1,dt*5);
  v->r.currentAngles[ROLL]+=(RAD2DEG(atan2(-DotProduct(normal,axis[1]),normal[2]))-v->r.currentAngles[ROLL])*Com_Clamp(0,1,dt*5);
  QCE_HogGroundVelocity(forward,v->qceVehicleSpeed,normal,groundZ+2-v->r.currentOrigin[2],v->qceVehicleVelocity);
 } else v->qceVehicleVelocity[2]-=g_gravity.value*dt;
 VectorMA(v->r.currentOrigin,dt,v->qceVehicleVelocity,end);
 trap_Trace(&tr,v->r.currentOrigin,hullMins,hullMaxs,end,v->s.number,MASK_SOLID);
 if(!tr.startsolid){VectorCopy(tr.endpos,v->r.currentOrigin);if(tr.fraction<1){float into=DotProduct(v->qceVehicleVelocity,tr.plane.normal);VectorMA(v->qceVehicleVelocity,-into,tr.plane.normal,v->qceVehicleVelocity);if(tr.plane.normal[2]<.5f)v->qceVehicleSpeed*=.35f;}}
 else {v->qceVehicleSpeed=0;VectorClear(v->qceVehicleVelocity);}
 TurretThink(v,dt);
 v->qceVehicleRPM+=(Com_Clamp(0,1,fabs(v->qceVehicleSpeed)/QCE_HOG_FORWARD+fabs(throttle)*.18f)-v->qceVehicleRPM)*Com_Clamp(0,1,dt*4);
 v->s.frame=(int)(v->qceVehicleRPM*255);v->s.time2=v->qceTurretLastShot;
 v->s.origin2[0]=v->qceTurretAngles[YAW];v->s.origin2[1]=v->qceTurretAngles[PITCH];v->s.origin2[2]=v->qceTurretSpin;
 v->s.pos.trType=TR_INTERPOLATE;v->s.pos.trTime=level.time;VectorCopy(v->r.currentOrigin,v->s.pos.trBase);VectorCopy(v->qceVehicleVelocity,v->s.pos.trDelta);
 v->s.clientNum=(v->qceRiders[0]?1:0)|(v->qceRiders[1]?2:0)|(v->qceRiders[2]?4:0);v->s.apos.trType=TR_INTERPOLATE;v->s.angles2[0]=steer*30;v->s.angles2[1]=v->qceVehicleSpeed;v->s.angles2[2]=contacts;VectorCopy(v->r.currentAngles,v->s.apos.trBase);trap_LinkEntity(v);
 for(i=0;i<3;i++)if(v->qceRiders[i]) {
  p=&g_entities[v->qceRiders[i]-1];SeatOrigin(v,i,p->client->ps.origin);VectorCopy(p->client->ps.origin,p->r.currentOrigin);VectorClear(p->client->ps.velocity);p->r.contents=CONTENTS_BODY;
  BG_PlayerStateToEntityState(&p->client->ps,&p->s,qtrue);trap_LinkEntity(p);
 }
}
void SP_qce_warthog(gentity_t *v) {
 v->classname="qce_warthog";v->qceVehicle=1;v->s.eType=ET_GENERAL;v->s.generic1=QCE_HOG_MARKER;
 v->s.modelindex=G_ModelIndex(QCE_HOG_MODEL);v->clipmask=MASK_SOLID;v->r.contents=CONTENTS_SOLID;
 VectorSet(v->r.mins,-28,-28,10);VectorSet(v->r.maxs,28,28,38);
 VectorCopy(v->s.origin,v->r.currentOrigin);VectorCopy(v->s.angles,v->r.currentAngles);
 VectorCopy(v->r.currentOrigin,v->pos1);VectorCopy(v->r.currentAngles,v->pos2);
 v->health=500;v->takedamage=qtrue;v->die=VehicleDie;v->think=VehicleThink;v->qceVehicleTime=level.time;v->nextthink=level.time+1;
 VectorCopy(v->r.currentOrigin,v->s.pos.trBase);VectorCopy(v->r.currentAngles,v->s.apos.trBase);trap_LinkEntity(v);
}
void G_QceVehicleCommand(gentity_t *p,const char *cmd) {
 if(!Q_stricmp(cmd,"qce_vehicle_exit")){G_QceVehicleRelease(p,qfalse);return;}
 if(!Q_stricmp(cmd,"qce_vehicle_enter")){G_QceVehicleUse(p);return;}
 if(!Q_stricmp(cmd,"qce_vehicle_status")) {
  gentity_t *v=Vehicle(p);trap_SendServerCommand(p->s.number,va("print \"Warthog entity=%d seat=%d speed=%.1f health=%d pitch=%.1f roll=%.1f turret=%d yaw=%.1f pitch=%.1f rpm=%.2f exit=%d\n\"",p->client->ps.qceVehicle,p->client->ps.qceVehicleSeat,v?v->qceVehicleSpeed:0,v?v->health:0,v?v->r.currentAngles[PITCH]:0,v?v->r.currentAngles[ROLL]:0,v?v->qceTurretShots:0,v?v->qceTurretAngles[YAW]:0,v?v->qceTurretAngles[PITCH]:0,v?v->qceVehicleRPM:0,p->client->qceVehicleExitTime));
  if(!v){int i;trap_SendServerCommand(p->s.number,va("print \"Player at %.1f %.1f %.1f\n\"",p->client->ps.origin[0],p->client->ps.origin[1],p->client->ps.origin[2]));for(i=MAX_CLIENTS;i<level.num_entities;i++)if(g_entities[i].inuse && g_entities[i].qceVehicle)trap_SendServerCommand(p->s.number,va("print \"Warthog %d at %.1f %.1f %.1f health=%d seats=%d,%d,%d\n\"",i,g_entities[i].r.currentOrigin[0],g_entities[i].r.currentOrigin[1],g_entities[i].r.currentOrigin[2],g_entities[i].health,g_entities[i].qceRiders[0],g_entities[i].qceRiders[1],g_entities[i].qceRiders[2]));}
  return;
 }
 if(!g_cheats.integer){trap_SendServerCommand(p->s.number,"print \"Vehicle spawning requires sv_cheats.\n\"");return;}
 if(p->health>0 && p->client->sess.sessionTeam!=TEAM_SPECTATOR) {
  gentity_t *v;vec3_t f,end,mins={-76,-49,20},maxs={76,49,66};trace_t tr;
  AngleVectors(p->client->ps.viewangles,f,NULL,NULL);f[2]=0;VectorNormalize(f);VectorMA(p->client->ps.origin,190,f,end);end[2]+=24;
  trap_Trace(&tr,end,mins,maxs,end,p->s.number,MASK_SOLID);
  if(tr.startsolid || tr.allsolid){trap_SendServerCommand(p->s.number,"print \"Not enough room to spawn a Warthog.\n\"");return;}
  v=G_Spawn();VectorCopy(end,v->s.origin);v->s.angles[YAW]=p->client->ps.viewangles[YAW];SP_qce_warthog(v);
 }
}
