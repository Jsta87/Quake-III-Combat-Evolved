#define QCE_WEAPON_TEST
#include "movement.c"
static void action(playerState_t *ps,int buttons,int msec) {
 pmove_t pm;memset(&pm,0,sizeof(pm));pm.ps=ps;pm.trace=trace;pm.pointcontents=contents;pm.tracemask=MASK_PLAYERSOLID;
 pm.cmd.serverTime=ps->commandTime+msec;pm.cmd.buttons=buttons;pm.cmd.weapon=ps->weapon;Pmove(&pm);
}
int main(void) {
 playerState_t ps,copy;int e=0,r=0,i,fraction;
 const qce_weapondef_t *ar=BG_QceWeaponDef(WP_MACHINEGUN);
 BG_QceUpdateError(WP_MACHINEGUN,300,1,&e,&r);assert(e==5000 && r==100);
 BG_QceUpdateError(WP_MACHINEGUN,300,1,&e,&r);assert(e==10000 && !r);
 BG_QceUpdateError(WP_MACHINEGUN,500,0,&e,&r);assert(e==5000);
 BG_QceUpdateError(WP_MACHINEGUN,500,0,&e,&r);assert(!e && !r);
 assert(fabs(BG_QceSpread(WP_MACHINEGUN,0)-ar->spread)<0.001);
 assert(fabs(BG_QceSpread(WP_MACHINEGUN,127)-ar->spread_max)<0.001);
 assert(BG_QceSpread(WP_MACHINEGUN,64)>ar->spread && BG_QceSpread(WP_MACHINEGUN,64)<ar->spread_max);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_MACHINEGUN;BG_QceAddWeapon(&ps,WP_MACHINEGUN,240);BG_QceAddWeapon(&ps,WP_BFG,60);copy=ps;
 BG_QceUpdateSpread(&ps,300,BUTTON_ATTACK);for(i=0;i<300;i++)BG_QceUpdateSpread(&copy,1,BUTTON_ATTACK);assert(!memcmp(&ps,&copy,sizeof(ps)));
 ps.qceError[1]=10000;BG_QceUpdateSpread(&ps,200,0);assert(!ps.qceError[1]); /* Holstered Magnum recovers. */
 ps.qceError[0]=10000;ps.qceErrorRemainder[0]=0;BG_QceRemoveWeapon(&ps,WP_MACHINEGUN);assert(!ps.qceError[0]);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_MACHINEGUN;BG_QceAddWeapon(&ps,WP_MACHINEGUN,240);copy=ps;
 for(i=0;i<80;i++){action(&ps,BUTTON_ATTACK,16);action(&copy,BUTTON_ATTACK,16);assert(!memcmp(&ps,&copy,sizeof(ps)));}
 fraction=(ps.eventParms[(ps.eventSequence-1)&1]>>1)&127;assert(fraction==127 && !(ps.eventParms[(ps.eventSequence-1)&1]&1));
 for(i=0;i<80;i++)action(&ps,0,16);assert(!ps.qceError[0]);
 init(&ps,0);ps.qceError[0]=2345;BG_QceUpdateSpread(&ps,100,0);assert(ps.qceError[0]==2345);
 assert(BG_QceDistanceDamageScale(WP_MACHINEGUN,2000)==1 && BG_QceDistanceDamageScale(WP_BFG,3200)==1);
 assert(BG_QceDistanceDamageScale(WP_SHOTGUN,120)==1 && BG_QceDistanceDamageScale(WP_SHOTGUN,240)==0);
 assert(BG_QceDistanceDamageScale(WP_SHOTGUN,180)>0.5 && BG_QceDistanceDamageScale(WP_SHOTGUN,180)<0.6);
 puts("PASS: tag-derived spread growth/recovery, angle endpoints, frame-independent prediction, holstered recovery, slot reset, shot-event capture, stock fallback and dry-air pellet attenuation");return 0;
}
