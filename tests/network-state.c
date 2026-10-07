#include "q_shared.h"
#include "qcommon.h"
#include "../engine/code/game/bg_public.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static cvar_t shownet;
cvar_t *cl_shownet=&shownet;
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int code,const char *fmt,...) {(void)code;(void)fmt;abort();}
int main(void) {
 byte buffer[16384];msg_t msg;playerState_t base={0},state={0},decoded={0};int i;
 assert(PROTOCOL_VERSION==93);
 state.commandTime=12345;state.weapon=8;state.qceHeat[0]=10000;state.qceHeat[1]=2500;
 state.qceHeatRemainder[0]=999;state.qceHeatRemainder[1]=1;state.qceError[0]=10000;state.qceError[1]=3456;state.qceErrorRemainder[0]=-999;state.qceErrorRemainder[1]=998;state.qceOverheated=3;state.qceChargeMs=600;
 state.qceRate[0]=10000;state.qceRate[1]=3456;state.qceRateRemainder[0]=-999;state.qceRateRemainder[1]=998;state.qceBattery[0]=1000000;state.qceBattery[1]=333333;state.qceZoom=6;state.qceCrouch=10000;state.qceOverheatTime[0]=1933;state.qceOverheatTime[1]=1133;
 state.stats[5]=75;state.ammo[8]=197;
 for(i=0;i<3;i++) {
  MSG_Init(&msg,buffer,sizeof(buffer));MSG_Bitstream(&msg);MSG_WriteDeltaPlayerstate(&msg,&base,&state);assert(!msg.overflowed);
  MSG_BeginReading(&msg);MSG_ReadDeltaPlayerstate(&msg,&base,&decoded);if(memcmp(&state,&decoded,sizeof(state))) {int n;for(n=0;n<sizeof(state)/sizeof(int);n++)if(((int*)&state)[n]!=((int*)&decoded)[n])fprintf(stderr,"offset %d expected %d decoded %d\n",n*4,((int*)&state)[n],((int*)&decoded)[n]);abort();}
  base=state;
  if(i==0){state.qceHeat[0]=2499;state.qceHeatRemainder[0]=0;state.qceOverheated=2;state.qceChargeMs=123;}
  if(i==1){state.qceRate[0]=state.qceRate[1]=state.qceRateRemainder[0]=state.qceRateRemainder[1]=state.qceBattery[0]=state.qceBattery[1]=state.qceZoom=state.qceCrouch=state.qceOverheatTime[0]=state.qceOverheatTime[1]=0;state.qceHeat[0]=state.qceHeat[1]=state.qceHeatRemainder[0]=state.qceHeatRemainder[1]=state.qceError[0]=state.qceError[1]=state.qceErrorRemainder[0]=state.qceErrorRemainder[1]=0;state.qceOverheated=0;state.qceChargeMs=0;}
 }
 {
  entityState_t from={0},to={0},read={0};int number;
  to.number=42;to.eType=ET_EVENTS+EV_SHOTGUN;to.generic1=WP_SHOTGUN;to.time2=127;
  MSG_Init(&msg,buffer,sizeof(buffer));MSG_Bitstream(&msg);MSG_WriteDeltaEntity(&msg,&from,&to,qtrue);
  MSG_BeginReading(&msg);number=MSG_ReadBits(&msg,GENTITYNUM_BITS);MSG_ReadDeltaEntity(&msg,&from,&read,number);
  assert(!memcmp(&to,&read,sizeof(to)));
 }
 {
  usercmd_t from={0},to={0},read={0};to.serverTime=123;to.weapon=WP_RAILGUN;to.buttons=BUTTON_ATTACK|BUTTON_QCE_ZOOM;
  MSG_Init(&msg,buffer,sizeof(buffer));MSG_Bitstream(&msg);MSG_WriteDeltaUsercmdKey(&msg,456,&from,&to);
  MSG_BeginReading(&msg);MSG_ReadDeltaUsercmdKey(&msg,456,&from,&read);assert(!memcmp(&to,&read,sizeof(to)));
 }
 puts("PASS: protocol-93 player-state heat/charge/spread/rate/battery/zoom/crouch/recovery delta serialization, recovery transitions and zero/reset state");return 0;
}
