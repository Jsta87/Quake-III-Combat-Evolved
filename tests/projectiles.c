#include "../engine/code/game/g_missile.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
gentity_t g_entities[MAX_GENTITIES];level_locals_t level;vmCvar_t g_gametype;
static gclient_t shooter,victim;static int damage_calls,amount,water;static float wall=1000000;static int collisionEntity=ENTITYNUM_WORLD;
void QDECL Com_Printf(const char *f,...) {(void)f;}
void QDECL Com_Error(int n,const char *f,...) {(void)n;(void)f;abort();}
gentity_t *G_Spawn(void) {int i;for(i=2;i<100;i++)if(!g_entities[i].inuse){gentity_t *e=&g_entities[i];memset(e,0,sizeof(*e));e->inuse=qtrue;e->s.number=i;return e;}abort();}
void G_FreeEntity(gentity_t *e) {e->inuse=qfalse;}
void G_RunThink(gentity_t *e) {if(e->nextthink && e->nextthink<=level.time)e->think(e);}
void G_SetOrigin(gentity_t *e,vec3_t p) {VectorCopy(p,e->r.currentOrigin);VectorCopy(p,e->s.pos.trBase);e->s.pos.trType=TR_STATIONARY;}
void trap_LinkEntity(gentity_t *e) {(void)e;}
void G_AddEvent(gentity_t *e,int event,int p) {(void)e;(void)event;(void)p;}
gentity_t *G_TempEntity(vec3_t p,int event) {static gentity_t e;(void)p;(void)event;return &e;}
void SnapVectorTowards(vec3_t p,vec3_t q) {(void)p;(void)q;}
void Weapon_HookThink(gentity_t *e) {(void)e;}
qboolean OnSameTeam(gentity_t *a,gentity_t *b) {(void)a;(void)b;return qfalse;}
qboolean LogAccuracyHit(gentity_t *a,gentity_t *b) {(void)a;(void)b;return qfalse;}
int trap_PointContents(const vec3_t p,int pass) {(void)p;(void)pass;return water?CONTENTS_WATER:0;}
void G_QceResolveHeadPoint(gentity_t *target,const vec3_t entry,const vec3_t dir,vec3_t p) {(void)target;(void)dir;VectorCopy(entry,p);}
void trap_Trace(trace_t *t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 int i;(void)mins;(void)maxs;(void)mask;memset(t,0,sizeof(*t));t->fraction=1;t->entityNum=ENTITYNUM_NONE;VectorCopy(end,t->endpos);
 if(pass!=collisionEntity && start[0]<wall && end[0]>=wall) {
  t->fraction=(wall-start[0])/(end[0]-start[0]);t->entityNum=collisionEntity;t->plane.normal[0]=-1;
  for(i=0;i<3;i++)t->endpos[i]=start[i]+t->fraction*(end[i]-start[i]);
 }
}
void G_Damage(gentity_t *target,gentity_t *inf,gentity_t *att,vec3_t dir,vec3_t p,int damage,int flags,int mod) {
 (void)inf;(void)att;(void)dir;(void)p;(void)flags;(void)mod;damage_calls++;amount=damage;target->client->ps.stats[STAT_QCE_SHIELD]=0;
}
qboolean G_RadiusDamage(vec3_t o,gentity_t *a,float d,float r,gentity_t *i,int m) {(void)o;(void)a;(void)d;(void)r;(void)i;(void)m;return qfalse;}
qboolean G_QceRadiusDamage(vec3_t o,gentity_t *a,float d,float minimum,float maximum,float inner,float outer,gentity_t *ignore,int mod) {(void)minimum;(void)maximum;(void)inner;return G_RadiusDamage(o,a,d,outer,ignore,mod);}
static void setup(int weapon) {
 memset(g_entities,0,sizeof(g_entities));memset(&shooter,0,sizeof(shooter));memset(&victim,0,sizeof(victim));memset(&level,0,sizeof(level));
 level.maxclients=2;level.num_entities=100;g_entities[0].inuse=qtrue;g_entities[0].s.number=0;g_entities[0].client=&shooter;g_entities[0].qceEntitySerial=10;
 shooter.ps.stats[STAT_QCE_COMBAT]=1;shooter.ps.weapon=weapon;g_entities[1].client=&victim;g_entities[1].inuse=qtrue;g_entities[1].takedamage=qtrue;
 damage_calls=amount=water=0;wall=1000000;collisionEntity=ENTITYNUM_WORLD;
}
static void run(gentity_t *e,int ms) {level.previousTime=level.time;level.time+=ms;G_RunMissile(e);}
int main(void) {
 vec3_t start={0,0,1000},dir={1,0,0},end={3200,0,1000};gentity_t *e;gentity_t copy;int i;
 setup(WP_MACHINEGUN);wall=500;collisionEntity=1;G_QceFireBullet(&g_entities[0],start,end,WP_MACHINEGUN,MOD_MACHINEGUN,1);e=&g_entities[2];
 run(e,16);assert(!damage_calls && e->r.currentOrigin[0]>414 && e->r.currentOrigin[0]<415);
 run(e,8);assert(damage_calls==1 && amount==10); /* Damage occurs after flight, not on trigger press. */
 setup(WP_MACHINEGUN);G_QceFireBullet(&g_entities[0],start,end,WP_MACHINEGUN,MOD_MACHINEGUN,1);e=&g_entities[2];run(e,200);assert(!e->inuse && !damage_calls);
 setup(WP_MACHINEGUN);wall=500;collisionEntity=1;G_QceFireBullet(&g_entities[0],start,end,QCE_HOG_TURRET_WEAPON,MOD_MACHINEGUN,1);e=&g_entities[2];
 assert(e->s.pos.trDelta[0]>23439 && e->s.pos.trDelta[0]<23441);run(e,16);assert(!damage_calls);run(e,8);assert(damage_calls==1 && amount>=16 && amount<=24);
 assert(e->target_ent==&g_entities[1]); /* source flesh passthrough; no instant head kill */
 setup(WP_MACHINEGUN);G_QceFireBullet(&g_entities[0],start,end,QCE_HOG_TURRET_WEAPON,MOD_MACHINEGUN,1);e=&g_entities[2];run(e,2000);assert(!e->inuse && !damage_calls);
 setup(WP_LIGHTNING);e=fire_plasma(&g_entities[0],start,dir);copy=*e;run(e,80);
 assert(fabs(e->r.currentOrigin[2]-(1000-0.5*256.692913*0.1*0.08*0.08))<0.001);
 level.time=level.previousTime=0;g_entities[3]=copy;for(i=0;i<10;i++)run(&g_entities[3],8);
 assert(fabs(e->r.currentOrigin[0]-g_entities[3].r.currentOrigin[0])<0.001 && fabs(e->r.currentOrigin[2]-g_entities[3].r.currentOrigin[2])<0.001);
 setup(WP_PLASMAGUN);e=fire_plasma(&g_entities[0],start,dir);run(e,800);
 assert(e->s.pos.trDelta[0]<4000 && e->s.pos.trDelta[0]>2000 && e->inuse);run(e,800);assert(!e->inuse);
 setup(WP_LIGHTNING);e=fire_plasma(&g_entities[0],start,dir);water=1;run(e,80);assert(fabs(e->r.currentOrigin[2]-1000)<0.001);
 setup(WP_ROCKET_LAUNCHER);e=fire_rocket(&g_entities[0],start,dir);run(e,1000);assert(e->s.pos.trDelta[0]<960 && e->s.pos.trDelta[0]>800 && fabs(e->r.currentOrigin[2]-1000)<0.001);
 setup(WP_SHOTGUN);wall=180;collisionEntity=1;G_QceFireBullet(&g_entities[0],start,end,WP_SHOTGUN,MOD_SHOTGUN,1);e=&g_entities[2];run(e,32);assert(damage_calls==1 && amount>=13 && amount<=18 && !e->inuse);
 setup(WP_SHOTGUN);wall=1000;collisionEntity=1;G_QceFireBullet(&g_entities[0],start,end,WP_SHOTGUN,MOD_SHOTGUN,1);e=&g_entities[2];run(e,150);assert(damage_calls==1 && amount==8);
 setup(WP_RAILGUN);wall=500;collisionEntity=1;G_QceFireBullet(&g_entities[0],start,end,WP_RAILGUN,MOD_QCE_SNIPER,1);e=&g_entities[2];run(e,8);assert(e->inuse && e->target_ent==&g_entities[1] && damage_calls==1);
 setup(WP_GRENADE_LAUNCHER);e=fire_plasma(&g_entities[0],start,dir);e->s.pos.trDelta[0]=320;e->qceProjectileWeapon=WP_GRENADE_LAUNCHER;
 {trace_t hit={0};hit.entityNum=ENTITYNUM_WORLD;VectorSet(hit.plane.normal,-0.1,0,0.994987);assert(G_QceMaterialResponse(e,&hit,&g_entities[ENTITYNUM_WORLD])==2);}
 puts("PASS: real missile flight/sweeps, finite bullet delay and range expiry, gravity/frame partitioning, plasma/rocket slowdown, water response, randomized pellet attenuation, sniper penetration and grazing Needler ricochet");return 0;
}
