/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BG_QCE_VEHICLE_H
#define BG_QCE_VEHICLE_H
/* Xbox Blood Gulch vehi vehicles\\warthog\\warthog, build 2276.
 * Retail per-tick speed/acceleration converted at 30 Hz and 80 Quake units/WU.
 * Hull, steering/suspension and health below are prototype tuning, not retail physics. */
#define QCE_HOG_FORWARD (0.255f*30*80)
#define QCE_HOG_REVERSE (0.1f*30*80)
#define QCE_HOG_ACCEL (0.00275f*30*30*80)
#define QCE_HOG_BRAKE (0.011f*30*30*80)
#define QCE_HOG_MODEL "models/qce/halo/warthog.md3"
#define QCE_HOG_MARKER 0x5143
static float QCE_HogSpeed(float speed,float throttle,float dt) {
 float target=throttle>=0?throttle*QCE_HOG_FORWARD:throttle*QCE_HOG_REVERSE;
 float rate=((speed<0 && target>0)||(speed>0 && target<0)||throttle==0)?QCE_HOG_BRAKE:QCE_HOG_ACCEL;
 float step=rate*dt;
 if(speed<target){speed+=step;if(speed>target)speed=target;}
 else {speed-=step;if(speed<target)speed=target;}
 return speed;
}
static void QCE_HogGroundVelocity(const vec3_t forward,float speed,const vec3_t normal,float heightError,vec3_t velocity) {
 float into;
 VectorScale(forward,speed,velocity);into=DotProduct(velocity,normal);
 VectorMA(velocity,-into,normal,velocity);
 velocity[2]+=heightError*8;
}
#endif
