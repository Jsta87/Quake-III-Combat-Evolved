#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];level_locals_t level;vmCvar_t g_qceCombat;
static int unlinked;
void trap_UnlinkEntity(gentity_t *e) {assert(e>=g_entities+MAX_CLIENTS);unlinked++;}
void trap_LocateGameData(gentity_t *data,int count,int size,playerState_t *ps,int stride) {(void)data;(void)count;(void)size;(void)ps;(void)stride;}
void QDECL G_Printf(const char *fmt,...) {(void)fmt;}
void QDECL G_Error(const char *fmt,...) {(void)fmt;abort();}
int main(void) {
 gitem_t weapon={0},flag={0};gentity_t *e;int i,serial;
 weapon.giType=IT_WEAPON;flag.giType=IT_TEAM;g_qceCombat.integer=1;level.time=10000;level.num_entities=ENTITYNUM_MAX_NORMAL;
 for(i=0;i<level.num_entities;i++){g_entities[i].inuse=qtrue;g_entities[i].s.number=i;}
 g_entities[MAX_CLIENTS].s.eType=ET_ITEM;g_entities[MAX_CLIENTS].item=&weapon; /* permanent map item */
 for(i=1;i<=3;i++){g_entities[MAX_CLIENTS+i].s.eType=ET_ITEM;g_entities[MAX_CLIENTS+i].item=&weapon;g_entities[MAX_CLIENTS+i].flags=FL_DROPPED_ITEM;g_entities[MAX_CLIENTS+i].timestamp=5000/i;}
 g_entities[MAX_CLIENTS+3].item=&flag;g_entities[MAX_CLIENTS+3].timestamp=0;
 assert(G_EntitiesFree());e=G_Spawn();assert(e==g_entities+MAX_CLIENTS+2 && e->inuse && unlinked==1);serial=e->qceEntitySerial;
 assert(g_entities[MAX_CLIENTS].inuse && g_entities[MAX_CLIENTS+3].item==&flag);
 e=G_Spawn();assert(e==g_entities+MAX_CLIENTS+1 && e->qceEntitySerial>serial && unlinked==2);
 assert(!G_EntitiesFree());e->inuse=qfalse;assert(G_EntitiesFree());assert(G_Spawn()==e);
 puts("PASS: full 4096-entity pool reclaims oldest dropped weapon while preserving map items, flags, players and serial ownership");return 0;
}
