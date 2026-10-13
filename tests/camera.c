#define QCE_WEAPON_TEST
#include "movement.c"
static int cameraBlocked;static float targetX=1000;static vec3_t aimPoint;
static void cameraTrace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask){
 trace(tr,start,mins,maxs,end,pass,mask);
 if(mask==MASK_SOLID && mins && mins[0]==-4){tr->fraction=cameraBlocked?.5f:1;{int i;for(i=0;i<3;i++)tr->endpos[i]=start[i]+tr->fraction*(end[i]-start[i]);}}
 if(mask==MASK_SHOT && !mins){float t=(targetX-start[0])/(end[0]-start[0]);tr->fraction=t;{int i;for(i=0;i<3;i++)tr->endpos[i]=start[i]+t*(end[i]-start[i]);}VectorCopy(tr->endpos,aimPoint);}
}
static void aimStep(playerState_t *ps,int side){
 pmove_t move;memset(&move,0,sizeof(move));move.ps=ps;move.trace=cameraTrace;move.pointcontents=contents;move.tracemask=MASK_PLAYERSOLID;
 move.cmd.serverTime=ps->commandTime+8;move.cmd.qceCameraMode=1;move.cmd.qceCameraBack=90;move.cmd.qceCameraUp=8;move.cmd.qceCameraSide=side;Pmove(&move);
}
int main(void){
 playerState_t ps,predicted;vec3_t eye,forward,delta;
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;predicted=ps;aimStep(&ps,24);aimStep(&predicted,24);assert(!memcmp(&ps,&predicted,sizeof(ps)));
 VectorCopy(ps.origin,eye);eye[2]+=ps.viewheight;VectorSubtract(aimPoint,eye,delta);VectorNormalize(delta);AngleVectors(ps.viewangles,forward,NULL,NULL);
 assert(DotProduct(delta,forward)>.999999f && ps.qceCameraAngles[YAW]==0 && ps.qceCameraAngles[PITCH]==0);
 assert(aimPoint[1]<-23.9f);aimStep(&ps,-24);assert(aimPoint[1]>23.9f);cameraBlocked=1;aimStep(&ps,-24);assert(fabs(aimPoint[1]-12)<.01f);
 cameraBlocked=0;aimStep(&ps,10000);assert(fabs(aimPoint[1]+200)<.01f); /* Both sides clamp hostile offsets. */
 targetX=20;{int i;for(i=0;i<20;i++){aimStep(&ps,24);VectorCopy(ps.origin,eye);eye[2]+=ps.viewheight;VectorSubtract(aimPoint,eye,delta);VectorNormalize(delta);AngleVectors(ps.viewangles,forward,NULL,NULL);assert(DotProduct(delta,forward)>.999999f && ps.qceCameraAngles[YAW]==0);}}
 puts("PASS: shared shoulder aim, screen-center target alignment, raw camera separation, left/right shoulders, camera collision, bounded offsets and prediction parity");return 0;
}
