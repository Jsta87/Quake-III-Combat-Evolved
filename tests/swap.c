#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];
level_locals_t level;
vmCvar_t g_gametype;
static gclient_t client;
static gentity_t drop;
static int drops,pickups,selects,wall,expectedWeapon=WP_SHOTGUN;
static int scoreTest,scoreSeen;
static gclient_t scoreClients[MAX_CLIENTS];
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int n,const char *fmt,...) {(void)n;(void)fmt;abort();}
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 (void)start;(void)mins;(void)maxs;(void)end;(void)pass;assert(mask==CONTENTS_SOLID);
 memset(tr,0,sizeof(*tr));tr->fraction=wall?0.5f:1.0f;
}
gentity_t *Drop_Item(gentity_t *ent,gitem_t *item,float angle) {
 (void)ent;(void)angle;memset(&drop,0,sizeof(drop));drop.item=item;drops++;return &drop;
}
void Touch_Item(gentity_t *item,gentity_t *other,trace_t *trace) {
 (void)trace;assert(BG_CanItemBeGrabbed(GT_FFA,&item->s,&other->client->ps));
 assert(BG_QceAddWeapon(&other->client->ps,item->item->giTag,10));pickups++;
 item->r.contents=0;
}
void trap_SendServerCommand(int n,const char *text) {
 if(scoreTest) {
  int offset,count,red,blue,pos,i,j;char *next;
  assert(strlen(text)<MAX_STRING_CHARS);assert(sscanf(text,"scores_chunk %d %d %d %d%n",&offset,&count,&red,&blue,&pos)==4);
  assert(offset==scoreSeen && count>=0 && count<=4);next=(char*)text+pos;
  for(i=0;i<count;i++)for(j=0;j<14;j++){long value=strtol(next,&next,10);if(j==0)assert(value==scoreSeen+i);}
  assert(!*next);scoreSeen+=count;return;
 }
 assert(n==0 && !strcmp(text,va("qce_select %d %d",expectedWeapon,client.ps.commandTime)));selects++;
}

