#define QCE_WEAPON_TEST
#include "movement.c"
static void action(playerState_t *ps,int buttons,int msec) {
 pmove_t pm={0};pm.ps=ps;pm.trace=trace;pm.pointcontents=contents;pm.tracemask=MASK_PLAYERSOLID;
 pm.cmd.serverTime=ps->commandTime+msec;pm.cmd.buttons=buttons;pm.cmd.weapon=ps->weapon;Pmove(&pm);
}
int main(void) {
 playerState_t ps,copy;int i,seq;
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_GRENADE_LAUNCHER;BG_QceAddWeapon(&ps,ps.weapon,100);copy=ps;
 assert(BG_QceFireTime(&ps)==333);
 BG_QceUpdateRate(&ps,500,BUTTON_ATTACK);for(i=0;i<500;i++)BG_QceUpdateRate(&copy,1,BUTTON_ATTACK);
 assert(!memcmp(&ps,&copy,sizeof(ps)) && BG_QceFireTime(&ps)==100);
 BG_QceUpdateRate(&ps,150,0);assert(BG_QceFireTime(&ps)==333);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_PLASMAGUN;BG_QceAddWeapon(&ps,ps.weapon,200);
 assert(BG_QceFireTime(&ps)==167);BG_QceUpdateRate(&ps,1800,BUTTON_ATTACK);assert(BG_QceFireTime(&ps)==100);
 /* Deliberately fractional remaining battery must not round down to whole shots. */
 ps.qceBattery[0]=12345;BG_QceBatteryShot(&ps,0);assert(ps.qceBattery[0]==7345 && ps.ammo[ps.weapon]==2);
 BG_QceBatteryShot(&ps,0);assert(ps.qceBattery[0]==2345 && ps.ammo[ps.weapon]==1);
 BG_QceBatteryShot(&ps,0);assert(!ps.qceBattery[0] && !ps.ammo[ps.weapon]);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_LIGHTNING;BG_QceAddWeapon(&ps,ps.weapon,500);
 ps.qceBattery[0]=113333;BG_QceBatteryShot(&ps,1);assert(ps.qceBattery[0]==3333 && ps.ammo[ps.weapon]==2);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_RAILGUN;BG_QceAddWeapon(&ps,ps.weapon,28);
 action(&ps,BUTTON_QCE_ZOOM,16);assert((ps.qceZoom&3)==1 && BG_QceZoom(ps.weapon,1)==2);
 action(&ps,BUTTON_QCE_ZOOM,16);assert((ps.qceZoom&3)==1);
 action(&ps,0,16);action(&ps,BUTTON_QCE_ZOOM,16);assert((ps.qceZoom&3)==2 && BG_QceZoom(ps.weapon,2)==8);
 action(&ps,0,16);action(&ps,BUTTON_ATTACK,16);assert(((ps.eventParms[(ps.eventSequence-1)&1]>>1)&127)==127);
 action(&ps,0,600);action(&ps,BUTTON_QCE_RELOAD,16);assert(!(ps.qceZoom&3) && ps.weaponTime==2767);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_RAILGUN;BG_QceAddWeapon(&ps,ps.weapon,28);ps.stats[STAT_QCE_MAG0]=0;
 action(&ps,BUTTON_QCE_RELOAD,16);assert(ps.weaponTime==3133);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_BFG;BG_QceAddWeapon(&ps,ps.weapon,132);
 action(&ps,BUTTON_QCE_ZOOM,16);action(&ps,0,16);action(&ps,BUTTON_QCE_ZOOM,16);assert(!(ps.qceZoom&3));
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_MACHINEGUN,120);seq=ps.eventSequence;
 action(&ps,BUTTON_QCE_MELEE,16);assert(ps.eventSequence==seq+1);action(&ps,0,90);assert(ps.eventSequence==seq+1);
 action(&ps,0,10);assert(ps.eventSequence==seq+2 && ps.events[(seq+1)&1]==EV_QCE_MELEE_STRIKE);
 init(&ps,1);for(i=0;i<100;i++)step(&ps,-127,0,0,0);assert(fabs(ps.velocity[0]+160)<0.1);
 init(&ps,1);for(i=0;i<100;i++)step(&ps,0,127,-127,0);assert(fabs(ps.velocity[1]+48)<0.1 && ps.viewheight==4 && ps.qceCrouch==10000);
 for(i=0;i<25;i++)step(&ps,0,0,0,0);assert(ps.viewheight==26 && !ps.qceCrouch);
 init(&ps,1);step(&ps,127,0,0,0);assert(fabs(ps.velocity[0]-6.144)<0.01);
 for(i=0;i<100;i++)step(&ps,127,0,0,0);for(i=0;i<30;i++)step(&ps,0,0,0,0);assert(fabs(ps.velocity[0])<0.01);
 assert(BG_QceDamage(WP_SHOTGUN,0,0)==8 && BG_QceDamage(WP_SHOTGUN,1,1)==25);
 puts("PASS: frame-independent fire-rate ramps, fractional battery, authoritative zoom cycles/scoped shot capture, full/empty reload, melee keyframe and imported directional movement/crouch camera/braking");return 0;
}
