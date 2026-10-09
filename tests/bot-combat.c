#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "ai_main.h"
#include <assert.h>
#include <stdio.h>
gentity_t g_entities[MAX_GENTITIES];level_locals_t level;
vmCvar_t g_gametype,g_gravity;
static gclient_t clients[3];static int blocked;
void BotQceCombatInput(bot_state_t *,int);
qboolean OnSameTeam(gentity_t *a,gentity_t *b) {return a->client->sess.sessionTeam==b->client->sess.sessionTeam;}
void trap_Trace(trace_t *t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 memset(t,0,sizeof(*t));t->fraction=blocked?0.5f:1;t->entityNum=ENTITYNUM_WORLD;
}
static void setup(bot_state_t *bs,float distance) {
 int i;memset(bs,0,sizeof(*bs));memset(g_entities,0,sizeof(g_entities));memset(clients,0,sizeof(clients));
 level.maxclients=3;g_gravity.value=800;g_gametype.integer=GT_FFA;blocked=0;
 for(i=0;i<3;i++){g_entities[i].client=&clients[i];g_entities[i].health=75;g_entities[i].inuse=1;clients[i].sess.sessionTeam=i?TEAM_BLUE:TEAM_RED;}
 bs->client=0;bs->enemy=1;clients[0].ps.stats[STAT_QCE_COMBAT]=1;clients[0].ps.stats[STAT_HEALTH]=75;clients[0].ps.viewheight=26;clients[0].ps.weapon=WP_MACHINEGUN;
 BG_QceSetGrenadeCount(&clients[0].ps,0,2);VectorSet(g_entities[1].r.currentOrigin,distance,0,0);g_entities[1].r.mins[2]=-24;
}
int main(void) {
 bot_state_t bs;int aim;
 setup(&bs,40);BotQceCombatInput(&bs,1000);assert(bs.lastucmd.buttons&BUTTON_QCE_MELEE);assert(!(bs.lastucmd.buttons&BUTTON_ATTACK));
 setup(&bs,400);BotQceCombatInput(&bs,1000);assert(bs.lastucmd.buttons&BUTTON_QCE_GRENADE);assert(bs.qceGrenadeAimUntil==1400);aim=bs.lastucmd.angles[0];bs.lastucmd.angles[0]=0;bs.lastucmd.buttons=0;BotQceCombatInput(&bs,1250);assert(bs.lastucmd.angles[0]==aim && (bs.lastucmd.buttons&BUTTON_QCE_GRENADE));
 setup(&bs,400);clients[0].ps.weaponstate=WEAPON_FIRING;clients[0].ps.weaponTime=67;BotQceCombatInput(&bs,1000);assert(bs.lastucmd.buttons&BUTTON_QCE_GRENADE);
 setup(&bs,400);blocked=1;BotQceCombatInput(&bs,1000);assert(!bs.lastucmd.buttons);
 setup(&bs,150);BotQceCombatInput(&bs,1000);assert(!bs.lastucmd.buttons);
 setup(&bs,400);g_gametype.integer=GT_TEAM;clients[2].sess.sessionTeam=TEAM_RED;VectorSet(g_entities[2].r.currentOrigin,400,30,0);BotQceCombatInput(&bs,1000);assert(!bs.lastucmd.buttons);
 setup(&bs,400);BG_QceSetGrenadeCount(&clients[0].ps,0,0);BG_QceSetGrenadeCount(&clients[0].ps,1,2);BotQceCombatInput(&bs,1000);assert(bs.lastucmd.buttons&BUTTON_QCE_GRENADE);assert(BG_QceGrenadeType(&clients[0].ps)==1);
 puts("PASS: bot melee reach, ballistic grenade aim held through release, obstacle/self/team safety and available grenade type");return 0;
}
