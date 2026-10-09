#define main existing_headshot_suite
#include "headshots.c"
#undef main
static int blocked;
void trap_Trace(trace_t *t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
 (void)start;(void)mins;(void)maxs;(void)end;(void)pass;(void)mask;memset(t,0,sizeof(*t));t->fraction=blocked?0:1;t->entityNum=ENTITYNUM_WORLD;
}
int trap_EntitiesInBox(const vec3_t mins,const vec3_t maxs,int *list,int max) {(void)mins;(void)maxs;(void)max;list[0]=1;return 1;}
qboolean LogAccuracyHit(gentity_t *a,gentity_t *b) {(void)a;(void)b;return qtrue;}
static void blast(float distance) {
 vec3_t origin={0,0,0};reset(0,1000);VectorSet(g_entities[1].r.currentOrigin,distance,0,-4);
 G_QceRadiusDamage(origin,&g_entities[0],100,20,100,40,160,NULL,MOD_ROCKET_SPLASH);
}
int main(void) {
 vec3_t point,entry={84,200,330},dir={1,0,0},origin={0,0,0};
 blast(39);assert(g_entities[1].health==900);blast(100);assert(g_entities[1].health==940);
 blast(159);assert(g_entities[1].health==979);blast(160);assert(g_entities[1].health==1000);
 /* Imported frag damage must not kill a fresh 75-shield/75-health Spartan. */
 reset(75,75);VectorSet(g_entities[1].r.currentOrigin,0,0,-4);
 G_QceRadiusDamage(origin,&g_entities[0],120,80,120,80,200,NULL,MOD_QCE_FRAG);
 assert(clients[1].ps.stats[STAT_QCE_SHIELD]==0 && g_entities[1].health==30);
 reset(75,75);VectorSet(g_entities[1].r.currentOrigin,140,0,-4);
 G_QceRadiusDamage(origin,&g_entities[0],120,80,120,80,200,NULL,MOD_QCE_FRAG);
 assert(clients[1].ps.stats[STAT_QCE_SHIELD]==0 && g_entities[1].health==50);
 g_knockback.value=1000;reset(75,75);shoot((vec3_t){100,200,300},MOD_MACHINEGUN,10,0);assert(clients[1].ps.velocity[0]==0);
 reset(75,75);shoot((vec3_t){100,200,300},MOD_GAUNTLET,10,0);assert(clients[1].ps.velocity[0]==0);
 reset(75,75);shoot((vec3_t){100,200,300},MOD_ROCKET,10,0);assert(fabs(clients[1].ps.velocity[0]-25)<0.01);
 reset(75,75);shoot((vec3_t){100,200,300},MOD_QCE_FRAG,10,DAMAGE_RADIUS);assert(fabs(clients[1].ps.velocity[0]-25)<0.01);
 g_knockback.value=0;
 blocked=1;blast(30);assert(g_entities[1].health==1000);blocked=0;
 reset(0,1000);VectorSet(g_entities[1].r.currentOrigin,0,0,-4);
 G_QceRadiusDamage(origin,&g_entities[0],100,20,100,40,160,&g_entities[1],MOD_ROCKET_SPLASH);assert(g_entities[1].health==1000);
 reset(0,100);G_QceResolveHeadPoint(&g_entities[1],entry,dir,point);assert(point[0]>90 && point[0]<100 && G_QceHeadshot(&g_entities[1],point,MOD_BFG,0));
 entry[1]=213;G_QceResolveHeadPoint(&g_entities[1],entry,dir,point);assert(!G_QceHeadshot(&g_entities[1],point,MOD_BFG,0));
 clients[1].ps.qceZoom=2;shoot((vec3_t){100,200,300},MOD_MACHINEGUN,10,0);assert(!clients[1].ps.qceZoom);
 reset(0,1000);VectorSet(g_entities[1].r.currentOrigin,0,0,-4);
 G_QceRadiusDamage(origin,&g_entities[1],100,20,100,40,160,NULL,MOD_QCE_FRAG);
 assert(g_entities[1].health==900); /* Halo explosives do full damage to their owner. */
 GV(GV_GRENADE_DAMAGE)=2;reset(0,1000);VectorSet(g_entities[1].r.currentOrigin,0,0,-4);
 G_QceRadiusDamage(origin,&g_entities[0],100,20,100,40,160,NULL,MOD_QCE_FRAG);assert(g_entities[1].health==800);GV(GV_GRENADE_DAMAGE)=1;
 GV(GV_HEALTH)=0;reset(0,1000);shoot((vec3_t){100,200,300},MOD_MACHINEGUN,100,0);assert(g_entities[1].health==1000);GV(GV_HEALTH)=1;
 /* Real combat path must transfer blast impulse to non-client vehicles. */
 reset(0,1000);g_entities[1].client=NULL;g_entities[1].qceVehicle=1;
 VectorSet(g_entities[1].r.currentOrigin,40,0,-4);
 G_QceRadiusDamage(origin,&g_entities[0],120,80,120,80,200,NULL,MOD_QCE_FRAG);
 assert(g_entities[1].qceVehicleVelocity[0]>80 && g_entities[1].qceVehicleVelocity[2]>30);
 assert(g_entities[1].qceVehicleAngularVelocity[1]>3);
 VectorClear(g_entities[1].qceVehicleVelocity);VectorClear(g_entities[1].qceVehicleAngularVelocity);
 G_Damage(&g_entities[1],NULL,&g_entities[0],dir,origin,120,0,MOD_MACHINEGUN);
 assert(VectorLength(g_entities[1].qceVehicleVelocity)==0 && VectorLength(g_entities[1].qceVehicleAngularVelocity)==0);
 G_Damage(&g_entities[1],NULL,&g_entities[0],dir,origin,120,DAMAGE_RADIUS|DAMAGE_NO_KNOCKBACK,MOD_QCE_FRAG);
 assert(VectorLength(g_entities[1].qceVehicleVelocity)==0);
 blocked=1;G_QceRadiusDamage(origin,&g_entities[0],120,80,120,80,200,NULL,MOD_QCE_FRAG);assert(VectorLength(g_entities[1].qceVehicleVelocity)==0);blocked=0;
 puts("PASS: actual radial damage inner/full strength, minimum-damage falloff, outer cutoff, wall occlusion, direct-hit exclusion, precision ray/head intersection and damage dezoom");return 0;
}
