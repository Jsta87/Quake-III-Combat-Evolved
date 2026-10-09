/* Numeric point-physics behavior from the documented Halo reference. */
#ifndef CG_HALO_PARTICLE_H
#define CG_HALO_PARTICLE_H
static int QCE_ParticlePhase(const int phases[4],int age,int *state,float *blend) {
 int phase=0;
 if(age<0)return 0;
 while(phase<4 && age>=phases[phase])age-=phases[phase++];
 if(phase==4)return 0;
 *state=phase/2;*blend=(phase&1)?age/(float)phases[phase]:0;
 return 1;
}
static void QCE_ParticleAirStep(float position[3],float velocity[3],const float physics[4],float dt) {
 float drag=dt*physics[2]/((physics[0]+0.0011f*118613.34f)*physics[3]);int i;
 if(drag>1)drag=1;
 if(drag<0)drag=0;
 velocity[2]+=0.0035651792f*30*30*80*physics[1]*dt;
 for(i=0;i<3;i++){velocity[i]*=1-drag;position[i]+=velocity[i]*dt;}
}
#endif
