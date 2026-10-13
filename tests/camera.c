#include "../engine/code/qcommon/q_shared.h"
#include "../engine/code/cgame/cg_qce_camera.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 vec3_t eye={0,0,0},point={100,0,0},axis[3]={{1,0,0},{0,1,0},{0,0,1}};
 float x,y;
 assert(QCE_ProjectAim(point,eye,axis,90,90,&x,&y));assert(fabs(x-.5f)<.0001 && fabs(y-.5f)<.0001);
 eye[1]=-24;eye[2]=8;
 assert(QCE_ProjectAim(point,eye,axis,90,90,&x,&y));assert(fabs(x-.38f)<.0001 && fabs(y-.54f)<.0001);
 eye[1]=24;assert(QCE_ProjectAim(point,eye,axis,90,90,&x,&y));assert(fabs(x-.62f)<.0001);
 assert(QCE_ProjectAim(point,eye,axis,90,60,&x,&y));assert(y>.54f);
 point[0]=-100;assert(!QCE_ProjectAim(point,eye,axis,90,90,&x,&y));
 point[0]=100;point[1]=1000;assert(!QCE_ProjectAim(point,eye,axis,90,90,&x,&y));
 point[1]=0;assert(!QCE_ProjectAim(point,eye,axis,0,90,&x,&y));
 puts("PASS: first-person aim projection, shoulder parallax, height, FOV, behind-camera and offscreen rejection");
 return 0;
}
