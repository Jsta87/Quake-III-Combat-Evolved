/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CG_QCE_CAMERA_H
#define CG_QCE_CAMERA_H
/* Quake viewaxis[1] points left. Output is normalized viewport coordinates. */
static qboolean QCE_ProjectAim(const vec3_t point,const vec3_t eye,vec3_t axis[3],float fovX,float fovY,float *x,float *y) {
 vec3_t delta;float depth,tx,ty;
 VectorSubtract(point,eye,delta);depth=DotProduct(delta,axis[0]);
 if(depth<=0.01f || fovX<=0 || fovX>=179 || fovY<=0 || fovY>=179)return qfalse;
 tx=tan(fovX*M_PI/360);ty=tan(fovY*M_PI/360);
 *x=.5f-DotProduct(delta,axis[1])/(2*depth*tx);
 *y=.5f-DotProduct(delta,axis[2])/(2*depth*ty);
 return *x>=0 && *x<=1 && *y>=0 && *y<=1;
}
#endif
