/* SPDX-License-Identifier: GPL-3.0-only */
#include "cg_local.h"
#include "cg_qce_visual.h"
float QCE_ShieldFade(int remaining,int broken) {
 float f=remaining/(float)(broken?QCE_SHIELD_BREAK_MS:QCE_SHIELD_HIT_MS);
 if(f<0)f=0;if(f>1)f=1;return f;
}
int QCE_DeathStart(int now,int serverTime) {
 return serverTime>0 && serverTime<=now?serverTime:now;
}
void QCE_ViewSway(qceViewPlayback_t *p,int now,const vec3_t angles) {
 float dt=(now-p->swayTime)*0.001f,blend;int axis;
 if(!p->swayValid || dt<=0 || dt>0.2f) {
  for(axis=0;axis<3;axis++){p->swayAngles[axis]=angles[axis];p->swayOffset[axis]=0;}
  p->swayValid=1;dt=0;
 }
 blend=dt/(dt+0.083333f);
 for(axis=0;axis<2;axis++) {
  float delta=angles[axis]-p->swayAngles[axis],target;
  while(delta>180)delta-=360;while(delta< -180)delta+=360;
  target=dt>0?delta/dt*0.0025f:0;
  if(target< -1.2f)target=-1.2f;if(target>1.2f)target=1.2f;
  p->swayOffset[axis]+=(target-p->swayOffset[axis])*blend;
 }
 for(axis=0;axis<3;axis++)p->swayAngles[axis]=angles[axis];p->swayTime=now;
}
