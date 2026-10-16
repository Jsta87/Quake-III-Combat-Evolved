#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
void QDECL Com_Error(int code,const char *format,...){abort();}
vmCvar_t g_cheats;
gentity_t g_entities[MAX_GENTITIES];
gitem_t bg_itemlist[4];int bg_numItems=4;
static gclient_t client;static gentity_t player,created;
static int traces,spawned,blocked,noHit,sky,ceiling,lastWeapon;static float blockedBelow;static vec3_t normal={0,0,1},surface={1000,20,0},lastEye,lastEnd;
static qboolean CheatsOk(gentity_t *p){return g_cheats.integer && p->health>0;}
void trap_SendServerCommand(int client,const char *text){}
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask){
 memset(tr,0,sizeof(*tr));traces++;tr->fraction=1;VectorCopy(end,tr->endpos);
 if(!mins && Distance(start,end)>1000){VectorCopy(start,lastEye);VectorCopy(end,lastEnd);tr->fraction=noHit?1:.1f;VectorCopy(surface,tr->endpos);VectorCopy(normal,tr->plane.normal);tr->surfaceFlags=sky?SURF_SKY:0;}
 else if(!mins && ceiling){tr->fraction=.5f;}
 else if(mins){tr->startsolid=blocked || (blockedBelow && start[2]<blockedBelow);assert(mask==MASK_PLAYERSOLID);assert(pass==ENTITYNUM_NONE);}
}
gitem_t *BG_FindItemForWeapon(weapon_t weapon){lastWeapon=weapon;return &bg_itemlist[1];}
const qce_weapondef_t *BG_QceWeaponDef(int weapon){static qce_weapondef_t definition;definition.ammo_initial=120;return &definition;}
void RegisterItem(gitem_t *item){}
void SaveRegisteredItems(void){}
gentity_t *G_Spawn(void){memset(&created,0,sizeof(created));spawned++;return &created;}
gentity_t *LaunchItem(gitem_t *item,vec3_t origin,vec3_t velocity){gentity_t *e=G_Spawn();e->classname=item->classname;VectorCopy(origin,e->s.origin);return e;}
void SP_qce_warthog(gentity_t *e){e->qceVehicle=1;e->classname="qce_warthog";}
void SP_qce_ghost(gentity_t *e){e->qceVehicle=2;e->classname="qce_ghost";}
void SP_qce_scorpion(gentity_t *e){e->qceVehicle=3;e->classname="qce_scorpion";}
#include "../engine/code/game/g_qce_spawn.h"
int main(void){
 player.client=&client;player.health=100;player.client->ps.viewheight=26;VectorSet(player.client->ps.origin,4,5,6);
 bg_itemlist[1].classname="weapon_bfg";bg_itemlist[1].giType=IT_WEAPON;bg_itemlist[1].giTag=WP_BFG;
 bg_itemlist[2].classname="item_health_large";bg_itemlist[2].giType=IT_HEALTH;
 bg_itemlist[3].classname="team_CTF_redflag";bg_itemlist[3].giType=IT_TEAM;
 QceSpawnCommand(&player,"pistol");assert(!spawned && !traces);g_cheats.integer=1;
 QceSpawnCommand(&player,"help");QceSpawnCommand(&player,"nonsense");QceSpawnCommand(&player,"team_CTF_redflag");assert(!spawned && !traces);
 QceSpawnCommand(&player,"pistol");assert(spawned==1 && created.count==120 && created.s.origin[2]==ITEM_RADIUS+1);
 assert(lastEye[0]==4 && lastEye[1]==5 && lastEye[2]==32 && lastEnd[0]>65000);
 QceSpawnCommand(&player,"item item_health_large");assert(spawned==2 && !Q_stricmp(created.classname,"item_health_large"));
 QceSpawnCommand(&player,"vehicle ghost");assert(spawned==3 && created.qceVehicle==2 && created.s.origin[2]==13);
 QceSpawnCommand(&player,"scorpion");assert(spawned==4 && created.qceVehicle==3 && created.s.origin[2]==2);
 VectorSet(normal,-1,0,0);QceSpawnCommand(&player,"ghost");assert(fabs(created.s.origin[0]-923)<.01f); /* full muzzle-facing footprint */
 blocked=1;QceSpawnCommand(&player,"warthog");assert(spawned==5);blocked=0;
 noHit=1;QceSpawnCommand(&player,"pistol");assert(spawned==5);noHit=0;sky=1;QceSpawnCommand(&player,"pistol");assert(spawned==5);sky=0;
 player.client->ps.qceVehicle=42;QceSpawnCommand(&player,"pistol");assert(spawned==5);player.client->ps.qceVehicle=0;
 VectorSet(normal,0,0,1);blockedBelow=28;QceSpawnCommand(&player,"pistol");assert(spawned==6 && created.s.origin[2]>=28 && created.s.origin[2]<=32);blockedBelow=0;
 ceiling=1;QceSpawnCommand(&player,"pistol");assert(spawned==6);ceiling=0;
 player.client->sess.sessionTeam=TEAM_SPECTATOR;QceSpawnCommand(&player,"pistol");assert(spawned==6);player.client->sess.sessionTeam=TEAM_FREE;
 {vec3_t lo={-10,-20,-1},hi={10,20,30},outLo,outHi;QceSpawnRotatedBounds(lo,hi,90,outLo,outHi);assert(fabs(outLo[0]+20)<.001f && fabs(outHi[1]-10)<.001f);}
 {int i;for(i=MAX_CLIENTS;i<ENTITYNUM_MAX_NORMAL;i++)g_entities[i].inuse=qtrue;QceSpawnCommand(&player,"pistol");assert(spawned==6);}
 memset(g_entities,0,sizeof(g_entities));
 {static const char *names[]={"pistol","assault rifle","shotgun","rocket","sniper","plasma rifle","weapon plasma pistol","needler"};static const int ids[]={WP_BFG,WP_MACHINEGUN,WP_SHOTGUN,WP_ROCKET_LAUNCHER,WP_RAILGUN,WP_PLASMAGUN,WP_LIGHTNING,WP_GRENADE_LAUNCHER};int i;for(i=0;i<8;i++){QceSpawnCommand(&player,names[i]);assert(lastWeapon==ids[i]);}}
 puts("PASS: spawn command cheat gate, aliases, item validation, aim ray, floor/wall offsets, vehicle footprint rotation, blocked/sky/empty targets and entity reserve");return 0;
}
