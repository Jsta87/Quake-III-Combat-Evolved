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
/* source driver/passenger/gunner exit tracks: 26/30/24 frames at 30 Hz */
static int QCE_HogExitMS(int seat) {return seat==0?867:seat==1?1000:800;}
/* QVM has no exp syscall. Reciprocal Taylor approximates 1-exp(-rate*dt). */
static float QCE_HogFilter(float dt,float rate) {
 float x=Com_Clamp(0,2,dt*rate);
 return 1-1/(1+x*(1+x*(.5f+x*(1.0f/6+x*(1.0f/24+x/120)))));
}
static float QCE_HogRamp(float value,qboolean active,float grow,float recover,float dt) {
 return Com_Clamp(0,1,value+(active?dt/grow:-dt/recover));
}
static float QCE_HogSuspensionAngle(int front,float wheelHeight) {
 float lo=front?-15:-30,hi=front?30:15,dx=front?19.34f:-18.959f,dz=front?-2.815f:-3.209f,mid,z;int i;
 for(i=0;i<16;i++){mid=(lo+hi)*.5f;z=-dx*sin(DEG2RAD(mid))+dz*cos(DEG2RAD(mid));if((z<wheelHeight)==(front==0))lo=mid;else hi=mid;}
 return (lo+hi)*.5f;
}
static void QCE_HogTurretTransform(const vec3_t origin,const vec3_t hullAngles,float yaw,float pitch,vec3_t gunOrigin,vec3_t muzzle,vec3_t gunAxis[3]) {
 vec3_t hull[3],turn[3],local[3],base,angles;int i;
 AnglesToAxis(hullAngles,hull);VectorCopy(origin,base);VectorMA(base,-40,hull[0],base);VectorMA(base,35.091f,hull[2],base);
 VectorSet(angles,0,yaw,0);AnglesToAxis(angles,local);MatrixMultiply(local,hull,turn);
 VectorCopy(base,gunOrigin);VectorMA(gunOrigin,22.376f,turn[0],gunOrigin);VectorMA(gunOrigin,30.421f,turn[2],gunOrigin);
 VectorSet(angles,pitch,0,0);AnglesToAxis(angles,local);MatrixMultiply(local,turn,gunAxis);
 VectorCopy(gunOrigin,muzzle);for(i=0;i<3;i++)VectorMA(muzzle,i==0?39:i==2?7.688f:0,gunAxis[i],muzzle);
}
static float QCE_HogSteer(float vehicleYaw,float mouseYaw) {
 return Com_Clamp(-1,1,AngleSubtract(mouseYaw,vehicleYaw)/30.0f);
}
static float QCE_HogThrottle(int forward,int side,int up) {
 return side<0 || up>0?0:forward/127.0f;
}
static void QCE_HogContactPlane(const float h[4],vec3_t axis[3],vec3_t normal,float *height) {
 float riseF=((h[1]+h[3])-(h[0]+h[2]))*.5f/104.0f;
 float riseL=((h[2]+h[3])-(h[0]+h[1]))*.5f/60.8f;
 VectorSet(normal,0,0,1);VectorMA(normal,-riseF,axis[0],normal);VectorMA(normal,-riseL,axis[1],normal);
 *height=(h[0]+h[1]+h[2]+h[3])*.25f-riseF*1.6f;
}
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
