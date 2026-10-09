/* SPDX-License-Identifier: GPL-3.0-only
 * Server-authoritative first Warthog prototype. No retail rigid-body simulation.
 */
#include "g_local.h"
#include "bg_qce_vehicle.h"
static const vec3_t seatOffsets[3]={{12,25,33},{12,-25,33},{-52,0,50}};
static gentity_t *Vehicle(gentity_t *player) {
 int n=player->client->ps.qceVehicle,seat=player->client->ps.qceVehicleSeat;
 if(n<=0 || n>=level.num_entities || seat<0 || seat>=3)return NULL;
 if(!g_entities[n].inuse || !g_entities[n].qceVehicle || g_entities[n].health<=0 || g_entities[n].qceRiders[seat]!=player->s.number+1)return NULL;
 return &g_entities[n];
}
static void SeatOrigin(gentity_t *v,int seat,vec3_t result) {
 vec3_t axis[3];int i;AnglesToAxis(v->r.currentAngles,axis);VectorCopy(v->r.currentOrigin,result);
 for(i=0;i<3;i++)VectorMA(result,seatOffsets[seat][i],axis[i],result);
}
void G_QceVehicleRelease(gentity_t *p,qboolean force) {
 gentity_t *v;trace_t tr;vec3_t dest,mins={-15,-15,-24},maxs={15,15,32},axis[3];int seat,i,found=0;
 if(!p->client || !p->client->ps.qceVehicle)return;
 v=Vehicle(p);seat=p->client->ps.qceVehicleSeat;
 if(v) {
  AnglesToAxis(v->r.currentAngles,axis);
  for(i=0;i<6;i++) {
   VectorCopy(v->r.currentOrigin,dest);dest[2]+=40;
   if(i<4)VectorMA(dest,(i&1?-1:1)*(i<2?112:170),axis[1],dest);
   else VectorMA(dest,(i&1?-1:1)*200,axis[0],dest);
   trap_Trace(&tr,dest,mins,maxs,dest,p->s.number,MASK_PLAYERSOLID);
   if(!tr.startsolid && !tr.allsolid){found=1;break;}
  }
  if(!found && !force){trap_SendServerCommand(p->s.number,"print \"Exit blocked. Move the Warthog to open ground.\n\"");return;}
  if(seat>=0 && seat<3 && v->qceRiders[seat]==p->s.number+1)v->qceRiders[seat]=0;
  if(found){VectorCopy(dest,p->client->ps.origin);VectorCopy(dest,p->r.currentOrigin);}
 }
 p->client->ps.qceVehicle=p->client->ps.qceVehicleSeat=0;
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
 for(i=0;i<3;i++)if(!best->qceRiders[i]){seat=i;break;}
 if(seat<0)return qfalse;
 /* A wall between player and vehicle must block entry. */
 {trace_t tr;trap_Trace(&tr,p->client->ps.origin,NULL,NULL,best->r.currentOrigin,p->s.number,MASK_SOLID);
  if(tr.startsolid || tr.allsolid || (tr.fraction<1 && tr.entityNum!=best->s.number))return qfalse;}
 best->qceRiders[seat]=p->s.number+1;p->client->ps.qceVehicle=best->s.number;p->client->ps.qceVehicleSeat=seat;
 p->client->ps.qceZoom=0;p->client->ps.pm_type=PM_FREEZE;p->client->ps.eFlags^=EF_TELEPORT_BIT;
 VectorClear(p->client->ps.velocity);SeatOrigin(best,seat,p->client->ps.origin);
 p->r.contents=CONTENTS_BODY;p->client->qcePickupLatched=qtrue;
 trap_SendServerCommand(p->s.number,va("print \"Warthog %s: E exits; WASD drives from driver seat.\n\"",seat==0?"driver":seat==1?"passenger":"rear seat (turret pending)"));
 return qtrue;
}
void G_QceVehicleInput(gentity_t *p,usercmd_t *cmd) {
 if(p->client->ps.qceVehicle) {
  if(!Vehicle(p) || p->client->noclip || p->health<=0)G_QceVehicleRelease(p,qtrue);
  else p->client->ps.pm_type=PM_FREEZE;
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
static void VehicleThink(gentity_t *v) {
 float dt=Com_Clamp(0.001f,0.1f,(level.time-v->qceVehicleTime)*0.001f),throttle=0,steer=0;
 vec3_t forward,end,start,normal,flat,axis[3];trace_t tr;int i;gentity_t *p;float groundZ=-99999;
 v->qceVehicleTime=level.time;v->nextthink=level.time+1;
 if(v->health<=0) {
  if(level.time<v->qceVehicleRespawn)return;
  VectorCopy(v->pos1,v->r.currentOrigin);VectorCopy(v->pos2,v->r.currentAngles);VectorClear(v->qceVehicleVelocity);v->qceVehicleSpeed=0;
  v->health=500;v->takedamage=qtrue;v->s.eFlags&=~EF_NODRAW;v->r.contents=CONTENTS_SOLID;
 }
 for(i=0;i<3;i++)if(v->qceRiders[i]) {
  p=&g_entities[v->qceRiders[i]-1];
  if(!p->inuse || !p->client || p->health<=0 || p->client->ps.qceVehicle!=v->s.number || p->client->ps.qceVehicleSeat!=i){v->qceRiders[i]=0;continue;}
  if(i==0){throttle=p->client->pers.cmd.forwardmove/127.0f;steer=-p->client->pers.cmd.rightmove/127.0f;}
 }
 /* Four support probes align the hull to a gently filtered terrain normal. */
 VectorSet(flat,0,v->r.currentAngles[YAW],0);AnglesToAxis(flat,axis);VectorClear(normal);
 for(i=0;i<4;i++) {
  VectorCopy(v->r.currentOrigin,start);VectorMA(start,i&1?72:-72,axis[0],start);VectorMA(start,i&2?38:-38,axis[1],start);start[2]+=36;
  VectorCopy(start,end);end[2]-=100;
  trap_Trace(&tr,start,NULL,NULL,end,v->s.number,MASK_SOLID);
  if(tr.fraction<1 && !tr.startsolid && tr.plane.normal[2]>.25f){float height=tr.endpos[2]+(tr.plane.normal[0]*(tr.endpos[0]-v->r.currentOrigin[0])+tr.plane.normal[1]*(tr.endpos[1]-v->r.currentOrigin[1]))/tr.plane.normal[2];VectorAdd(normal,tr.plane.normal,normal);if(height>groundZ)groundZ=height;}
 }
 if(groundZ>-99999 && v->r.currentOrigin[2]-groundZ<45) {
  VectorNormalize(normal);v->qceVehicleSpeed=QCE_HogSpeed(v->qceVehicleSpeed,throttle,dt);
  v->r.currentAngles[YAW]+=steer*v->qceVehicleSpeed/120.0f*33.0797f*dt;
  AngleVectors(v->r.currentAngles,forward,NULL,NULL);
  v->r.currentAngles[PITCH]+=(RAD2DEG(atan2(DotProduct(normal,axis[0]),normal[2]))-v->r.currentAngles[PITCH])*Com_Clamp(0,1,dt*5);
  v->r.currentAngles[ROLL]+=(RAD2DEG(atan2(-DotProduct(normal,axis[1]),normal[2]))-v->r.currentAngles[ROLL])*Com_Clamp(0,1,dt*5);
  QCE_HogGroundVelocity(forward,v->qceVehicleSpeed,normal,groundZ+2-v->r.currentOrigin[2],v->qceVehicleVelocity);
 } else v->qceVehicleVelocity[2]-=g_gravity.value*dt;
 VectorMA(v->r.currentOrigin,dt,v->qceVehicleVelocity,end);
 trap_Trace(&tr,v->r.currentOrigin,v->r.mins,v->r.maxs,end,v->s.number,MASK_SOLID);
 if(!tr.startsolid){VectorCopy(tr.endpos,v->r.currentOrigin);if(tr.fraction<1){float into=DotProduct(v->qceVehicleVelocity,tr.plane.normal);VectorMA(v->qceVehicleVelocity,-into,tr.plane.normal,v->qceVehicleVelocity);if(tr.plane.normal[2]<.5f)v->qceVehicleSpeed*=.35f;}}
 else {v->qceVehicleSpeed=0;VectorClear(v->qceVehicleVelocity);}
 v->s.pos.trType=TR_LINEAR;v->s.pos.trTime=level.time;VectorCopy(v->r.currentOrigin,v->s.pos.trBase);VectorCopy(v->qceVehicleVelocity,v->s.pos.trDelta);
 v->s.apos.trType=TR_INTERPOLATE;VectorCopy(v->r.currentAngles,v->s.apos.trBase);trap_LinkEntity(v);
 for(i=0;i<3;i++)if(v->qceRiders[i]) {
  p=&g_entities[v->qceRiders[i]-1];SeatOrigin(v,i,p->client->ps.origin);VectorCopy(p->client->ps.origin,p->r.currentOrigin);VectorClear(p->client->ps.velocity);p->r.contents=CONTENTS_BODY;
  BG_PlayerStateToEntityState(&p->client->ps,&p->s,qtrue);trap_LinkEntity(p);
 }
}
void SP_qce_warthog(gentity_t *v) {
 v->classname="qce_warthog";v->qceVehicle=1;v->s.eType=ET_GENERAL;v->s.generic1=QCE_HOG_MARKER;
 v->s.modelindex=G_ModelIndex(QCE_HOG_MODEL);v->clipmask=MASK_SOLID;v->r.contents=CONTENTS_SOLID;
 VectorSet(v->r.mins,-76,-49,20);VectorSet(v->r.maxs,76,49,66);
 VectorCopy(v->s.origin,v->r.currentOrigin);VectorCopy(v->s.angles,v->r.currentAngles);
 VectorCopy(v->r.currentOrigin,v->pos1);VectorCopy(v->r.currentAngles,v->pos2);
 v->health=500;v->takedamage=qtrue;v->die=VehicleDie;v->think=VehicleThink;v->qceVehicleTime=level.time;v->nextthink=level.time+1;
 VectorCopy(v->r.currentOrigin,v->s.pos.trBase);VectorCopy(v->r.currentAngles,v->s.apos.trBase);trap_LinkEntity(v);
}
void G_QceVehicleCommand(gentity_t *p,const char *cmd) {
 if(!Q_stricmp(cmd,"qce_vehicle_exit")){G_QceVehicleRelease(p,qfalse);return;}
 if(!Q_stricmp(cmd,"qce_vehicle_enter")){G_QceVehicleUse(p);return;}
 if(!Q_stricmp(cmd,"qce_vehicle_status")) {
  gentity_t *v=Vehicle(p);trap_SendServerCommand(p->s.number,va("print \"Warthog entity=%d seat=%d speed=%.1f health=%d\n\"",p->client->ps.qceVehicle,p->client->ps.qceVehicleSeat,v?v->qceVehicleSpeed:0,v?v->health:0));
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
