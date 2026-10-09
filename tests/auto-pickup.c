/* Exercise the real free-slot pickup, not a mocked swap touch. */
#include "g_local.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
gentity_t g_entities[MAX_GENTITIES];level_locals_t level;
vmCvar_t g_gametype,g_weaponRespawn,g_weaponTeamRespawn;
float qceVariantValues[GV_COUNT];
static int selects;
void trap_SendServerCommand(int client,const char *text){assert(client==0 && !strcmp(text,"qce_select 3 0"));selects++;}
void QDECL Com_Printf(const char *fmt,...){(void)fmt;}
void QDECL Com_Error(int n,const char *fmt,...){(void)n;(void)fmt;abort();}
extern int Pickup_Weapon(gentity_t *item,gentity_t *player);
int main(void){
 gclient_t client={0};gentity_t item={0},*player=&g_entities[0];player->client=&client;
 GV(GV_PICKUP)=1;GV(GV_WEAPON_RESPAWN)=5;client.ps.stats[STAT_QCE_COMBAT]=1;
 BG_QceAddWeapon(&client.ps,WP_MACHINEGUN,120);client.ps.weapon=WP_MACHINEGUN;
 client.ps.weaponstate=WEAPON_READY;client.ps.qceChargeMs=400;client.ps.qceZoom=1;
 item.item=BG_FindItemForWeapon(WP_SHOTGUN);
 assert(Pickup_Weapon(&item,player)==5);
 assert(client.ps.weapon==WP_SHOTGUN && client.ps.weaponstate==WEAPON_RAISING);
 assert(client.ps.weaponTime==BG_QceWeaponDef(WP_SHOTGUN)->ready_ms);
 assert((client.ps.stats[STAT_QCE_GRENADES]&QCE_PICKUP_DRAW) && selects==1);
 assert(client.ps.qceChargeMs==0 && client.ps.qceZoom==0 && BG_QceSlot(&client.ps,WP_MACHINEGUN)>=0);
 client.ps.weapon=WP_MACHINEGUN;client.ps.weaponstate=WEAPON_READY;
 Pickup_Weapon(&item,player);assert(client.ps.weapon==WP_MACHINEGUN && client.ps.weaponstate==WEAPON_READY && selects==1);
 puts("PASS: automatic second-weapon draw, authoritative selection, action reset and same-type ammo pickup without selection");return 0;
}
