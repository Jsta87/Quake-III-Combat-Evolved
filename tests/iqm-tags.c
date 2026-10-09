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
 puts("PASS: renderer IQM attachment endpoints, interpolation direction, child bind transforms and missing tags");return 0;
}
