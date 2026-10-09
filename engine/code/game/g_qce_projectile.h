/* QCE authoritative projectile travel on Quake BSP geometry. */
void G_QceInitProjectile(gentity_t *bolt,gentity_t *owner,int weapon) {
 bolt->s.weapon=weapon;bolt->qceProjectileWeapon=weapon;bolt->qceProjectileTime=level.time;
 bolt->qceOwnerSerial=owner->qceEntitySerial;bolt->qceDamageScale=1;
 bolt->s.pos.trTime=level.time;bolt->s.pos.trType=TR_LINEAR;
}

void G_QceFireBullet(gentity_t *owner,vec3_t start,vec3_t end,int weapon,int mod,int quad) {
 const qce_weapondef_t *def=BG_QceWeaponDef(weapon);
 vec3_t dir;
 gentity_t *bolt=G_Spawn();
 bolt->classname="qce_bullet";bolt->s.eType=ET_MISSILE;
 bolt->r.svFlags=SVF_NOCLIENT;bolt->s.weapon=weapon;
 bolt->r.ownerNum=owner->s.number;bolt->parent=owner;bolt->clipmask=MASK_SHOT;
 bolt->qceBullet=1;bolt->count=quad;bolt->damage=def->damage;bolt->methodOfDeath=mod;
 G_QceInitProjectile(bolt,owner,weapon);
 VectorCopy(start,bolt->r.currentOrigin);VectorCopy(start,bolt->s.pos.trBase);VectorCopy(start,bolt->s.origin2);
 VectorSubtract(end,start,dir);VectorNormalize(dir);VectorScale(dir,def->projectile_speed,bolt->s.pos.trDelta);
 bolt->think=G_FreeEntity;bolt->nextthink=level.time+60000;
}

/* Eight-millisecond substeps bound curved sweeps; range is distance, not a
 * timeout estimated from launch speed. Water selects its own tagged response. */
static void G_QceRunProjectile(gentity_t *ent) {
 const qce_weapondef_t *def=BG_QceWeaponDef(ent->qceProjectileWeapon);
 vec3_t origin,velocity,delta;
 float initial,final,lower,upper,gravity,range,speed,newSpeed,deceleration,dt,length,scale;
 int elapsed=level.time-ent->qceProjectileTime,part,passent,water;
 trace_t tr;
 if(ent->s.pos.trType==TR_STATIONARY) {G_RunThink(ent);return;}
 if(elapsed<0)elapsed=0;
 while(elapsed>0 && ent->inuse) {
  part=elapsed>8?8:elapsed;elapsed-=part;dt=part*0.001f;
  water=trap_PointContents(ent->r.currentOrigin,ent->r.ownerNum)&MASK_WATER;
  gravity=water?def->water_gravity:def->projectile_gravity;
  lower=water?def->water_falloff_start:def->falloff_start;
  upper=water?def->water_falloff_end:def->falloff_end;
  initial=def->projectile_speed;final=def->projectile_final_speed;range=def->projectile_range;
  if(ent->qceCharged) {initial=final=def->charged_speed;range=def->charged_range;gravity=0;}
  if(ent->qceGrenadeType) {
   gravity=BG_QceGrenadeDef(ent->qceGrenadeType-1)->gravity;
   initial=final=0;range=0;
  }
  VectorCopy(ent->s.pos.trDelta,velocity);speed=VectorLength(velocity);newSpeed=speed;
  if(initial>final && upper>lower && ent->qceTravelled+speed*dt>lower) {
   float decelTime=dt;
   if(ent->qceTravelled<lower && speed>0)decelTime-=(lower-ent->qceTravelled)/speed;
   deceleration=(initial*initial-final*final)/(2*(upper-lower));
   newSpeed=speed-deceleration*decelTime;if(newSpeed<final)newSpeed=final;
   if(speed>0)VectorScale(velocity,newSpeed/speed,velocity);
  }
  VectorMA(ent->r.currentOrigin,dt*0.5f,ent->s.pos.trDelta,origin);
  VectorMA(origin,dt*0.5f,velocity,origin);
  origin[2]-=0.5f*BG_QceMovementDef()->gravity*gravity*dt*dt;
  velocity[2]-=BG_QceMovementDef()->gravity*gravity*dt;
  VectorSubtract(origin,ent->r.currentOrigin,delta);length=VectorLength(delta);
  if(range>0 && ent->qceTravelled+length>range) {
   scale=length>0?(range-ent->qceTravelled)/length:0;
   if(scale<0)scale=0;
   VectorMA(ent->r.currentOrigin,scale,delta,origin);length*=scale;
  }
  passent=ent->target_ent?ent->target_ent->s.number:ent->r.ownerNum;
  trap_Trace(&tr,ent->r.currentOrigin,ent->r.mins,ent->r.maxs,origin,passent,ent->clipmask);
  if(tr.startsolid || tr.allsolid)tr.fraction=0;
  ent->qceTravelled+=length*tr.fraction;
  VectorCopy(tr.endpos,ent->r.currentOrigin);VectorCopy(velocity,ent->s.pos.trDelta);
  ent->qceDamageScale=initial>final?(speed+(newSpeed-speed)*tr.fraction-final)/(initial-final):1;
  if(initial>final && upper>lower && ent->qceTravelled>=upper)ent->qceDamageScale=0;
  if(ent->qceDamageScale<0)ent->qceDamageScale=0;
  if(ent->qceDamageScale>1)ent->qceDamageScale=1;
  if(tr.fraction<1) {
   if(tr.surfaceFlags&SURF_NOIMPACT) {G_FreeEntity(ent);return;}
   G_MissileImpact(ent,&tr);
   if(!ent->inuse || ent->s.eType!=ET_MISSILE || ent->qceStuck)return;
   if(ent->s.pos.trType==TR_STATIONARY)break;
  }
  if(range>0 && ent->qceTravelled>=range-0.001f) {G_FreeEntity(ent);return;}
  /* A zero maximum range means expire after the tagged slowdown finishes. */
  if(!ent->qceGrenadeType && range==0 && initial>final && newSpeed<=final) {G_FreeEntity(ent);return;}
 }
 ent->qceProjectileTime=level.time;
 VectorCopy(ent->r.currentOrigin,ent->s.pos.trBase);ent->s.pos.trTime=level.time;
 trap_LinkEntity(ent);G_RunThink(ent);
}
