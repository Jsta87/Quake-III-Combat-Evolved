#include "q_shared.h"
#include "bg_public.h"
#include "bg_qce_aim.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 vec3_t eye={0,0,24},direction={1,0,0},base={400,15,0},target;
 float a,m,d;
 assert(QCE_AimAttenuation(5,10)==1);
 assert(QCE_AimAttenuation(7.5f,10)==0.5f);
 assert(QCE_AimAttenuation(10,10)==0);
 QCE_AimPill(eye,direction,base,28,6.4f,target);
 assert(fabs(target[2]-24)<0.001f && fabs(target[1]-8.6f)<0.001f);
 QCE_AimLevels(WP_BFG,1,400,0.01f,&a,&m,&d);assert(a==1 && m==1);
 QCE_AimLevels(WP_BFG,1,2400,0,&a,&m,&d);assert(a==0 && m==0);
 QCE_AimLevels(WP_RAILGUN,1,100,0,&a,&m,&d);assert(a==0 && m==0);
 QCE_AimLevels(WP_RAILGUN,8,8000,0.001f,&a,&m,&d);assert(a==1 && m==1);
 QCE_AimLevels(WP_ROCKET_LAUNCHER,1,200,0,&a,&m,&d);assert(a==0 && m==1 && d==0);
 puts("PASS: CE plateau/falloff attenuation, closest-point pill width, tagged cones/ranges, zoom-only sniper and rocket zero projectile correction");return 0;
}
