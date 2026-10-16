#include <assert.h>
#include <math.h>
#include <stdio.h>
#ifdef TEST_GL2
#include "../engine/code/renderergl2/tr_local.h"
#else
#include "../engine/code/renderergl1/tr_local.h"
#endif
int main(void) {
 iqmData_t data;
 orientation_t tag;
 iqmTransform_t poses[8];
 refEntity_t overlay;
 int parents[2]={-1,0},i;
 float bind[24]={1,0,0,0,0,1,0,0,0,0,1,0, 1,0,0,4,0,1,0,0,0,0,1,0};
 float inverse[24];
 memset(&data,0,sizeof(data));memset(poses,0,sizeof(poses));memcpy(inverse,bind,sizeof(bind));inverse[15]=-4;
 for(i=0;i<8;i++) {poses[i].rotate[3]=1;VectorSet(poses[i].scale,1,1,1);}
 poses[1].translate[0]=poses[3].translate[0]=4;poses[2].translate[0]=10;
 data.num_joints=data.num_poses=2;data.num_frames=2;data.jointNames="root\0tag_flash";
 data.jointParents=parents;data.bindJoints=bind;data.invBindJoints=inverse;data.poses=poses;
 assert(R_IQMLerpTag(&tag,&data,0,1,0,"tag_flash"));assert(fabs(tag.origin[0]-4)<0.0001);
 assert(R_IQMLerpTag(&tag,&data,0,1,1,"tag_flash"));assert(fabs(tag.origin[0]-14)<0.0001);
 assert(R_IQMLerpTag(&tag,&data,0,1,0.25f,"tag_flash"));assert(fabs(tag.origin[0]-6.5f)<0.0001);
 assert(R_IQMLerpTag(&tag,&data,1,0,0.25f,"tag_flash"));assert(fabs(tag.origin[0]-11.5f)<0.0001);
 assert(!R_IQMLerpTag(&tag,&data,0,1,0,"missing"));
 memset(&overlay,0,sizeof(overlay));data.num_frames=4;
 poses[4].translate[1]=2;poses[6].translate[1]=6;
 overlay.qceOverlayOldFrame=2;overlay.qceOverlayFrame=3;overlay.qceOverlayJoints=1;overlay.qceOverlayWeight=1;overlay.qceOverlayBacklerp=0.5f;
 assert(R_IQMLerpTagRef(&tag,&data,0,1,0.25f,"tag_flash",&overlay));assert(fabs(tag.origin[0]-6.5f)<0.0001 && fabs(tag.origin[1]-4)<0.0001);
 overlay.qceOverlayWeight=0.5f;
 assert(R_IQMLerpTagRef(&tag,&data,0,1,0.25f,"tag_flash",&overlay));assert(fabs(tag.origin[1]-2)<0.0001);
 overlay.qceOverlayFrame=999;assert(R_IQMLerpTagRef(&tag,&data,0,1,0.25f,"tag_flash",&overlay));assert(fabs(tag.origin[1])<0.0001);
 overlay.qceOverlayFrame=overlay.qceOverlayOldFrame=2;overlay.qceOverlayWeight=1;poses[4].rotate[2]=poses[4].rotate[3]=sqrtf(0.5f);
 assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"tag_flash",&overlay));assert(fabs(tag.origin[0])<0.0001 && fabs(tag.origin[1]-6)<0.0001);
 memset(&overlay,0,sizeof(overlay));overlay.qceAimGrid=1;
 for(i=0;i<4;i++)overlay.qceAimFrames[i]=i;
 for(i=0;i<8;i++){poses[i].rotate[0]=poses[i].rotate[1]=poses[i].rotate[2]=0;poses[i].rotate[3]=1;VectorClear(poses[i].translate);}
 poses[2].translate[0]=10;poses[4].translate[1]=20;poses[6].translate[0]=10;poses[6].translate[1]=20;
 for(i=0;i<4;i++)poses[i*2+1].translate[0]=4;
 overlay.qceAimYaw=.25f;overlay.qceAimPitch=.5f;
 assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"tag_flash",&overlay));assert(fabs(tag.origin[0]-6.5f)<.0001 && fabs(tag.origin[1]-10)<.0001);
 overlay.qceAimPitch=.501f;assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"tag_flash",&overlay));assert(fabs(tag.origin[1]-10.02f)<.0001);
 overlay.qceAimPitch=1;overlay.qceAimYaw=1;assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"tag_flash",&overlay));assert(fabs(tag.origin[0]-14)<.0001 && fabs(tag.origin[1]-20)<.0001);
 overlay.qceAimFrames[3]=999;assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"tag_flash",&overlay));assert(fabs(tag.origin[0]-4)<.0001 && fabs(tag.origin[1])<.0001);
 /* Upper-body action pose must coexist with independent moving legs. */
 {
  iqmTransform_t layered[16];int hierarchy[4]={-1,0,0,2};float identity[48];
  memset(layered,0,sizeof(layered));memset(identity,0,sizeof(identity));
  for(i=0;i<16;i++){layered[i].rotate[3]=1;VectorSet(layered[i].scale,1,1,1);}
  for(i=0;i<4;i++)identity[i*12]=identity[i*12+5]=identity[i*12+10]=1;
  data.num_joints=data.num_poses=4;data.num_frames=4;data.poses=layered;
  data.jointNames="bip01 pelvis\0bip01 l thigh\0bip01 spine\0tag_hand";
  data.jointParents=hierarchy;data.bindJoints=data.invBindJoints=identity;
  layered[0].translate[0]=20;layered[1].translate[0]=20;layered[2].translate[0]=50;layered[3].translate[0]=4;
  layered[4].translate[0]=2;layered[5].translate[0]=5;
  layered[8].translate[0]=4;layered[9].translate[0]=7;
  memset(&overlay,0,sizeof(overlay));overlay.qceLowerBody=1;overlay.qceLowerOldFrame=overlay.qceLowerFrame=1;
  assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"bip01 l thigh",&overlay));assert(fabs(tag.origin[0]-7)<.0001);
  assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"tag_hand",&overlay));assert(fabs(tag.origin[0]-56)<.0001);
  overlay.qceLowerFrame=2;overlay.qceLowerBacklerp=.5f;
  assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"bip01 l thigh",&overlay));assert(fabs(tag.origin[0]-9)<.0001);
  overlay.qceLowerPreviousFrame=overlay.qceLowerPreviousOldFrame=1;overlay.qceLowerBlend=.5f;
  assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"bip01 l thigh",&overlay));assert(fabs(tag.origin[0]-8)<.0001);
  memset(&overlay,0,sizeof(overlay));overlay.qcePlayerAim=1;overlay.qcePlayerPitch=45;
  assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"tag_hand",&overlay));
  assert(tag.origin[2]<-1 && tag.origin[0]<74); /* pitched spine moves hands */
  assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"bip01 l thigh",&overlay));assert(fabs(tag.origin[0]-40)<.0001);
  memset(&overlay,0,sizeof(overlay));
  overlay.qceLowerBody=0;overlay.qceTransitionOldFrame=1;overlay.qceTransitionFrame=2;overlay.qceTransitionBacklerp=.5f;overlay.qceTransitionWeight=.5f;
  assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"bip01 l thigh",&overlay));assert(fabs(tag.origin[0]-24.5f)<.0001);
  overlay.qceTransitionFrame=999;
  assert(R_IQMLerpTagRef(&tag,&data,0,0,0,"bip01 l thigh",&overlay));assert(fabs(tag.origin[0]-40)<.0001);
 }
 puts("PASS: renderer IQM attachment endpoints, interpolation direction, child bind transforms and missing tags");return 0;
}