static void setup(void) {
 gentity_t *ent=&g_entities[0],*item=&g_entities[MAX_CLIENTS];
 memset(g_entities,0,sizeof(g_entities));memset(&client,0,sizeof(client));memset(&level,0,sizeof(level));
 drops=pickups=selects=wall=0;expectedWeapon=WP_SHOTGUN;ent->client=&client;ent->health=100;
 client.ps.stats[STAT_QCE_COMBAT]=1;client.ps.stats[STAT_HEALTH]=100;client.ps.weapon=WP_MACHINEGUN;
 BG_QceAddWeapon(&client.ps,WP_MACHINEGUN,45);BG_QceAddWeapon(&client.ps,WP_ROCKET_LAUNCHER,4);
 client.ps.stats[STAT_QCE_MAG0]=12;client.ps.commandTime=1000;level.time=1000;level.num_entities=MAX_CLIENTS+1;
 item->inuse=qtrue;item->r.linked=qtrue;item->r.contents=CONTENTS_TRIGGER;item->s.eType=ET_ITEM;
 item->item=BG_FindItemForWeapon(WP_SHOTGUN);item->s.modelindex=item->item-bg_itemlist;
 VectorSet(item->r.currentOrigin,40,0,0);
}
static void score_tests(void) {
 int cases[]={0,1,4,5,128},i,j;
 setup();level.clients=scoreClients;scoreTest=1;
 for(i=0;i<5;i++) {
  level.numConnectedClients=cases[i];scoreSeen=0;
  for(j=0;j<cases[i];j++){level.sortedClients[j]=j;scoreClients[j].ps.persistant[PERS_SCORE]=-2147483647;scoreClients[j].pers.connected=CON_CONNECTED;}
  DeathmatchScoreboardMessage(&g_entities[0]);assert(scoreSeen==cases[i]);
 }
 scoreTest=0;
}
int main(void) {
 score_tests();
 setup();client.ps.weaponstate=WEAPON_RELOADING;client.ps.weaponTime=2000;G_QceSwapWeapon(&g_entities[0]);assert(pickups==1 && drops==1 && drop.qceDroppedMagazine==13);
 setup();BG_QceReload(&client.ps);client.ps.weaponstate=WEAPON_RELOADING;client.ps.weaponTime=2000;client.ps.qceReloadCommit=-1;G_QceSwapWeapon(&g_entities[0]);assert(drop.qceDroppedMagazine==46 && drop.count==45);
 setup();client.ps.weaponstate=WEAPON_RELOADING;client.ps.weaponTime=2000;wall=1;G_QceSwapWeapon(&g_entities[0]);assert(!pickups && client.ps.weaponstate==WEAPON_RELOADING);

 setup();G_QceSwapWeapon(&g_entities[0]);
 assert(drops==1 && pickups==1 && selects==1 && client.ps.weapon==WP_SHOTGUN);
 assert(client.qcePickupLatched && (client.ps.stats[STAT_QCE_GRENADES]&QCE_PICKUP_DRAW));
 assert(client.ps.eventSequence==0); /* replacement must not trigger empty-ammo selection */
 assert(drop.count==45 && drop.qceDroppedMagazine==13 && drop.s.time==3000);
 assert(BG_QceSlot(&client.ps,WP_MACHINEGUN)<0 && BG_QceMagazine(&client.ps,WP_SHOTGUN)==10);
 assert(BG_QceSlot(&client.ps,WP_ROCKET_LAUNCHER)>=0 && client.ps.ammo[WP_ROCKET_LAUNCHER]==4);
 setup();client.pers.cmd.weapon=WP_ROCKET_LAUNCHER;G_QceSwapWeapon(&g_entities[0]);assert(!drops && !pickups);
 client.ps.weapon=WP_ROCKET_LAUNCHER;client.ps.weaponstate=WEAPON_RAISING;client.ps.weaponTime=700;G_QceSwapWeapon(&g_entities[0]);
 assert(drops==1 && pickups==1 && BG_QceSlot(&client.ps,WP_MACHINEGUN)>=0 && BG_QceSlot(&client.ps,WP_ROCKET_LAUNCHER)<0);
 setup();wall=1;G_QceSwapWeapon(&g_entities[0]);assert(!drops && !pickups && !client.qcePickupLatched);
 setup();g_entities[MAX_CLIENTS].r.currentOrigin[0]=70;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();g_entities[MAX_CLIENTS].r.currentOrigin[0]=-40;G_QceSwapWeapon(&g_entities[0]);assert(pickups==1);
 setup();client.ps.weaponTime=1;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();g_entities[MAX_CLIENTS].s.time=2000;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();g_entities[0].health=0;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();BG_QceRemoveWeapon(&client.ps,WP_ROCKET_LAUNCHER);G_QceSwapWeapon(&g_entities[0]);assert(!drops && pickups==1);
 setup();client.ps.weapon=WP_GAUNTLET;G_QceSwapWeapon(&g_entities[0]);assert(!drops);

 setup();BG_QceRemoveWeapon(&client.ps,WP_MACHINEGUN);BG_QceAddWeapon(&client.ps,WP_PLASMAGUN,100);client.ps.weapon=WP_PLASMAGUN;
 g_entities[MAX_CLIENTS].item=BG_FindItemForWeapon(WP_PLASMAGUN);g_entities[MAX_CLIENTS].s.modelindex=g_entities[MAX_CLIENTS].item-bg_itemlist;
 assert(!BG_CanItemBeGrabbed(GT_FFA,&g_entities[MAX_CLIENTS].s,&client.ps));
 client.ps.qceError[0]=7000;client.ps.qceErrorRemainder[0]=-432;client.ps.qceHeat[0]=10000;client.ps.qceHeatRemainder[0]=321;client.ps.qceOverheated=1;
 client.ps.qceBattery[0]=499123;client.ps.qceRate[0]=5678;client.ps.qceRateRemainder[0]=-321;client.ps.qceOverheatTime[0]=1234;
 expectedWeapon=WP_PLASMAGUN;G_QceSwapWeapon(&g_entities[0]);
 assert(drops==1 && pickups==1 && client.ps.weapon==WP_PLASMAGUN && drop.count==100 && client.ps.ammo[WP_PLASMAGUN]==10);
 assert(drop.qceDroppedBattery==499123 && drop.qceDroppedRate==5678 && drop.qceDroppedRateRemainder==-321 && drop.qceDroppedOverheatTime==1234);
 assert(drop.qceDroppedError==7000 && drop.qceDroppedErrorRemainder==-432);
 assert(drop.qceDroppedHeat==10000 && drop.qceDroppedHeatRemainder==321 && drop.qceDroppedOverheated==1 && drop.qceDroppedHeatTime==level.time);
 puts("PASS: nearby weapon replacement, loaded/total ammo preservation, select feedback, free slot, any direction/range/LOS, owner lock and action guards");return 0;
}
