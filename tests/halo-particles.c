#include "q_shared.h"
#include "../engine/code/cgame/cg_halo_particle.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 int phases[4]={0,450,0,1800},state,i;float f,position[3]={0},velocity[3]={192,0,1280};
 float physics[4]={59.306675f,.375f,2000,1.5f};float drag=(1.0f/30)*2000/((59.306675f+.0011f*118613.34f)*1.5f);
 assert(!QCE_ParticlePhase(phases,-1,&state,&f));
 assert(QCE_ParticlePhase(phases,0,&state,&f) && state==0 && f==0);
 assert(QCE_ParticlePhase(phases,450,&state,&f) && state==1 && f==0);
 assert(QCE_ParticlePhase(phases,1350,&state,&f) && state==1 && fabs(f-.5f)<.0001);
 assert(!QCE_ParticlePhase(phases,2250,&state,&f));
 QCE_ParticleAirStep(position,velocity,physics,1.0f/30);
 assert(fabs(velocity[0]-192*(1-drag))<.001);
 assert(velocity[2]>900 && velocity[2]<1000 && position[2]>30 && position[2]<34);
 for(i=1;i<30;i++)QCE_ParticleAirStep(position,velocity,physics,1.0f/30);
 assert(position[2]>130 && position[2]<160 && velocity[2]>9 && velocity[2]<12);
 puts("PASS: zero-duration particle transitions, phase boundaries, source mass/drag/buoyancy and 30-Hz explosion motion");return 0;
}
