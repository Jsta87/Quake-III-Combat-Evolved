/* SPDX-License-Identifier: GPL-3.0-or-later
 * Xbox CE aim attenuation and closest-point pill targeting. */
#ifndef BG_QCE_AIM_H
#define BG_QCE_AIM_H
typedef struct {float autoAngle,autoRange,magnetAngle,magnetRange,deviation;int zoomOnly;} qce_aimdef_t;
#include "bg_qce_aim.generated.h"
static float QCE_AimAttenuation(float value,float maximum) {
 if(maximum<=0 || value>=maximum)return 0;
 return value<=maximum*0.5f?1:(maximum-value)/(maximum*0.5f);
}
static void QCE_AimPill(const vec3_t eye,const vec3_t direction,const vec3_t base,float height,float width,vec3_t target) {
 vec3_t offset,perpendicular;float denominator,t,projection,length;
 VectorSubtract(eye,base,offset);denominator=1-direction[2]*direction[2];
 t=denominator>0.00001f?(offset[2]-direction[2]*DotProduct(offset,direction))/denominator:0;
 if(t<0)t=0;
 if(t>height)t=height;
 VectorCopy(base,target);target[2]+=t;VectorSubtract(target,eye,offset);
 projection=DotProduct(offset,direction);VectorMA(offset,-projection,direction,perpendicular);
 length=VectorLength(perpendicular);if(length>width && length>0)VectorScale(perpendicular,width/length,perpendicular);
 VectorSubtract(target,perpendicular,target);
}
static void QCE_AimLevels(int weapon,float zoom,float distance,float angle,float *autoLevel,float *magnetLevel,float *deviation) {
 const qce_aimdef_t *def=&qce_aimdefs[weapon];
 *autoLevel=*magnetLevel=0;*deviation=0;
 if(def->zoomOnly && zoom<=1)return;
 if(zoom<1)zoom=1;
 *autoLevel=QCE_AimAttenuation(distance,def->autoRange*zoom)*QCE_AimAttenuation(angle,def->autoAngle/zoom);
 *magnetLevel=QCE_AimAttenuation(distance,def->magnetRange*zoom)*QCE_AimAttenuation(angle,def->magnetAngle/zoom);
 *deviation=(def->deviation>def->autoAngle?def->deviation:def->autoAngle)/zoom;
}
#endif
