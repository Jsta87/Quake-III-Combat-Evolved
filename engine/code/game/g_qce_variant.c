/* Halo-inspired variants. Files contain only validated variant values, never executable commands.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#include "g_local.h"
#include "g_qce_variant.h"
float qceVariantValues[GV_COUNT]={1,1,1,1,1,1,1,1,1,1,2,2,1,0,0,1,1,1.7f,0,5,0,20,0,0,2};
typedef struct { const char *name,*initial; float minimum,maximum; qboolean integer; vmCvar_t cvar; } variantCvar_t;
static variantCvar_t variants[GV_COUNT]={
 {"gv_movespeed","1",0,4,0},{"gv_jumpHeight","1",0,4,0},{"gv_gravity","1",0,4,0},
 {"gv_shieldMultiplier","1",0,8,0},{"gv_healthMultiplier","1",0,8,0},
 {"gv_damageMultiplier","1",0,8,0},{"gv_meleeMultiplier","1",0,8,0},{"gv_grenadeMultiplier","1",0,8,0},
 {"gv_shieldRechargeDelay","1",0,8,0},{"gv_shieldRechargeRate","1",0,8,0},
 {"gv_fragGrenades","2",0,4,1},{"gv_plasmaGrenades","2",0,4,1},{"gv_grenades","1",0,1,1},
 {"gv_infiniteAmmo","0",0,1,1},{"gv_infiniteGrenades","0",0,1,1},
 {"gv_weaponPickup","1",0,1,1},{"gv_weaponDrop","1",0,1,1},
 {"gv_respawnTime","1.7",0,60,0},{"gv_suicidePenalty","0",0,60,0},
 {"gv_weaponRespawn","5",1,120,1},{"gv_friendlyFire","0",0,1,1},
 {"gv_scoreLimit","20",0,10000,1},{"gv_timeLimit","0",0,1440,1},{"gv_invisibility","0",0,1,1},{"gv_maxHeldWeapons","2",0,8,1}
};
static const char *stringNames[]={"gv_primaryWeapon","gv_secondaryWeapon","gv_mapWeaponSet","gv_gameType"};
static const char *stringDefaults[]={"assaultrifle","none","default","slayer"};
static vmCvar_t stringCvars[4];
static const int allWeapons[]={WP_BFG,WP_MACHINEGUN,WP_SHOTGUN,WP_ROCKET_LAUNCHER,WP_RAILGUN,WP_PLASMAGUN,WP_LIGHTNING,WP_GRENADE_LAUNCHER};
static int Weapon(const char *name) {
 static const char *names[]={"pistol","assaultrifle","shotgun","rocketlauncher","sniperrifle","plasmarifle","plasmapistol","needler"};
 int i;
 if(!Q_stricmp(name,"none"))return WP_NONE;
 if(!Q_stricmp(name,"random"))return -1;
 if(!Q_stricmp(name,"rocket"))return WP_ROCKET_LAUNCHER;
 if(!Q_stricmp(name,"sniper"))return WP_RAILGUN;
 for(i=0;i<8;i++)if(!Q_stricmp(name,names[i]))return allWeapons[i];
 return -2;
}
static qboolean Number(const char *s,float *number) {
 const char *p=s;int digits=0,dots=0;
 if(*p=='+' || *p=='-')p++;
 for(;*p;p++) {if(*p=='.') {if(++dots>1)return qfalse;}else if(*p>='0' && *p<='9')digits++;else return qfalse;}
 if(!digits || strlen(s)>24)return qfalse;
 *number=atof(s);return qtrue;
}
static qboolean Valid(int index,const char *value) {
 float n;int i;
 static const char *sets[]={"default","none","random","pistols","shotguns","rifles","rockets","snipers","plasma","needlers"};
 if(index<GV_COUNT)return Number(value,&n) && n>=variants[index].minimum && n<=variants[index].maximum && (!variants[index].integer || n==(int)n);
 index-=GV_COUNT;
 if(index<2)return Weapon(value)!=-2;
 if(index==2) {for(i=0;i<ARRAY_LEN(sets);i++)if(!Q_stricmp(value,sets[i]))return qtrue;return qfalse;}
 return !Q_stricmp(value,"slayer") || !Q_stricmp(value,"team_slayer") || !Q_stricmp(value,"ctf");
}
static const char *Name(int index) {return index<GV_COUNT?variants[index].name:stringNames[index-GV_COUNT];}
void G_QceVariantRegister(void) {
 int i;
 for(i=0;i<GV_COUNT;i++)trap_Cvar_Register(&variants[i].cvar,variants[i].name,variants[i].initial,CVAR_ARCHIVE);
 for(i=0;i<4;i++)trap_Cvar_Register(&stringCvars[i],stringNames[i],stringDefaults[i],CVAR_ARCHIVE);
 G_QceVariantUpdate();
}
void G_QceVariantUpdate(void) {
 int i,type;
 float previousPickup=GV(GV_PICKUP);
 for(i=0;i<GV_COUNT;i++) {
  trap_Cvar_Update(&variants[i].cvar);
  if(!Valid(i,variants[i].cvar.string)) {
   G_Printf("Invalid %s=%s; using %s\n",variants[i].name,variants[i].cvar.string,variants[i].initial);
   trap_Cvar_Set(variants[i].name,variants[i].initial);trap_Cvar_Update(&variants[i].cvar);
  }
  qceVariantValues[i]=variants[i].cvar.value;
 }
 for(i=0;i<4;i++) {
  trap_Cvar_Update(&stringCvars[i]);
  if(!Valid(GV_COUNT+i,stringCvars[i].string)) {trap_Cvar_Set(stringNames[i],stringDefaults[i]);trap_Cvar_Update(&stringCvars[i]);}
 }
 if(!g_qceCombat.integer)return;
 if(previousPickup!=GV(GV_PICKUP))for(i=0;i<level.maxclients;i++) {
  if(!g_entities[i].client)continue;
  if(GV(GV_PICKUP))g_entities[i].client->ps.qceVariantFlags&=~8;
  else g_entities[i].client->ps.qceVariantFlags|=8;
  g_entities[i].client->qcePickupLatched=qfalse;
 }
 if(g_friendlyFire.integer!=(int)GV(GV_FRIENDLY_FIRE))trap_Cvar_Set("g_friendlyFire",va("%d",(int)GV(GV_FRIENDLY_FIRE)));
 if(g_fraglimit.integer!=(int)GV(GV_SCORE_LIMIT))trap_Cvar_Set("fraglimit",va("%d",(int)GV(GV_SCORE_LIMIT)));
 if(g_timelimit.value!=GV(GV_TIME_LIMIT))trap_Cvar_Set("timelimit",va("%g",GV(GV_TIME_LIMIT)));
 type=!Q_stricmp(stringCvars[3].string,"ctf")?GT_CTF:!Q_stricmp(stringCvars[3].string,"team_slayer")?GT_TEAM:GT_FFA;
 /* g_gametype is latched: changing a variant takes effect at the next map/restart. */
 if(g_gametype.integer!=type)trap_Cvar_Set("g_gametype",va("%d",type));
}
void G_QceVariantPlayer(playerState_t *ps) {
 int slot,weapon;
 ps->qceVariantScale[0]=(int)(GV(GV_MOVE)*1000+1.5f);
 ps->qceVariantScale[1]=(int)(GV(GV_JUMP)*1000+1.5f);
 ps->qceVariantScale[2]=(int)(GV(GV_GRAVITY)*1000+1.5f);
 ps->qceVariantFlags=((int)GV(GV_INFINITE_AMMO)) | (!GV(GV_GRENADES)?2:0) | (GV(GV_INFINITE_GRENADES)?4:0) | (!GV(GV_PICKUP)?8:0);
 ps->qceMaxShield=(int)(BG_QcePlayerDef()->shield*GV(GV_SHIELD)+0.5f);
 if(ps->stats[STAT_QCE_SHIELD]>ps->qceMaxShield)ps->stats[STAT_QCE_SHIELD]=ps->qceMaxShield;
 if(GV(GV_INFINITE_AMMO))for(slot=0;slot<8;slot++) {
  weapon=BG_QceHeldWeapon(ps,slot);
  if(!weapon)continue;
  ps->ammo[weapon]=BG_QceWeaponDef(weapon)->ammo_max;
  if(BG_QceWeaponDef(weapon)->battery_cost>0) {
   ps->qceBattery[slot]=1000000;BG_QceSetMagazine(ps,slot,BG_QceCapacity(weapon));
  }
 }
 if(GV(GV_INVISIBLE))ps->powerups[PW_INVIS]=level.time+1000;
}
void G_QceVariantSpawn(playerState_t *ps) {
 int i,weapon,primary=WP_NONE;
 ps->stats[STAT_WEAPONS]=ps->stats[STAT_QCE_SLOTS]=ps->stats[STAT_QCE_MAG0]=ps->stats[STAT_QCE_MAG1]=0;
 memset(ps->ammo,0,sizeof(ps->ammo));
 memset(ps->qceHeat,0,sizeof(ps->qceHeat));memset(ps->qceHeatRemainder,0,sizeof(ps->qceHeatRemainder));
 memset(ps->qceError,0,sizeof(ps->qceError));memset(ps->qceErrorRemainder,0,sizeof(ps->qceErrorRemainder));
 memset(ps->qceRate,0,sizeof(ps->qceRate));memset(ps->qceRateRemainder,0,sizeof(ps->qceRateRemainder));
 memset(ps->qceBattery,0,sizeof(ps->qceBattery));memset(ps->qceOverheatTime,0,sizeof(ps->qceOverheatTime));ps->qceOverheated=0;
 memset(ps->qceExtraSlots,0,sizeof(ps->qceExtraSlots));memset(ps->qceExtraMags,0,sizeof(ps->qceExtraMags));
 ps->qceMaxHeldWeapons=GV(GV_MAX_HELD)?(int)GV(GV_MAX_HELD):9;
 for(i=0;i<2;i++) {
  weapon=Weapon(stringCvars[i].string);
  if(weapon==-1) {do {weapon=allWeapons[rand()%8];}while(i && weapon==primary);}
  if(weapon>WP_NONE)BG_QceAddWeapon(ps,weapon,BG_QceWeaponDef(weapon)->ammo_initial);
  if(!i)primary=weapon;
 }
 ps->weapon=primary?primary:(ps->stats[STAT_QCE_SLOTS]&15);
 ps->stats[STAT_MAX_HEALTH]=(int)(BG_QcePlayerDef()->health*(GV(GV_HEALTH)>0?GV(GV_HEALTH):1)+0.5f);
 if(ps->stats[STAT_MAX_HEALTH]<1)ps->stats[STAT_MAX_HEALTH]=1;
 G_QceVariantPlayer(ps);ps->stats[STAT_QCE_SHIELD]=ps->qceMaxShield;
 ps->stats[STAT_QCE_GRENADES]=0;
 BG_QceSetGrenadeCount(ps,0,GV(GV_GRENADES)?(int)GV(GV_FRAGS):0);
 BG_QceSetGrenadeCount(ps,1,GV(GV_GRENADES)?(int)GV(GV_PLASMAS):0);
}
gitem_t *G_QceVariantMapWeapon(gitem_t *item) {
 const char *set=stringCvars[2].string;int weapon=WP_NONE;
 if(!g_qceCombat.integer || item->giType!=IT_WEAPON)return item;
 if(item->giTag==WP_GAUNTLET || !Q_stricmp(set,"none"))return NULL;
 if(!Q_stricmp(set,"default"))return item;
 if(!Q_stricmp(set,"random"))weapon=allWeapons[rand()%8];
 else if(!Q_stricmp(set,"pistols"))weapon=WP_BFG;
 else if(!Q_stricmp(set,"shotguns"))weapon=WP_SHOTGUN;
 else if(!Q_stricmp(set,"rifles"))weapon=rand()%2?WP_MACHINEGUN:WP_PLASMAGUN;
 else if(!Q_stricmp(set,"rockets"))weapon=WP_ROCKET_LAUNCHER;
 else if(!Q_stricmp(set,"snipers"))weapon=WP_RAILGUN;
 else if(!Q_stricmp(set,"plasma"))weapon=rand()%2?WP_PLASMAGUN:WP_LIGHTNING;
 else if(!Q_stricmp(set,"needlers"))weapon=WP_GRENADE_LAUNCHER;
 return weapon?BG_FindItemForWeapon(weapon):item;
}
void G_QceVariantRecharge(gentity_t *ent) {
 gclient_t *cl=ent->client;
 int elapsed,total,points,max=cl->ps.qceMaxShield,period;
 if(ent->health<=0 || !max || !GV(GV_RECHARGE_RATE) || level.time<cl->qceShieldNextTick || cl->ps.stats[STAT_QCE_SHIELD]>=max)return;
 elapsed=level.time-cl->qceShieldNextTick;
 period=(int)(BG_QcePlayerDef()->shield_recharge_ms/GV(GV_RECHARGE_RATE));if(period<1)period=1;
 if(elapsed>period)elapsed=period;
 total=elapsed*max+cl->qceShieldRemainder;points=total/period;cl->qceShieldRemainder=total%period;
 cl->ps.stats[STAT_QCE_SHIELD]+=points;
 if(cl->ps.stats[STAT_QCE_SHIELD]>=max) {cl->ps.stats[STAT_QCE_SHIELD]=max;cl->qceShieldRemainder=0;}
 cl->qceShieldNextTick=level.time;
}
/* Restricted basename, bounded text, complete validation before changing any cvars. */
qboolean G_QceVariantCommand(const char *command) {
 char filename[64],path[96],text[8192],values[GV_COUNT+4][32],key[64],*cursor,*token;
 fileHandle_t file;int i,j,length,found[GV_COUNT+4],save;
 if(Q_stricmp(command,"gv_save") && Q_stricmp(command,"gv_load"))return qfalse;
 save=!Q_stricmp(command,"gv_save");
 if(trap_Argc()!=2) {G_Printf("Usage: %s <filename> (variant basename without extension)\n",command);return qtrue;}
 trap_Argv(1,filename,sizeof(filename));
 length=strlen(filename);
 if(length>4 && !Q_stricmp(filename+length-4,".cfg")) {filename[length-4]=0;length-=4;}
 if(!length || length>48) {G_Printf("Invalid variant filename\n");return qtrue;}
 for(i=0;i<length;i++)if(!((filename[i]>='a' && filename[i]<='z') || (filename[i]>='A' && filename[i]<='Z') || (filename[i]>='0' && filename[i]<='9') || filename[i]=='_' || filename[i]=='-')) {G_Printf("Use letters, digits, underscore or hyphen for the filename\n");return qtrue;}
 Com_sprintf(path,sizeof(path),"variants/%s.cfg",filename);
 if(save) {
  G_QceVariantUpdate();text[0]=0;Q_strcat(text,sizeof(text),"qce_variant 1\n");
  for(i=0;i<GV_COUNT+4;i++)Q_strcat(text,sizeof(text),va("%s \"%s\"\n",Name(i),i<GV_COUNT?variants[i].cvar.string:stringCvars[i-GV_COUNT].string));
  trap_FS_FOpenFile(path,&file,FS_WRITE);
  if(!file) {G_Printf("Unable to write %s\n",path);return qtrue;}
  trap_FS_Write(text,strlen(text),file);trap_FS_FCloseFile(file);G_Printf("Saved %s\n",path);return qtrue;
 }
 length=trap_FS_FOpenFile(path,&file,FS_READ);
 if(length<=0 || length>=sizeof(text)) {if(file)trap_FS_FCloseFile(file);G_Printf("Missing or oversized variant %s\n",path);return qtrue;}
 trap_FS_Read(text,length,file);trap_FS_FCloseFile(file);text[length]=0;cursor=text;
 if(Q_stricmp(COM_Parse(&cursor),"qce_variant") || Q_stricmp(COM_Parse(&cursor),"1")) {G_Printf("Unsupported variant format\n");return qtrue;}
 memset(found,0,sizeof(found));
 for(j=0;j<GV_COUNT+4;j++) {
  Q_strncpyz(key,COM_Parse(&cursor),sizeof(key));
  if(!key[0])break;
  for(i=0;i<GV_COUNT+4;i++)if(!Q_stricmp(key,Name(i)))break;
  token=COM_Parse(&cursor);
  if(i==GV_COUNT+4 || found[i] || strlen(token)>=sizeof(values[0]) || !Valid(i,token)) {G_Printf("Invalid variant entry %s; nothing changed\n",key);return qtrue;}
  Q_strncpyz(values[i],token,sizeof(values[i]));found[i]=1;
 }
 /* Version 1 files written before maxHeldWeapons retain the old two-gun default. */
 if(j==GV_COUNT+3 && !found[GV_MAX_HELD]) {Q_strncpyz(values[GV_MAX_HELD],"2",sizeof(values[0]));j++;}
 if(j!=GV_COUNT+4 || COM_Parse(&cursor)[0]) {G_Printf("Incomplete or extra variant entries; nothing changed\n");return qtrue;}
 for(i=0;i<GV_COUNT+4;i++)trap_Cvar_Set(Name(i),values[i]);
 G_QceVariantUpdate();G_Printf("Loaded %s; loadouts/weapon limit/health apply on respawn, map weapon set/game type on next map\n",path);return qtrue;
}
