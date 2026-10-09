#include "g_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];
level_locals_t level;
vmCvar_t g_knockback,g_friendlyFire,g_debugDamage,g_gametype;
static gclient_t clients[2];
static int deaths,pains,sameTeam,notices,shieldHits,shieldBreaks;
static gentity_t notice,shieldNotice;
gentity_t *G_TempEntity(vec3_t origin,int event) {assert(origin);if(event==EV_QCE_SHIELD_HIT || event==EV_QCE_SHIELD_BREAK) {if(event==EV_QCE_SHIELD_BREAK)shieldBreaks++;else shieldHits++;memset(&shieldNotice,0,sizeof(shieldNotice));return &shieldNotice;}assert(event==EV_QCE_HEADSHOT);notices++;memset(&notice,0,sizeof(notice));return &notice;}
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_Error(int level,const char *fmt,...) {(void)level;(void)fmt;abort();}
void QDECL G_Printf(const char *fmt,...) {(void)fmt;}
void G_AddEvent(gentity_t *ent,int event,int parm) {(void)ent;(void)event;(void)parm;}
qboolean OnSameTeam(gentity_t *a,gentity_t *b) {(void)a;(void)b;return sameTeam;}
void Team_CheckHurtCarrier(gentity_t *target,gentity_t *attacker) {(void)target;(void)attacker;}
static void died(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,int damage,int mod) {
 (void)inflictor;(void)attacker;(void)mod;assert(target->health<=0 && damage>0);deaths++;
}
static void pain(gentity_t *target,gentity_t *attacker,int damage) {(void)target;(void)attacker;assert(damage>0);pains++;}
static void reset(int shield,int health) {
 int i;
 memset(g_entities,0,sizeof(g_entities));memset(clients,0,sizeof(clients));memset(&level,0,sizeof(level));
 deaths=pains=sameTeam=notices=shieldHits=shieldBreaks=0;g_friendlyFire.integer=0;
 for(i=0;i<2;i++) {
  g_entities[i].client=&clients[i];g_entities[i].s.number=i;g_entities[i].s.eType=ET_PLAYER;
  g_entities[i].health=health;g_entities[i].takedamage=qtrue;g_entities[i].die=died;g_entities[i].pain=pain;
  clients[i].ps.stats[STAT_QCE_COMBAT]=1;clients[i].ps.stats[STAT_MAX_HEALTH]=100;
  clients[i].ps.stats[STAT_HEALTH]=health;clients[i].ps.stats[STAT_QCE_SHIELD]=shield;
 }
 VectorSet(g_entities[1].r.currentOrigin,100,200,300);
 VectorSet(g_entities[1].r.mins,-15,-15,-24);VectorSet(g_entities[1].r.maxs,15,15,32);
 level.time=1000;
}
static void shoot(vec3_t point,int mod,int damage,int flags) {
 vec3_t dir={1,0,0};G_Damage(&g_entities[1],&g_entities[0],&g_entities[0],dir,point,damage,flags,mod);
}
int main(void) {
 vec3_t head={100,200,330},body={100,200,300},edge={100,200,320.8f};
 reset(100,100);shoot(head,MOD_BFG,50,0);
 assert(g_entities[1].health==100 && clients[1].ps.stats[STAT_QCE_SHIELD]==50 && deaths==0);
 assert(clients[1].qceShieldNextTick==7000);assert(shieldHits==1 && shieldBreaks==0 && shieldNotice.s.otherEntityNum==1);
 reset(20,100);shoot(head,MOD_BFG,50,0);
 assert(g_entities[1].health==0 && clients[1].ps.stats[STAT_QCE_SHIELD]==0 && deaths==1 && notices==1);
 reset(50,100);shoot(head,MOD_BFG,50,0);
 assert(g_entities[1].health==0 && deaths==1 && notices==1); /* exact depletion, no overflow */
 assert(shieldBreaks==1 && shieldHits==0);
 reset(100,100);shoot(body,MOD_BFG,40,0);shoot(body,MOD_BFG,40,0);
 assert(clients[1].ps.stats[STAT_QCE_SHIELD]==20 && g_entities[1].health==100 && deaths==0);
 shoot(head,MOD_BFG,40,0);assert(g_entities[1].health==0 && deaths==1 && notices==1);
 reset(20,100);shoot(body,MOD_BFG,50,0);
 assert(g_entities[1].health==55 && deaths==0 && notices==0);
 reset(0,100);shoot(head,MOD_BFG,50,0);
 assert(notice.s.otherEntityNum==1 && notice.s.otherEntityNum2==0);
 reset(0,100);shoot(body,MOD_BFG,50,0);assert(g_entities[1].health==25 && deaths==0 && notices==0);
 reset(0,200);shoot(head,MOD_BFG,50,0);assert(g_entities[1].health==0 && deaths==1);
 reset(0,100);shoot(head,MOD_MACHINEGUN,7,0);assert(g_entities[1].health==93);
 reset(0,100);shoot(head,MOD_GAUNTLET,50,0);assert(g_entities[1].health==50);
 reset(0,100);shoot(head,MOD_ROCKET_SPLASH,50,DAMAGE_RADIUS);assert(g_entities[1].health==50);
 reset(0,100);shoot(head,MOD_BFG,50,DAMAGE_RADIUS);assert(g_entities[1].health==25);
 reset(0,100);shoot(head,MOD_BFG,50,DAMAGE_NO_ARMOR);assert(g_entities[1].health==50);
 reset(0,100);shoot(NULL,MOD_BFG,50,0);assert(g_entities[1].health==25);
 reset(0,100);clients[1].ps.stats[STAT_QCE_COMBAT]=0;
 shoot(head,MOD_BFG,50,0);assert(g_entities[1].health==50);
 reset(0,100);clients[0].ps.stats[STAT_QCE_COMBAT]=0;
 shoot(head,MOD_BFG,50,0);assert(g_entities[1].health==25);
 reset(0,100);g_entities[1].flags|=FL_GODMODE;
 shoot(head,MOD_BFG,50,0);assert(g_entities[1].health==100 && deaths==0);
 reset(0,100);sameTeam=1;shoot(head,MOD_BFG,50,0);assert(g_entities[1].health==100);
 reset(0,100);g_entities[1].client->noclip=qtrue;shoot(head,MOD_BFG,50,0);assert(g_entities[1].health==100);
 reset(0,100);assert(G_QceHeadshot(&g_entities[1],edge,MOD_BFG,0));
 edge[2]=320.7f;assert(!G_QceHeadshot(&g_entities[1],edge,MOD_BFG,0));
 edge[2]=333;assert(!G_QceHeadshot(&g_entities[1],edge,MOD_BFG,0));
 edge[0]=116;edge[2]=330;assert(!G_QceHeadshot(&g_entities[1],edge,MOD_BFG,0));
 g_entities[1].r.maxs[2]=16;edge[0]=100;edge[2]=315;
 assert(G_QceHeadshot(&g_entities[1],edge,MOD_BFG,0));
 edge[2]=307;assert(!G_QceHeadshot(&g_entities[1],edge,MOD_BFG,0));

 reset(100,100);shoot(head,MOD_QCE_SNIPER,50,0);
 assert(g_entities[1].health==0 && clients[1].ps.stats[STAT_QCE_SHIELD]==0 && deaths==1);
 reset(100,100);shoot(body,MOD_QCE_SNIPER,50,0);
 assert(g_entities[1].health==100 && clients[1].ps.stats[STAT_QCE_SHIELD]==50 && deaths==0);
 reset(100,100);shoot(body,MOD_PLASMA,60,0);
 assert(clients[1].ps.stats[STAT_QCE_SHIELD]==0 && g_entities[1].health==95);
 reset(0,100);shoot(body,MOD_PLASMA,20,0);assert(g_entities[1].health==90);
 reset(100,100);shoot(body,MOD_GAUNTLET,50,DAMAGE_QCE_BACKSMACK);
 assert(g_entities[1].health==0 && clients[1].ps.stats[STAT_QCE_SHIELD]==0);
 reset(100,100);g_entities[1].flags|=FL_GODMODE;shoot(body,MOD_GAUNTLET,50,DAMAGE_QCE_BACKSMACK);
 assert(g_entities[1].health==100 && clients[1].ps.stats[STAT_QCE_SHIELD]==100);
 reset(100,100);sameTeam=1;shoot(body,MOD_GAUNTLET,50,DAMAGE_QCE_BACKSMACK);
 assert(g_entities[1].health==100 && clients[1].ps.stats[STAT_QCE_SHIELD]==100);

 reset(0,100);clients[0].ps.weapon=WP_MACHINEGUN;shoot(head,MOD_BFG,50,0);
 assert(g_entities[1].health==50 && notices==0); /* source definition can disable the critical policy */
 reset(100,100);clients[0].ps.weapon=WP_RAILGUN;shoot(head,MOD_QCE_SNIPER,50,0);
 assert(g_entities[1].health==0 && deaths==1);
 reset(100,100);clients[0].ps.weapon=WP_PLASMAGUN;shoot(body,MOD_PLASMA,60,0);
 assert(g_entities[1].health==95 && clients[1].ps.stats[STAT_QCE_SHIELD]==0);
 reset(100,100);clients[0].ps.weapon=WP_MACHINEGUN;
 G_Damage(&g_entities[1],&g_entities[3],&g_entities[0],NULL,body,60,DAMAGE_RADIUS,MOD_PLASMA_SPLASH);
 assert(g_entities[1].health==95 && clients[1].ps.stats[STAT_QCE_SHIELD]==0); /* switching cannot reclassify an in-flight missile */

 reset(75,75);clients[0].ps.weapon=WP_BFG;clients[0].ps.stats[STAT_MAX_HEALTH]=75;clients[0].pers.maxHealth=100;
 shoot(body,MOD_BFG,25,0);shoot(body,MOD_BFG,25,0);
 assert(clients[1].ps.stats[STAT_QCE_SHIELD]==25 && g_entities[1].health==75);
 shoot(head,MOD_BFG,25,0);assert(g_entities[1].health==0 && notices==1 && deaths==1);
 reset(75,75);clients[0].ps.weapon=WP_BFG;clients[0].ps.stats[STAT_MAX_HEALTH]=75;clients[0].pers.maxHealth=100;
 shoot(body,MOD_BFG,25,0);shoot(body,MOD_BFG,25,0);shoot(body,MOD_BFG,25,0);
 assert(clients[1].ps.stats[STAT_QCE_SHIELD]==0 && g_entities[1].health==75);
 reset(75,75);clients[0].ps.weapon=WP_RAILGUN;shoot(head,MOD_QCE_SNIPER,101,0);
 assert(g_entities[1].health==0 && notices==1);
 reset(75,75);clients[0].ps.weapon=WP_LIGHTNING;
 G_Damage(&g_entities[1],&g_entities[3],&g_entities[0],NULL,body,70,0,MOD_QCE_OVERCHARGE);
 assert(clients[1].ps.stats[STAT_QCE_SHIELD]==0 && g_entities[1].health==75 && notices==0);
 reset(30,75);clients[0].ps.weapon=WP_MACHINEGUN; /* In-flight charged bolt keeps its own damage policy. */
 G_Damage(&g_entities[1],&g_entities[3],&g_entities[0],NULL,body,70,0,MOD_QCE_OVERCHARGE);
 assert(clients[1].ps.stats[STAT_QCE_SHIELD]==0 && g_entities[1].health==51); /* float product rounds 40 * .600000024 to 24 */
 reset(0,75);G_Damage(&g_entities[1],&g_entities[3],&g_entities[0],NULL,body,70,0,MOD_QCE_OVERCHARGE);assert(g_entities[1].health==33);
 reset(75,75);g_entities[1].flags|=FL_GODMODE;G_Damage(&g_entities[1],&g_entities[3],&g_entities[0],NULL,body,70,0,MOD_QCE_OVERCHARGE);assert(clients[1].ps.stats[STAT_QCE_SHIELD]==75);
 reset(75,75);sameTeam=1;G_Damage(&g_entities[1],&g_entities[3],&g_entities[0],NULL,body,70,0,MOD_QCE_OVERCHARGE);assert(clients[1].ps.stats[STAT_QCE_SHIELD]==75);
 reset(75,75);G_Damage(&g_entities[1],&g_entities[3],&g_entities[0],NULL,body,70,DAMAGE_NO_ARMOR,MOD_QCE_OVERCHARGE);assert(clients[1].ps.stats[STAT_QCE_SHIELD]==75);
 puts("PASS: actual G_Damage post-hit shield gating, exact shield depletion, third-shot headshot, body overflow, sniper exception, plasma damage scaling, backsmack protections, source-policy resolution, body/nonprecision/stock fallback, protections and standing/crouched head-zone bounds");
 return 0;
}
