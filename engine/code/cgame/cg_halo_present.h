/* QCE first-person presentation. Pure timing helpers shared by client tests.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef CG_HALO_PRESENT_H
#define CG_HALO_PRESENT_H
static int QCE_ViewDuration( const qceViewClip_t *clip ) {
 return clip->fps>0 ? clip->count*1000/clip->fps : 0;
}
static int QCE_ViewSelect( qceViewPlayback_t *p, const qceViewInput_t *in, const qceViewClip_t *clips ) {
 int clip=QCE_VIEW_IDLE, start=in->now, restart=0;
 int fire=in->chargedFire && clips[QCE_VIEW_CHARGED_FIRE].count>0 ? QCE_VIEW_CHARGED_FIRE : QCE_VIEW_FIRE;
 if(p->weapon!=in->weapon || in->now<p->time) {
  memset(p,0,sizeof(*p));p->weapon=in->weapon;p->state=-1;p->clip=-1;p->soundClip=-1;p->ejectSequence=-1;
 }
 if(p->hot && !in->hot)p->recoveryStart=in->now;
 if(in->state==WEAPON_RAISING)clip=QCE_VIEW_READY;
 else if(in->state==WEAPON_DROPPING)clip=QCE_VIEW_PUTAWAY;
 else if(in->state==WEAPON_RELOAD_ENTER)clip=QCE_VIEW_RELOAD_ENTER;
 else if(in->state==WEAPON_RELOAD_EXIT)clip=QCE_VIEW_RELOAD_EXIT;
 else if(in->state==WEAPON_RELOAD_EXIT_EMPTY)clip=QCE_VIEW_RELOAD_EXIT_EMPTY;
 else if(in->state==WEAPON_RELOADING) {
  clip=(p->state==WEAPON_RELOADING && in->reloadRounds!=1)?p->clip:
       (in->magazine>0?QCE_VIEW_RELOAD_FULL:QCE_VIEW_RELOAD_EMPTY);
  restart=in->reloadRounds==1 && (in->magazine!=p->magazine || in->weaponTime>p->weaponTime+8);
 }
 else if(in->state==WEAPON_MELEEING) {clip=QCE_VIEW_MELEE;start=in->meleeTime>0?in->meleeTime:in->now;}
 else if(in->grenadeTime>0 && in->now-in->grenadeTime<QCE_ViewDuration(&clips[QCE_VIEW_GRENADE])) {clip=QCE_VIEW_GRENADE;start=in->grenadeTime;}
 /* Heat is authoritative on the final shot: enter cooling immediately. */
 else if(in->hot) {
  clip=in->chargedFire && clips[QCE_VIEW_CHARGED_HOT].count>0?QCE_VIEW_CHARGED_HOT:QCE_VIEW_OVERHEAT;
  if(p->hot && (p->clip==QCE_VIEW_HOT_IDLE || ((p->clip==QCE_VIEW_OVERHEAT || p->clip==QCE_VIEW_CHARGED_HOT) && in->now-p->start>=QCE_ViewDuration(&clips[clip]))))clip=QCE_VIEW_HOT_IDLE;
 }
 else if(in->fireTime>0 && in->now-in->fireTime<QCE_ViewDuration(&clips[fire])) {clip=fire;start=in->fireTime;}
 else if(in->charge>0) {
  start=in->now-in->charge;
  clip= in->charge<in->chargeMs && clips[QCE_VIEW_CHARGE_ENTER].count>0 ? QCE_VIEW_CHARGE_ENTER : QCE_VIEW_CHARGE;
 }
 else if(p->recoveryStart>0 && in->now-p->recoveryStart<QCE_ViewDuration(&clips[QCE_VIEW_RECOVER])) {clip=QCE_VIEW_RECOVER;start=p->recoveryStart;}
 if(clips[clip].count<=0 && clip==QCE_VIEW_RELOAD_EMPTY)clip=QCE_VIEW_RELOAD_FULL;
 if(clips[clip].count<=0)clip=QCE_VIEW_IDLE;
 /* Firing ends during cooling; that state change must not restart its clip/audio. */
 if(clip!=p->clip || (in->state!=p->state && in->phaseMs>0) || restart) {
  p->start=start;p->clip=clip;
  if(in->phaseMs>0 && in->weaponTime>0) {
   int elapsed=in->phaseMs-in->weaponTime;
   if(elapsed<0)elapsed=0;
   p->start=in->now-elapsed;
  }
 }
 if(clip==QCE_VIEW_FIRE || clip==QCE_VIEW_CHARGED_FIRE || clip==QCE_VIEW_GRENADE)p->start=start;
 p->state=in->state;p->magazine=in->magazine;p->weaponTime=in->weaponTime;p->hot=in->hot;p->time=in->now;
 return clip;
}
static void QCE_ViewFrames( const qceViewClip_t *clip, int kind, int elapsed, int phaseMs, int *oldframe, int *frame, float *backlerp ) {
 float value;
 int step,loop=kind==QCE_VIEW_IDLE || kind==QCE_VIEW_CHARGE || kind==QCE_VIEW_HOT_IDLE;
 /* Charging enters the authored pose; charged jitter is an additive overlay. */
 if(kind==QCE_VIEW_CHARGE) {
  *oldframe=*frame=clip->first+clip->count-1;*backlerp=0;return;
 }
 if(elapsed<0)elapsed=0;
 value=phaseMs>0 ? elapsed*(clip->count-1)/(float)phaseMs : elapsed*clip->fps/1000.0f;
 step=(int)value;
 if(loop && step>=clip->count)step=clip->loop+(step-clip->loop)%(clip->count-clip->loop);
 else if(!loop && step>=clip->count-1) {step=clip->count-1;value=(float)step;}
 *oldframe=clip->first+step;*frame=*oldframe+1;
 if(step==clip->count-1)*frame=clip->first+(loop?clip->loop:step);
 *backlerp=1.0f-(value-(int)value);
}
/* Trigger each event once per clip instance, including repeated shots/inserts. */
static int QCE_ViewSoundDue( qceViewPlayback_t *p, int elapsed, const qceViewClip_t *clip, const qceViewSound_t *event, int phaseMs ) {
 int due;
 if(p->soundClip!=p->clip || p->soundStart!=p->start) {
  p->soundClip=p->clip;p->soundStart=p->start;p->soundPlayed=0;
 }
 due=phaseMs>0 && clip->count>1 ? event->frame*phaseMs/(clip->count-1) : event->frame*1000/clip->fps;
 if(event->count>0 && !event->loop && !p->soundPlayed && elapsed>=due) {p->soundPlayed=1;return 1;}
 return 0;
}
#endif
