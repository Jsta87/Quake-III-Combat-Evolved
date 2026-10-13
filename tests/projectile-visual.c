/* Exercise the real client projectile path, including late snapshots and trails. */
#include "../engine/code/cgame/cg_ents.c"
#include <assert.h>
#include <stdio.h>
cg_t cg;cgs_t cgs;centity_t cg_entities[MAX_GENTITIES];
static localEntity_t effects[8];static int count,blocked;
localEntity_t *CG_AllocLocalEntity(void){assert(count<8);memset(&effects[count],0,sizeof(effects[count]));return &effects[count++];}
void CG_Trace(trace_t *trace,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int skip,int mask){
 int i;memset(trace,0,sizeof(*trace));trace->fraction=blocked?.5f:1;for(i=0;i<3;i++)trace->endpos[i]=start[i]+trace->fraction*(end[i]-start[i]);
}
int main(void){
 centity_t missile;centity_t *player=&cg_entities[0];vec3_t result,start={14,0,26},target={1014,0,26};
 memset(&missile,0,sizeof(missile));cg.time=600;cg.renderingThirdPerson=qtrue;
 missile.currentState.time=100;missile.currentState.otherEntityNum2=1;missile.currentState.pos.trType=TR_LINEAR;
 VectorCopy(start,missile.currentState.origin2);VectorCopy(target,missile.currentState.angles2);
 player->currentValid=qtrue;player->qceWorldMuzzleTime=cg.time;VectorSet(player->qceWorldMuzzle,10,0,0);
 VectorCopy(start,missile.lerpOrigin);CG_QceProjectileVisualOrigin(&missile,result);
 assert(fabs(result[0]-10)<.001f && fabs(result[2])<.001f); /* Starts at barrel, even >120 ms after launch. */
 VectorSet(missile.lerpOrigin,414,0,26);CG_QceProjectileVisualOrigin(&missile,result);
 assert(fabs(result[2]-(26-26*.648f))<.001f);
 player->currentValid=qfalse;player->qceWorldMuzzleTime=1;CG_QceProjectileVisualOrigin(&missile,result);
 assert(fabs(result[2]-(26-26*.648f))<.001f); /* Cached launch survives shooter leaving PVS. */
 VectorCopy(target,missile.lerpOrigin);CG_QceProjectileVisualOrigin(&missile,result);assert(fabs(result[2]-26)<.001f);
 VectorCopy(start,missile.lerpOrigin);CG_QceProjectileVisualOrigin(&missile,result);assert(fabs(result[2]-26)<.001f); /* No offset revival after ricochet. */
 player->currentValid=qtrue;player->qceWorldMuzzleTime=cg.time;missile.currentState.time=101;blocked=1;
 CG_QceProjectileVisualOrigin(&missile,result);assert(fabs(result[2]-13)<.001f);blocked=0;
 cg.renderingThirdPerson=qfalse;CG_QceProjectileVisualOrigin(&missile,result);assert(fabs(result[2]-26)<.001f);
 cg.renderingThirdPerson=qtrue;CG_HaloSniperTrail(start,target,0);assert(count==4);
 assert(fabs(effects[0].refEntity.origin[0]-10)<.001f && fabs(effects[0].refEntity.origin[2])<.001f);
 assert(fabs(effects[3].refEntity.oldorigin[0]-target[0])<.001f && fabs(effects[3].refEntity.oldorigin[2]-target[2])<.001f);
 for(count=1;count<4;count++)assert(fabs(effects[count-1].refEntity.oldorigin[2]-effects[count].refEntity.origin[2])<.001f);
 puts("PASS: barrel launch, late snapshot retention, distance convergence, PVS cache, monotonic ricochet handling, wall clipping, first-person preservation and sniper curve endpoints");return 0;
}
