#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
level_locals_t level;
vmCvar_t g_gametype,g_weaponRespawn,g_weaponTeamRespawn;
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int n,const char *fmt,...) {(void)n;(void)fmt;abort();}
int Pickup_Weapon(gentity_t *ent,gentity_t *other);
int main(void) {
 gentity_t player={0},drop={0};gclient_t client={0};int slot;
 player.client=&client;client.ps.stats[STAT_QCE_COMBAT]=1;g_weaponRespawn.integer=5;
 drop.item=BG_FindItemForWeapon(WP_PLASMAGUN);drop.flags=FL_DROPPED_ITEM;drop.count=100;drop.qceDroppedMagazine=101;
 drop.qceDroppedHeat=10000;drop.qceDroppedHeatRemainder=123;drop.qceDroppedOverheated=1;drop.qceDroppedHeatTime=1000;
 level.time=2000;assert(Pickup_Weapon(&drop,&player)==5);slot=BG_QceSlot(&client.ps,WP_PLASMAGUN);
 assert(client.ps.qceHeat[slot]==7300 && client.ps.qceHeatRemainder[slot]==123 && BG_QceOverheated(&client.ps,WP_PLASMAGUN));
 assert(client.ps.ammo[WP_PLASMAGUN]==100 && BG_QceMagazine(&client.ps,WP_PLASMAGUN)==100);
 BG_QceRemoveWeapon(&client.ps,WP_PLASMAGUN);level.time=4000;Pickup_Weapon(&drop,&player);assert(client.ps.qceHeat[0]==1900 && !BG_QceOverheated(&client.ps,WP_PLASMAGUN));
 BG_QceRemoveWeapon(&client.ps,WP_PLASMAGUN);level.time=1000000;Pickup_Weapon(&drop,&player);assert(!client.ps.qceHeat[0] && !client.ps.qceHeatRemainder[0]);
 BG_QceRemoveWeapon(&client.ps,WP_PLASMAGUN);memset(&drop,0,sizeof(drop));drop.item=BG_FindItemForWeapon(WP_PLASMAGUN);
 Pickup_Weapon(&drop,&player);assert(client.ps.ammo[WP_PLASMAGUN]==200 && !client.ps.qceHeat[0]);
 puts("PASS: actual dropped-weapon pickup preserves heat/lock, cools for elapsed world time with battery-age penalty, and fresh map weapons start cold");return 0;
}
