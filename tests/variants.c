/* Exercise real variant registration, validation, persistence, loadouts and regeneration. */
#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
level_locals_t level;
vmCvar_t g_qceCombat,g_friendlyFire,g_fraglimit,g_timelimit,g_gametype;
static char names[40][64],values[40][64],fileText[8192],path[96],arg[64],command[32];
static int count,argcValue=2,sets;
static int Find(const char *name) {int i;for(i=0;i<count;i++)if(!Q_stricmp(name,names[i]))return i;assert(count<40);Q_strncpyz(names[count],name,64);return count++;}
void trap_Cvar_Update(vmCvar_t *v) {assert(v->handle>0);Q_strncpyz(v->string,values[v->handle-1],sizeof(v->string));v->value=atof(v->string);v->integer=atoi(v->string);}
void trap_Cvar_Register(vmCvar_t *v,const char *name,const char *initial,int flags) {int i=Find(name);(void)flags;if(!values[i][0])Q_strncpyz(values[i],initial,64);v->handle=i+1;trap_Cvar_Update(v);}
void trap_Cvar_Set(const char *name,const char *value) {Q_strncpyz(values[Find(name)],value,64);sets++;}
int trap_Argc(void) {return argcValue;}
void trap_Argv(int n,char *out,int length) {Q_strncpyz(out,n?arg:command,length);}
int trap_FS_FOpenFile(const char *name,fileHandle_t *f,fsMode_t mode) {Q_strncpyz(path,name,sizeof(path));*f=1;if(mode==FS_WRITE){fileText[0]=0;return 0;}return strlen(fileText);}
void trap_FS_Read(void *out,int size,fileHandle_t f) {(void)f;memcpy(out,fileText,size);}
void trap_FS_Write(const void *in,int size,fileHandle_t f) {(void)f;assert(size<sizeof(fileText));memcpy(fileText,in,size);fileText[size]=0;}
void trap_FS_FCloseFile(fileHandle_t f) {(void)f;}
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL G_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int code,const char *fmt,...) {(void)code;(void)fmt;abort();}
static void Set(const char *name,const char *value) {trap_Cvar_Set(name,value);G_QceVariantUpdate();}
static void Cmd(const char *cmd,const char *name) {Q_strncpyz(command,cmd,sizeof(command));Q_strncpyz(arg,name,sizeof(arg));assert(G_QceVariantCommand(command));}
int main(void) {
 playerState_t ps;entityState_t pickup;gentity_t ent;gclient_t cl;char saved[8192];int before,i;
 g_qceCombat.integer=1;G_QceVariantRegister();
 memset(&ps,0,sizeof(ps));ps.stats[STAT_QCE_COMBAT]=1;G_QceVariantSpawn(&ps);
 assert(ps.weapon==WP_MACHINEGUN && ps.qceMaxShield==75 && ps.stats[STAT_MAX_HEALTH]==75 && !(ps.stats[STAT_WEAPONS]&(1<<WP_GAUNTLET)));
 memset(&pickup,0,sizeof(pickup));pickup.modelindex=BG_FindItemForWeapon(WP_SHOTGUN)-bg_itemlist;
 assert(BG_CanItemBeGrabbed(GT_FFA,&pickup,&ps));Set("gv_weaponPickup","0");G_QceVariantPlayer(&ps);assert(!BG_CanItemBeGrabbed(GT_FFA,&pickup,&ps));Set("gv_weaponPickup","1");
 Set("gv_primaryWeapon","pistol");Set("gv_secondaryWeapon","assaultrifle");Set("gv_shieldMultiplier","2");Set("gv_healthMultiplier","2");Set("gv_movespeed","0.5");Set("gv_gravity","0");
 G_QceVariantSpawn(&ps);assert(ps.weapon==WP_BFG && BG_QceSlot(&ps,WP_MACHINEGUN)>=0 && ps.qceMaxShield==150 && ps.stats[STAT_MAX_HEALTH]==150);
 assert(ps.qceVariantScale[0]==501 && ps.qceVariantScale[2]==1);
 ps.ammo[WP_BFG]=0;ps.stats[STAT_QCE_MAG0]=0;Set("gv_infiniteAmmo","1");G_QceVariantPlayer(&ps);
 assert(ps.ammo[WP_BFG]==BG_QceWeaponDef(WP_BFG)->ammo_max && ps.stats[STAT_QCE_MAG0]==0 && BG_QceCanReload(&ps));Set("gv_infiniteAmmo","0");
 Cmd("gv_save","roundtrip.cfg");assert(!strcmp(path,"variants/roundtrip.cfg"));strcpy(saved,fileText);
 Set("gv_shieldMultiplier","0");Set("gv_primaryWeapon","none");Cmd("gv_load","roundtrip");assert(GV(GV_SHIELD)==2);G_QceVariantSpawn(&ps);assert(ps.weapon==WP_BFG);
 before=sets;strcpy(fileText,"qce_variant 1\ngv_movespeed 0.1\ngv_unknown 1\n");Cmd("gv_load","invalid");assert(sets==before && GV(GV_MOVE)==0.5f);
 strcpy(fileText,saved);strcat(fileText,"exec evil.cfg\n");Cmd("gv_load","invalid");assert(sets==before);
 strcpy(fileText,saved);Cmd("gv_save","../escape");assert(!strcmp(fileText,saved));
 strcpy(fileText,saved);{char *line=strstr(fileText,"gv_maxHeldWeapons "),*end;assert(line);end=strchr(line,'\n');assert(end);memmove(line,end+1,strlen(end+1)+1);}Set("gv_maxHeldWeapons","0");Cmd("gv_load","legacy");assert(GV(GV_MAX_HELD)==2);
 Set("gv_movespeed","NaN");assert(GV(GV_MOVE)==1);Set("gv_fragGrenades","4.5");assert(GV(GV_FRAGS)==2);
 Set("gv_mapWeaponSet","none");assert(!G_QceVariantMapWeapon(BG_FindItemForWeapon(WP_ROCKET_LAUNCHER)));
 Set("gv_mapWeaponSet","shotguns");assert(G_QceVariantMapWeapon(BG_FindItemForWeapon(WP_BFG))->giTag==WP_SHOTGUN);
 Set("gv_maxHeldWeapons","0");G_QceVariantSpawn(&ps);assert(ps.qceMaxHeldWeapons==9);Set("gv_maxHeldWeapons","1");G_QceVariantSpawn(&ps);assert(BG_QceSlot(&ps,WP_MACHINEGUN)<0);Set("gv_maxHeldWeapons","2");
 Set("gv_primaryWeapon","random");Set("gv_secondaryWeapon","random");for(i=0;i<100;i++) {memset(&ps,0,sizeof(ps));ps.stats[STAT_QCE_COMBAT]=1;G_QceVariantSpawn(&ps);assert((ps.stats[STAT_QCE_SLOTS]&15)!=((ps.stats[STAT_QCE_SLOTS]>>4)&15));}
 memset(&ent,0,sizeof(ent));memset(&cl,0,sizeof(cl));ent.client=&cl;ent.health=75;cl.ps.qceMaxShield=150;level.time=1000;G_QceVariantRecharge(&ent);assert(cl.ps.stats[STAT_QCE_SHIELD]>0 && cl.ps.stats[STAT_QCE_SHIELD]<150);
 level.time=10000;G_QceVariantRecharge(&ent);assert(cl.ps.stats[STAT_QCE_SHIELD]==150);
 Set("gv_shieldRechargeRate","0");cl.ps.stats[STAT_QCE_SHIELD]=0;level.time=20000;G_QceVariantRecharge(&ent);assert(!cl.ps.stats[STAT_QCE_SHIELD]);
 puts("PASS: variant defaults, scaled loadouts/vitality/movement, unique random loadouts, map weapon sets, shield recharge, save/load roundtrip and atomic malformed-file rejection");return 0;
}
