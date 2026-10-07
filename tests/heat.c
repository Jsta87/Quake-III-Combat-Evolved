#define QCE_WEAPON_TEST
#include "movement.c"
static void action(playerState_t *ps,int buttons,int weapon,int msec) {
 pmove_t pm;memset(&pm,0,sizeof(pm));pm.ps=ps;pm.trace=trace;pm.pointcontents=contents;pm.tracemask=MASK_PLAYERSOLID;
 pm.cmd.serverTime=ps->commandTime+msec;pm.cmd.buttons=buttons;pm.cmd.weapon=weapon;Pmove(&pm);
}
int main(void) {
 playerState_t ps,copy;int heat,remainder,locked,i,ammo;
 const qce_weapondef_t *rifle=BG_QceWeaponDef(WP_PLASMAGUN),*pistol=BG_QceWeaponDef(WP_LIGHTNING);
 assert(rifle->heat_per_shot==800 && rifle->heat_loss_per_second==3000 && rifle->heat_age_penalty==200);
 assert(pistol->heat_per_shot==1600 && pistol->heat_loss_per_second==6500 && pistol->heat_age_penalty==0);
 heat=10000;remainder=0;locked=1;BG_QceCoolWeapon(WP_PLASMAGUN,200,2500,&heat,&remainder,&locked);
 assert(heat==2500 && locked);BG_QceCoolWeapon(WP_PLASMAGUN,200,1,&heat,&remainder,&locked);assert(heat==2497 && !locked);
 heat=10000;remainder=0;locked=1;BG_QceCoolWeapon(WP_PLASMAGUN,0,3125,&heat,&remainder,&locked);
 assert(heat==2500 && locked);BG_QceCoolWeapon(WP_PLASMAGUN,0,1,&heat,&remainder,&locked);assert(heat<2500 && !locked);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_PLASMAGUN,200);BG_QceAddWeapon(&ps,WP_LIGHTNING,500);
 ps.qceHeat[0]=ps.qceHeat[1]=10000;ps.qceOverheated=3;copy=ps;
 BG_QceCoolWeapons(&ps,1000);for(i=0;i<1000;i++)BG_QceCoolWeapons(&copy,1);assert(!memcmp(&ps,&copy,sizeof(ps)));
 assert(ps.qceHeat[0]==7000 && ps.qceHeat[1]==3500); /* Holstered weapon cools too. */
 BG_QceRemoveWeapon(&ps,WP_LIGHTNING);assert(!ps.qceHeat[1] && !(ps.qceOverheated&2));
 BG_QceAddWeapon(&ps,WP_LIGHTNING,500);assert(!ps.qceHeat[1]);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_PLASMAGUN;BG_QceAddWeapon(&ps,WP_PLASMAGUN,200);
 ps.qceHeat[0]=10000;ps.qceOverheated=1;action(&ps,BUTTON_ATTACK,WP_PLASMAGUN,16);
 assert(ps.ammo[WP_PLASMAGUN]==200 && BG_QceMagazine(&ps,WP_PLASMAGUN)==200 && !(ps.eFlags&EF_FIRING));
 for(i=0;i<100;i++)action(&ps,BUTTON_ATTACK,WP_PLASMAGUN,16);assert(ps.ammo[WP_PLASMAGUN]==200);
 for(i=0;i<100 && ps.ammo[WP_PLASMAGUN]==200;i++)action(&ps,BUTTON_ATTACK,WP_PLASMAGUN,16);assert(ps.ammo[WP_PLASMAGUN]<200);
 ps.qceHeat[0]=0;ps.qceOverheated=0;copy=ps;
 for(i=0;i<500 && !BG_QceOverheated(&ps,WP_PLASMAGUN);i++) {
  action(&ps,BUTTON_ATTACK,WP_PLASMAGUN,16);action(&copy,BUTTON_ATTACK,WP_PLASMAGUN,16);assert(!memcmp(&ps,&copy,sizeof(ps)));
 }
 assert(BG_QceOverheated(&ps,WP_PLASMAGUN));ammo=ps.ammo[WP_PLASMAGUN];
 for(i=0;i<25;i++)action(&ps,BUTTON_ATTACK,WP_PLASMAGUN,16);assert(ps.ammo[WP_PLASMAGUN]==ammo);
 action(&ps,BUTTON_QCE_MELEE,WP_PLASMAGUN,16);assert(ps.weaponstate==WEAPON_MELEEING);
 /* Short taps release a primary bolt; holding releases the charged secondary. */
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_LIGHTNING;BG_QceAddWeapon(&ps,WP_LIGHTNING,500);
 action(&ps,BUTTON_ATTACK,WP_LIGHTNING,50);assert(ps.qceChargeMs==50 && ps.ammo[WP_LIGHTNING]==500);
 action(&ps,0,WP_LIGHTNING,10);assert(ps.ammo[WP_LIGHTNING]==499 && !ps.qceChargeMs);
 assert(ps.events[(ps.eventSequence-1)&1]==EV_FIRE_WEAPON && ps.eventParms[(ps.eventSequence-1)&1]==0);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_LIGHTNING;BG_QceAddWeapon(&ps,WP_LIGHTNING,500);
 for(i=0;i<6;i++)action(&ps,BUTTON_ATTACK,WP_LIGHTNING,100);
 assert(ps.qceChargeMs==600 && ps.ammo[WP_LIGHTNING]==500);ps.qceHeat[0]=3300;
 action(&ps,BUTTON_ATTACK,WP_LIGHTNING,100);assert(ps.qceHeat[0]==3300 && ps.qceChargeMs==600);
 copy=ps;action(&ps,0,WP_LIGHTNING,10);action(&copy,0,WP_LIGHTNING,10);assert(!memcmp(&ps,&copy,sizeof(ps)));
 assert(ps.ammo[WP_LIGHTNING]==445 && BG_QceMagazine(&ps,WP_LIGHTNING)==445 && !ps.qceChargeMs && ps.qceHeat[0]==10000 && BG_QceOverheated(&ps,WP_LIGHTNING));
 assert(ps.events[(ps.eventSequence-1)&1]==EV_FIRE_WEAPON && ps.eventParms[(ps.eventSequence-1)&1]==1);
 action(&ps,BUTTON_ATTACK,WP_LIGHTNING,100);assert(ps.ammo[WP_LIGHTNING]==445 && !ps.qceChargeMs);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_LIGHTNING;BG_QceAddWeapon(&ps,WP_LIGHTNING,8);
 for(i=0;i<6;i++)action(&ps,BUTTON_ATTACK,WP_LIGHTNING,100);action(&ps,0,WP_LIGHTNING,10);
 assert(ps.ammo[WP_LIGHTNING]==0 && BG_QceMagazine(&ps,WP_LIGHTNING)==0 && ps.eventParms[(ps.eventSequence-1)&1]==1);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_LIGHTNING;BG_QceAddWeapon(&ps,WP_LIGHTNING,500);BG_QceAddWeapon(&ps,WP_MACHINEGUN,120);
 action(&ps,BUTTON_ATTACK,WP_LIGHTNING,100);action(&ps,0,WP_MACHINEGUN,16);assert(!ps.qceChargeMs && ps.ammo[WP_LIGHTNING]==500);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_LIGHTNING;BG_QceAddWeapon(&ps,WP_LIGHTNING,500);
 action(&ps,BUTTON_ATTACK,WP_LIGHTNING,100);action(&ps,BUTTON_QCE_MELEE,WP_LIGHTNING,16);assert(!ps.qceChargeMs && ps.ammo[WP_LIGHTNING]==500 && ps.weaponstate==WEAPON_MELEEING);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_LIGHTNING;BG_QceAddWeapon(&ps,WP_LIGHTNING,500);BG_QceSetGrenadeCount(&ps,0,2);
 action(&ps,BUTTON_ATTACK,WP_LIGHTNING,100);action(&ps,BUTTON_QCE_GRENADE,WP_LIGHTNING,16);
 assert(!ps.qceChargeMs && ps.ammo[WP_LIGHTNING]==500 && BG_QceGrenadeCount(&ps,0)==1);
 ps.qceChargeMs=600;ps.stats[STAT_HEALTH]=0;action(&ps,0,WP_LIGHTNING,16);assert(!ps.qceChargeMs);
 heat=10000;remainder=999;locked=1;BG_QceCoolWeapon(WP_LIGHTNING,500,600000,&heat,&remainder,&locked);assert(!heat && !remainder && !locked);
 init(&ps,0);ps.qceHeat[0]=1234;BG_QceCoolWeapons(&ps,1000);assert(ps.qceHeat[0]==1234); /* Stock behavior unchanged. */
 puts("PASS: imported heat values, strict recovery threshold, fractional/frame-independent cooling, battery-age penalty, holstered cooling, inventory resets, predicted overheat fire lock, ammo conservation, charge release/consumption, cancellation, melee and stock fallback");return 0;
}
