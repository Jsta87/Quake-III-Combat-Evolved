#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];
level_locals_t level;
vmCvar_t g_gametype;
static gclient_t client;
static gentity_t drop;
static int drops,pickups,selects,wall;
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
void trap_SendServerCommand(int n,const char *text) {assert(n==0 && !strcmp(text,"qce_select 3"));selects++;}
static void setup(void) {
 gentity_t *ent=&g_entities[0],*item=&g_entities[MAX_CLIENTS];
 memset(g_entities,0,sizeof(g_entities));memset(&client,0,sizeof(client));memset(&level,0,sizeof(level));
 drops=pickups=selects=wall=0;ent->client=&client;ent->health=100;
 client.ps.stats[STAT_QCE_COMBAT]=1;client.ps.stats[STAT_HEALTH]=100;client.ps.weapon=WP_MACHINEGUN;
 BG_QceAddWeapon(&client.ps,WP_MACHINEGUN,45);BG_QceAddWeapon(&client.ps,WP_ROCKET_LAUNCHER,4);
 client.ps.stats[STAT_QCE_MAG0]=12;client.ps.commandTime=1000;level.time=1000;level.num_entities=MAX_CLIENTS+1;
 item->inuse=qtrue;item->r.linked=qtrue;item->r.contents=CONTENTS_TRIGGER;item->s.eType=ET_ITEM;
 item->item=BG_FindItemForWeapon(WP_SHOTGUN);item->s.modelindex=item->item-bg_itemlist;
 VectorSet(item->r.currentOrigin,40,0,0);
}
int main(void) {
 setup();G_QceSwapWeapon(&g_entities[0]);
 assert(drops==1 && pickups==1 && selects==1 && client.ps.weapon==WP_SHOTGUN);
 assert(client.ps.eventSequence==0); /* replacement must not trigger empty-ammo selection */
 assert(drop.count==45 && drop.qceDroppedMagazine==13 && drop.s.time==3000);
 assert(BG_QceSlot(&client.ps,WP_MACHINEGUN)<0 && BG_QceMagazine(&client.ps,WP_SHOTGUN)==8);
 assert(BG_QceSlot(&client.ps,WP_ROCKET_LAUNCHER)>=0 && client.ps.ammo[WP_ROCKET_LAUNCHER]==4);
 setup();wall=1;G_QceSwapWeapon(&g_entities[0]);assert(!drops && !pickups);
 setup();g_entities[MAX_CLIENTS].r.currentOrigin[0]=70;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();g_entities[MAX_CLIENTS].r.currentOrigin[0]=-40;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();client.ps.weaponTime=1;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();g_entities[MAX_CLIENTS].s.time=2000;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();g_entities[0].health=0;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 setup();BG_QceRemoveWeapon(&client.ps,WP_ROCKET_LAUNCHER);G_QceSwapWeapon(&g_entities[0]);assert(!drops && pickups==1);
 setup();client.ps.weapon=WP_GAUNTLET;G_QceSwapWeapon(&g_entities[0]);assert(!drops);
 puts("PASS: nearby weapon replacement, loaded/total ammo preservation, select feedback, free slot, facing/range/LOS, owner lock and action guards");return 0;
}
