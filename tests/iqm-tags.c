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
 iqmTransform_t poses[4];
 int parents[2]={-1,0},i;
 float bind[24]={1,0,0,0,0,1,0,0,0,0,1,0, 1,0,0,4,0,1,0,0,0,0,1,0};
 float inverse[24];
 memset(&data,0,sizeof(data));memset(poses,0,sizeof(poses));memcpy(inverse,bind,sizeof(bind));inverse[15]=-4;
 for(i=0;i<4;i++) {poses[i].rotate[3]=1;VectorSet(poses[i].scale,1,1,1);}
 poses[1].translate[0]=poses[3].translate[0]=4;poses[2].translate[0]=10;
 data.num_joints=data.num_poses=2;data.num_frames=2;data.jointNames="root\0tag_flash";
 data.jointParents=parents;data.bindJoints=bind;data.invBindJoints=inverse;data.poses=poses;
 assert(R_IQMLerpTag(&tag,&data,0,1,0,"tag_flash"));assert(fabs(tag.origin[0]-4)<0.0001);
 assert(R_IQMLerpTag(&tag,&data,0,1,1,"tag_flash"));assert(fabs(tag.origin[0]-14)<0.0001);
 assert(R_IQMLerpTag(&tag,&data,0,1,0.25f,"tag_flash"));assert(fabs(tag.origin[0]-6.5f)<0.0001);
 assert(R_IQMLerpTag(&tag,&data,1,0,0.25f,"tag_flash"));assert(fabs(tag.origin[0]-11.5f)<0.0001);
 assert(!R_IQMLerpTag(&tag,&data,0,1,0,"missing"));
 puts("PASS: renderer IQM attachment endpoints, interpolation direction, child bind transforms and missing tags");return 0;
}
