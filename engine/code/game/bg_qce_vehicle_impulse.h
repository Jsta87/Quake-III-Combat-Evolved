/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BG_QCE_VEHICLE_IMPULSE_H
#define BG_QCE_VEHICLE_IMPULSE_H
/* vehicles.c vehicle_accelerate: translation plus cross(up, acceleration)*pi.
 * Velocity is in Quake units/s; convert to Halo world units for angular impulse. */
static void QCE_HogBlastImpulse(const vec3_t direction,float strength,vec3_t velocity,vec3_t angularVelocity) {
 vec3_t dir,torque,up={0,0,1};
 VectorNormalize2(direction,dir);dir[2]+=.45f;VectorNormalize(dir);
 VectorMA(velocity,strength,dir,velocity);CrossProduct(up,dir,torque);
 VectorMA(angularVelocity,strength*M_PI/80,torque,angularVelocity);
}
#endif
