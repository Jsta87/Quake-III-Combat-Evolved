#include "q_shared.h"
#include "qcommon.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static cvar_t shownet;
cvar_t *cl_shownet=&shownet;
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int code,const char *fmt,...) {(void)code;(void)fmt;abort();}
int main(void) {
 byte buffer[16384];msg_t msg;playerState_t base={0},state={0},decoded={0};int i;
 assert(PROTOCOL_VERSION==91);
 state.commandTime=12345;state.weapon=8;state.qceHeat[0]=10000;state.qceHeat[1]=2500;
 state.qceHeatRemainder[0]=999;state.qceHeatRemainder[1]=1;state.qceOverheated=3;state.qceChargeMs=600;
 state.stats[5]=75;state.ammo[8]=197;
 for(i=0;i<3;i++) {
  MSG_Init(&msg,buffer,sizeof(buffer));MSG_Bitstream(&msg);MSG_WriteDeltaPlayerstate(&msg,&base,&state);assert(!msg.overflowed);
  MSG_BeginReading(&msg);MSG_ReadDeltaPlayerstate(&msg,&base,&decoded);assert(!memcmp(&state,&decoded,sizeof(state)));
  base=state;
  if(i==0){state.qceHeat[0]=2499;state.qceHeatRemainder[0]=0;state.qceOverheated=2;state.qceChargeMs=123;}
  if(i==1){state.qceHeat[0]=state.qceHeat[1]=state.qceHeatRemainder[0]=state.qceHeatRemainder[1]=state.qceOverheated=0;state.qceChargeMs=0;}
 }
 puts("PASS: protocol-91 player-state heat/lock delta serialization, recovery transitions and zero/reset state");return 0;
}
