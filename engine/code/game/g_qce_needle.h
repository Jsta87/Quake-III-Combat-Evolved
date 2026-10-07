/* Server-only needle behavior. Scalar values come from the generated profile. */
static gentity_t *QceNeedleOwner(gentity_t *ent) {
 gentity_t *owner;
 if(ent->r.ownerNum<0 || ent->r.ownerNum>=ENTITYNUM_WORLD)return &g_entities[ENTITYNUM_WORLD];
 owner=&g_entities[ent->r.ownerNum];
 return owner->inuse && owner->qceEntitySerial==ent->qceOwnerSerial?owner:&g_entities[ENTITYNUM_WORLD];
}
static qboolean QceNeedleTarget(gentity_t *ent,gentity_t *target) {
 return target->inuse && target->client && target->takedamage && target->health>0 &&
 target->qceEntitySerial==ent->qceAttachSerial && target->client->ps.persistant[PERS_SPAWN_COUNT]==ent->qceAttachSpawn;
}
void G_QceNeedleThink(gentity_t *ent) {
 gentity_t *target,*owner;
 if(!ent->qceNeedle)return;
 owner=QceNeedleOwner(ent);
 if(ent->qceStuck && ent->qceAttachEntity>=0 && ent->qceAttachEntity<ENTITYNUM_WORLD) {
  target=&g_entities[ent->qceAttachEntity];
  if(QceNeedleTarget(ent,target) && ent->damage>0)
   G_Damage(target,ent,owner,NULL,ent->r.currentOrigin,ent->damage,DAMAGE_NO_KNOCKBACK,ent->methodOfDeath);
 }
 ent->parent=owner;
 G_ExplodeMissile(ent);
}
qboolean G_QceNeedleImpact(gentity_t *ent,trace_t *trace) {
 gentity_t *target,*needle;
 const qce_weapondef_t *def;
 vec3_t offset,angles,forward,right,up;
 int i,count;
 if(!ent->qceNeedle)return qfalse;
 target=&g_entities[trace->entityNum];def=BG_QceWeaponDef(WP_GRENADE_LAUNCHER);
 ent->qceStuck=1;ent->qceAttachEntity=trace->entityNum;
 ent->qceAttachSerial=target->qceEntitySerial;
 ent->qceAttachSpawn=target->client?target->client->ps.persistant[PERS_SPAWN_COUNT]:0;
 G_SetOrigin(ent,trace->endpos);ent->nextthink=level.time+def->attachment_ms;
 VectorSubtract(trace->endpos,target->r.currentOrigin,offset);
 VectorCopy(target->r.currentAngles,angles);
 if(target->client)VectorSet(angles,0,target->client->ps.viewangles[YAW],0);
 AngleVectors(angles,forward,right,up);
 ent->qceAttachOffset[0]=DotProduct(offset,forward);
 ent->qceAttachOffset[1]=DotProduct(offset,right);
 ent->qceAttachOffset[2]=DotProduct(offset,up);
 /* Count live attachments on this incarnation, across shooters, as the reference does. */
 count=0;
 if(QceNeedleTarget(ent,target)) {
  for(i=0;i<level.num_entities;i++) {
   needle=&g_entities[i];
   if(needle->inuse && needle->s.eType==ET_MISSILE && needle->qceNeedle && needle->qceStuck && !needle->qceNeedleSpent &&
      needle->qceAttachEntity==ent->qceAttachEntity && needle->qceAttachSerial==ent->qceAttachSerial &&
      needle->qceAttachSpawn==ent->qceAttachSpawn && needle->nextthink>=level.time)count++;
  }
  if(def->combine_count>0 && count>=def->combine_count) {
   for(i=0;i<level.num_entities;i++) {
    needle=&g_entities[i];
    if(needle->inuse && needle->s.eType==ET_MISSILE && needle->qceNeedle && needle->qceStuck && !needle->qceNeedleSpent &&
       needle->qceAttachEntity==ent->qceAttachEntity && needle->qceAttachSerial==ent->qceAttachSerial &&
       needle->qceAttachSpawn==ent->qceAttachSpawn && needle->nextthink>=level.time) {
     needle->qceNeedleSpent=1;needle->nextthink=level.time+1;
    }
   }
   ent->damage=0;ent->splashDamage=def->combine_damage;ent->splashRadius=def->combine_radius;
   G_SetOrigin(ent,target->r.currentOrigin);
   VectorClear(ent->qceAttachOffset);
  }
 }
 trap_LinkEntity(ent);return qtrue;
}
void G_QceTrackNeedle(gentity_t *ent) {
 const qce_weapondef_t *def;
 gentity_t *target;
 vec3_t desired,current,origin,turned;
 trace_t trace;
 float speed,dot,angle,step,a,b;
 int elapsed;
 if((!ent->qceNeedle && !ent->qceCharged) || ent->qceStuck || ent->qceTrackEntity<0 || ent->qceTrackEntity>=ENTITYNUM_WORLD)return;
 target=&g_entities[ent->qceTrackEntity];
 elapsed=level.time-ent->qceTrackTime;ent->qceTrackTime=level.time;
 if(elapsed<=0)return;
 if(!target->inuse || !target->client || target->health<=0 || target->qceEntitySerial!=ent->qceTrackSerial ||
    target->client->ps.persistant[PERS_SPAWN_COUNT]!=ent->qceTrackSpawn) {ent->qceTrackEntity=ENTITYNUM_NONE;return;}
 VectorCopy(target->r.currentOrigin,origin);origin[2]+=(target->r.mins[2]+target->r.maxs[2])*0.5f;
 trap_Trace(&trace,ent->r.currentOrigin,NULL,NULL,origin,ent->s.number,MASK_SHOT);
 if(trace.fraction<1 && trace.entityNum!=target->s.number)return;
 VectorSubtract(origin,ent->r.currentOrigin,desired);if(VectorNormalize(desired)==0)return;
 VectorCopy(ent->s.pos.trDelta,current);speed=VectorNormalize(current);if(speed<=0)return;
 dot=DotProduct(current,desired);if(dot>1)dot=1;if(dot<-1)dot=-1;
 angle=atan2(sqrt(1-dot*dot),dot);def=BG_QceWeaponDef(ent->qceCharged?WP_LIGHTNING:WP_GRENADE_LAUNCHER);step=(ent->qceCharged?def->charged_tracking_radians:def->tracking_radians)*elapsed/1000.0f;
 if(angle<=step)VectorCopy(desired,turned);
 else if(angle<3.13f) {
  a=sin(angle-step)/sin(angle);b=sin(step)/sin(angle);
  VectorScale(current,a,turned);VectorMA(turned,b,desired,turned);VectorNormalize(turned);
 } else return;
 /* Change velocity at the last simulated position; retain this frame's elapsed travel. */
 VectorCopy(ent->r.currentOrigin,ent->s.pos.trBase);ent->s.pos.trTime=level.previousTime;
 VectorScale(turned,speed,ent->s.pos.trDelta);
}
static void QceAcquireNeedle(gentity_t *ent,vec3_t dir,float range) {
 gentity_t *target,*owner;
 vec3_t offset,end;
 trace_t trace;
 float best=0.95f,dot,distance;
 int i;
 owner=QceNeedleOwner(ent);ent->qceTrackEntity=ENTITYNUM_NONE;
 for(i=0;i<level.maxclients;i++) {
  target=&g_entities[i];
  if(target==owner || !target->inuse || !target->client || !target->takedamage || target->health<=0 ||
     target->client->sess.sessionTeam==TEAM_SPECTATOR || (g_gametype.integer>=GT_TEAM && OnSameTeam(owner,target)))continue;
  VectorCopy(target->r.currentOrigin,end);end[2]+=(target->r.mins[2]+target->r.maxs[2])*0.5f;
  VectorSubtract(end,ent->r.currentOrigin,offset);distance=VectorNormalize(offset);
  dot=DotProduct(dir,offset);if(distance>range || dot<=best)continue;
  trap_Trace(&trace,ent->r.currentOrigin,NULL,NULL,end,ent->r.ownerNum,MASK_SHOT);
  if(trace.fraction<1 && trace.entityNum!=i)continue;
  best=dot;ent->qceTrackEntity=i;ent->qceTrackSerial=target->qceEntitySerial;
  ent->qceTrackSpawn=target->client->ps.persistant[PERS_SPAWN_COUNT];
 }
 ent->qceTrackTime=level.time;
}
