/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CG_QCE_PROJECTILE_VISUAL_H
#define CG_QCE_PROJECTILE_VISUAL_H
static float QCE_VisualOffsetWeight(float progress) {
 float t=Com_Clamp(0,1,progress);
 return 1-t*t*(3-2*t);
}
static float QCE_ProjectileVisualWeight(const vec3_t start,const vec3_t target,const vec3_t current) {
 vec3_t delta,travel;float lengthSquared;
 VectorSubtract(target,start,delta);VectorSubtract(current,start,travel);
 lengthSquared=DotProduct(delta,delta);
 return lengthSquared>.001f?QCE_VisualOffsetWeight(DotProduct(travel,delta)/lengthSquared):0;
}
#endif
