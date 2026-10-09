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
const char *NET_AdrToString(netadr_t address) {(void)address;return "test";}
static void fragment_test(void) {
 static byte packet[MAX_MSGLEN],payload[60000];static netchan_t channel;
 msg_t msg;int offset,count,i;static cvar_t quiet;
 extern cvar_t *showpackets,*showdrop;
 showpackets=showdrop=&quiet;channel.sock=NS_CLIENT;channel.challenge=1234;
 for(i=0;i<sizeof(payload);i++)payload[i]=(byte)(i*37);
 for(offset=0;offset<sizeof(payload);offset+=1300) {
  count=sizeof(payload)-offset;if(count>1300)count=1300;
  MSG_InitOOB(&msg,packet,sizeof(packet));MSG_WriteLong(&msg,1u<<31|1);
  MSG_WriteLong(&msg,NETCHAN_GENCHECKSUM(channel.challenge,1));MSG_WriteShort(&msg,offset);MSG_WriteShort(&msg,count);MSG_WriteData(&msg,payload+offset,count);
  if(offset+count==sizeof(payload)) {
   assert(Netchan_Process(&channel,&msg));assert(msg.cursize==sizeof(payload)+4);assert(!memcmp(msg.data+4,payload,sizeof(payload)));
  } else assert(!Netchan_Process(&channel,&msg));
 }
 assert(channel.incomingSequence==1);
}
int main(void) {
 byte buffer[16384];msg_t msg;playerState_t base={0},state={0},decoded={0};int i;
 fragment_test();
 assert(PROTOCOL_VERSION==99);
 assert(MAX_CLIENTS==128);assert(MAX_RELIABLE_COMMANDS==512);assert(MAX_GENTITIES==4096);state.qceVariantScale[0]=4001;state.qceVariantScale[1]=501;state.qceVariantScale[2]=1;state.qceVariantFlags=15;state.qceMaxShield=600;state.clientNum=127;state.qceReloadCommit=-1;state.qceReloadEmpty=1;
 state.commandTime=12345;state.weapon=8;state.qceHeat[0]=10000;state.qceHeat[1]=2500;
 state.qceHeatRemainder[0]=999;state.qceHeatRemainder[1]=1;state.qceError[0]=10000;state.qceError[1]=3456;state.qceErrorRemainder[0]=-999;state.qceErrorRemainder[1]=998;state.qceOverheated=3;state.qceChargeMs=600;
 state.qceRate[0]=10000;state.qceRate[1]=3456;state.qceRateRemainder[0]=-999;state.qceRateRemainder[1]=998;state.qceBattery[0]=1000000;state.qceBattery[1]=333333;state.qceZoom=6;state.qceCrouch=10000;state.qceOverheatTime[0]=1933;state.qceOverheatTime[1]=1133;
 state.qceMaxHeldWeapons=9;state.qceVehicle=4094;state.qceVehicleSeat=2;
 for(i=2;i<8;i++) {state.qceHeat[i]=10000-i;state.qceHeatRemainder[i]=999;state.qceError[i]=4567;state.qceErrorRemainder[i]=-777;state.qceRate[i]=10000;state.qceRateRemainder[i]=-888;state.qceBattery[i]=1000000;state.qceOverheatTime[i]=5000;state.qceExtraSlots[i-2]=i+2;state.qceExtraMags[i-2]=123;}state.qceOverheated=255;
 state.stats[STAT_QCE_COMBAT]=3;state.stats[STAT_QCE_MOVEMENT]=110;state.stats[5]=75;state.ammo[8]=197;
 for(i=0;i<3;i++) {
  state.weaponstate=WEAPON_RELOAD_ENTER+i;
  MSG_Init(&msg,buffer,sizeof(buffer));MSG_Bitstream(&msg);MSG_WriteDeltaPlayerstate(&msg,&base,&state);assert(!msg.overflowed);
  MSG_BeginReading(&msg);MSG_ReadDeltaPlayerstate(&msg,&base,&decoded);if(memcmp(&state,&decoded,sizeof(state))) {int n;for(n=0;n<sizeof(state)/sizeof(int);n++)if(((int*)&state)[n]!=((int*)&decoded)[n])fprintf(stderr,"offset %d expected %d decoded %d\n",n*4,((int*)&state)[n],((int*)&decoded)[n]);abort();}
  base=state;
  if(i==0){state.qceHeat[0]=2499;state.qceHeatRemainder[0]=0;state.qceOverheated=2;state.qceChargeMs=123;}
  if(i==1){state.qceReloadCommit=state.qceReloadEmpty=0;state.qceRate[0]=state.qceRate[1]=state.qceRateRemainder[0]=state.qceRateRemainder[1]=state.qceBattery[0]=state.qceBattery[1]=state.qceZoom=state.qceCrouch=state.qceOverheatTime[0]=state.qceOverheatTime[1]=0;state.qceHeat[0]=state.qceHeat[1]=state.qceHeatRemainder[0]=state.qceHeatRemainder[1]=state.qceError[0]=state.qceError[1]=state.qceErrorRemainder[0]=state.qceErrorRemainder[1]=0;state.qceOverheated=0;state.qceChargeMs=0;}
 }
 {
  entityState_t from={0},to={0},read={0};int number;
  to.number=3000;to.otherEntityNum=3500;to.groundEntityNum=ENTITYNUM_WORLD;to.eType=ET_EVENTS+EV_SHOTGUN;to.generic1=WP_SHOTGUN;to.time2=127;
  MSG_Init(&msg,buffer,sizeof(buffer));MSG_Bitstream(&msg);MSG_WriteDeltaEntity(&msg,&from,&to,qtrue);
  MSG_BeginReading(&msg);number=MSG_ReadBits(&msg,GENTITYNUM_BITS);MSG_ReadDeltaEntity(&msg,&from,&read,number);
  assert(!memcmp(&to,&read,sizeof(to)));
 }
 {
  usercmd_t from={0},to={0},read={0};to.serverTime=123;to.weapon=WP_RAILGUN;to.buttons=BUTTON_ATTACK|BUTTON_QCE_ZOOM|BUTTON_QCE_PICKUP;
  MSG_Init(&msg,buffer,sizeof(buffer));MSG_Bitstream(&msg);MSG_WriteDeltaUsercmdKey(&msg,456,&from,&to);
  MSG_BeginReading(&msg);MSG_ReadDeltaUsercmdKey(&msg,456,&from,&read);assert(!memcmp(&to,&read,sizeof(to)));
 }
 puts("PASS: protocol-99 player-state heat/charge/spread/rate/battery/zoom/crouch/recovery/reload-phase delta serialization, recovery transitions and zero/reset state");return 0;
}
