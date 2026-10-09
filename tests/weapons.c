#define QCE_WEAPON_TEST
#include "movement.c"
static void action(playerState_t *ps, int buttons, int weapon, int msec) {
 pmove_t pm;memset(&pm,0,sizeof(pm));pm.ps=ps;pm.trace=trace;
 pm.pointcontents=contents;pm.tracemask=MASK_PLAYERSOLID;
 pm.cmd.serverTime=ps->commandTime+msec;pm.cmd.buttons=buttons;pm.cmd.weapon=weapon;
 Pmove(&pm);
}
static void advance(playerState_t *ps,int buttons,int frames) {
 int i;for(i=0;i<frames;i++)action(ps,buttons,WP_MACHINEGUN,16);
}
static void reload_interrupt_tests(void) {
 playerState_t ps,predicted;int i;
 for(i=0;i<2;i++) {
  init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_MACHINEGUN,120);BG_QceAddWeapon(&ps,WP_SHOTGUN,24);
  ps.stats[STAT_QCE_MAG0]=10;action(&ps,BUTTON_QCE_RELOAD,WP_MACHINEGUN,16);
  if(i)while(ps.qceReloadCommit>0)action(&ps,0,WP_MACHINEGUN,16);
  predicted=ps;action(&ps,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);action(&predicted,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);
  assert(!memcmp(&ps,&predicted,sizeof(ps)));assert(ps.weaponstate==WEAPON_MELEEING);
  assert(BG_QceMagazine(&ps,WP_MACHINEGUN)==(i?60:10));assert(ps.ammo[WP_MACHINEGUN]==120);
 }
}
static void inventory_limits(void) {
 playerState_t ps,predicted;int limit,w,count;
 for(limit=1;limit<=9;limit++) {
  memset(&ps,0,sizeof(ps));ps.stats[STAT_QCE_COMBAT]=1;ps.qceMaxHeldWeapons=limit;count=0;
  for(w=WP_MACHINEGUN;w<=WP_BFG;w++) {
   if(BG_QceAddWeapon(&ps,w,BG_QceWeaponDef(w)->ammo_initial))count++;
  }
  assert(count==(limit>=8?8:limit));
  for(w=WP_MACHINEGUN;w<=WP_BFG;w++)if(BG_QceSlot(&ps,w)>=0) {
   int slot=BG_QceSlot(&ps,w);ps.weapon=w;predicted=ps;
   if(BG_QceWeaponDef(w)->reload_rounds>0) {BG_QceSetMagazine(&ps,slot,0);BG_QceSetMagazine(&predicted,slot,0);}
   BG_QceReload(&ps);BG_QceReload(&predicted);assert(!memcmp(&ps,&predicted,sizeof(ps)));
   assert(BG_QceMagazine(&ps,w)>0);
  }
  assert(BG_QceRemoveWeapon(&ps,WP_MACHINEGUN)>0);
  assert(BG_QceAddWeapon(&ps,WP_MACHINEGUN,10));assert(BG_QceMagazine(&ps,WP_MACHINEGUN)==10);
 }
}
int main(void) {
 playerState_t ps,predicted;entityState_t item;int seq,i;
 reload_interrupt_tests();inventory_limits();
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;
 assert(BG_QceAddWeapon(&ps,WP_MACHINEGUN,120));
 assert(BG_QceAddWeapon(&ps,WP_SHOTGUN,10));
 assert(!BG_QceAddWeapon(&ps,WP_ROCKET_LAUNCHER,2));
 assert(BG_QceMagazine(&ps,WP_MACHINEGUN)==60 && ps.ammo[WP_MACHINEGUN]==120);
 predicted=ps;
 action(&ps,BUTTON_ATTACK,WP_MACHINEGUN,16);
 action(&predicted,BUTTON_ATTACK,WP_MACHINEGUN,16);
 assert(!memcmp(&ps,&predicted,sizeof(ps)));
 assert(ps.ammo[WP_MACHINEGUN]==119 && BG_QceMagazine(&ps,WP_MACHINEGUN)==59);
 advance(&ps,0,10);
 action(&ps,BUTTON_QCE_RELOAD,WP_MACHINEGUN,16);
 assert(ps.weaponstate==WEAPON_RELOADING);
 seq=ps.eventSequence;
 action(&ps,BUTTON_ATTACK,WP_MACHINEGUN,16);
 assert(ps.weapon==WP_MACHINEGUN && ps.weaponstate==WEAPON_RELOADING);
 assert(!(ps.eFlags&EF_FIRING));
 ps.stats[STAT_QCE_GRENADES]=2;
 action(&ps,BUTTON_QCE_GRENADE,WP_MACHINEGUN,16);
 assert((ps.stats[STAT_QCE_GRENADES]&7)==1 && ps.eventSequence==seq+1);
 assert(ps.weaponstate==WEAPON_READY && ps.qceReloadCommit==0);
 assert(BG_QceMagazine(&ps,WP_MACHINEGUN)==59 && ps.ammo[WP_MACHINEGUN]==119);
 advance(&ps,0,60);action(&ps,BUTTON_QCE_RELOAD,WP_MACHINEGUN,16);
 advance(&ps,0,240);
 assert(ps.weaponstate==WEAPON_READY && BG_QceMagazine(&ps,WP_MACHINEGUN)==60);
 assert(ps.ammo[WP_MACHINEGUN]==119); /* cancelled/restarted reload conserves ammo */
 ps.stats[STAT_QCE_MAG0]=0;ps.ammo[WP_MACHINEGUN]=3;
 action(&ps,0,WP_MACHINEGUN,16);assert(ps.weaponstate==WEAPON_RELOADING);
 advance(&ps,0,240);assert(BG_QceMagazine(&ps,WP_MACHINEGUN)==3);
 ps.stats[STAT_QCE_GRENADES]=2;seq=ps.eventSequence;
 action(&ps,BUTTON_QCE_GRENADE,WP_MACHINEGUN,16);
 assert((ps.stats[STAT_QCE_GRENADES]&7)==1 && ps.eventSequence==seq+1);
 assert(ps.events[seq&(MAX_PS_EVENTS-1)]==EV_QCE_GRENADE);
 advance(&ps,BUTTON_QCE_GRENADE,100);
 assert((ps.stats[STAT_QCE_GRENADES]&7)==1 && ps.eventSequence==seq+1);
 assert(ps.ammo[WP_MACHINEGUN]==3);
 action(&ps,0,WP_MACHINEGUN,16);action(&ps,BUTTON_QCE_GRENADE,WP_MACHINEGUN,16);
 assert((ps.stats[STAT_QCE_GRENADES]&7)==0);
 advance(&ps,0,100);seq=ps.eventSequence;
 action(&ps,BUTTON_QCE_GRENADE,WP_MACHINEGUN,16);assert(ps.eventSequence==seq);
 assert(BG_QceRemoveWeapon(&ps,WP_MACHINEGUN)==3);
 assert(BG_QceSlot(&ps,WP_MACHINEGUN)<0 && !(ps.stats[STAT_WEAPONS]&(1<<WP_MACHINEGUN)));
 assert(BG_QceAddWeapon(&ps,WP_ROCKET_LAUNCHER,2));
 assert(!BG_QceAddWeapon(&ps,WP_RAILGUN,5));
 memset(&item,0,sizeof(item));item.modelindex=BG_FindItemForWeapon(WP_SHOTGUN)-bg_itemlist;
 item.otherEntityNum=ps.clientNum;item.time=2000;ps.commandTime=1000;
 assert(!BG_CanItemBeGrabbed(GT_FFA,&item,&ps));
 ps.commandTime=2000;assert(BG_CanItemBeGrabbed(GT_FFA,&item,&ps));
 init(&ps,0);action(&ps,BUTTON_QCE_RELOAD,WP_MACHINEGUN,16);
 assert(ps.weaponstate!=WEAPON_RELOADING);
 action(&ps,BUTTON_ATTACK,WP_MACHINEGUN,16);assert(ps.ammo[WP_MACHINEGUN]==99);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_MACHINEGUN,1);
 for(i=0;i<30;i++) action(&ps,BUTTON_ATTACK,WP_MACHINEGUN,16);
 assert(ps.ammo[WP_MACHINEGUN]==0 && BG_QceMagazine(&ps,WP_MACHINEGUN)==0);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_MACHINEGUN,60);
 ps.stats[STAT_QCE_GRENADES]=2;seq=ps.eventSequence;
 action(&ps,BUTTON_QCE_MELEE|BUTTON_ATTACK,WP_MACHINEGUN,16);
 assert(ps.weapon==WP_MACHINEGUN && ps.weaponstate==WEAPON_MELEEING);
 assert(!(ps.eFlags&EF_FIRING) && ps.ammo[WP_MACHINEGUN]==60);
 assert(ps.events[seq&(MAX_PS_EVENTS-1)]==EV_QCE_MELEE && ps.eventSequence==seq+1);
 action(&ps,BUTTON_QCE_MELEE|BUTTON_QCE_RELOAD|BUTTON_ATTACK,WP_SHOTGUN,16);
 assert(ps.weapon==WP_MACHINEGUN && ps.weaponstate==WEAPON_MELEEING && ps.eventSequence==seq+1);
 advance(&ps,BUTTON_QCE_MELEE,100);
 assert(ps.weaponstate==WEAPON_READY && ps.eventSequence==seq+2);
 assert((ps.stats[STAT_QCE_GRENADES]&7)==2 && BG_QceMagazine(&ps,WP_MACHINEGUN)==60);
 action(&ps,BUTTON_QCE_MELEE|BUTTON_QCE_GRENADE,WP_MACHINEGUN,16);
 assert((ps.stats[STAT_QCE_GRENADES]&QCE_MELEE_HELD) && (ps.stats[STAT_QCE_GRENADES]&7)==1);
 advance(&ps,BUTTON_QCE_MELEE|BUTTON_QCE_GRENADE,100);
 assert(ps.eventSequence==seq+3); /* grenade must not clear the held melee latch */
 action(&ps,0,WP_MACHINEGUN,16);action(&ps,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);
 assert(ps.eventSequence==seq+4);
 init(&ps,0);seq=ps.eventSequence;action(&ps,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);
 assert(ps.eventSequence==seq); /* disabled profile preserves stock behavior */
 ps.stats[STAT_QCE_COMBAT]=1;ps.stats[STAT_HEALTH]=0;action(&ps,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);
 assert(ps.eventSequence==seq);

 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_MACHINEGUN,60);
 BG_QceSetGrenadeCount(&ps,0,3);BG_QceSetGrenadeCount(&ps,1,2);BG_QceToggleGrenade(&ps);
 seq=ps.eventSequence;action(&ps,BUTTON_QCE_GRENADE,WP_MACHINEGUN,16);
 assert(BG_QceGrenadeCount(&ps,0)==3 && BG_QceGrenadeCount(&ps,1)==1 && BG_QceGrenadeType(&ps)==1);
 assert(ps.eventParms[seq&(MAX_PS_EVENTS-1)]==1);
 advance(&ps,0,60);BG_QceToggleGrenade(&ps);action(&ps,BUTTON_QCE_GRENADE,WP_MACHINEGUN,16);
 assert(BG_QceGrenadeCount(&ps,0)==2 && BG_QceGrenadeCount(&ps,1)==1);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_MACHINEGUN,60);
 meleeTarget=100;seq=ps.eventSequence;
 action(&ps,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);assert(ps.velocity[0]<1); /* CE default never lunges */
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=3;BG_QceAddWeapon(&ps,WP_MACHINEGUN,60);seq=ps.eventSequence;predicted=ps;
 action(&ps,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);action(&predicted,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);
 assert(!memcmp(&ps,&predicted,sizeof(ps)) && ps.velocity[0]>490 && ps.eventSequence==seq+1);
 for(i=0;i<5;i++)action(&ps,0,WP_MACHINEGUN,16);
 assert(ps.eventSequence==seq+1);
 for(i=0;i<3;i++)action(&ps,0,WP_MACHINEGUN,16);
 assert(ps.eventSequence==seq+2 && ps.events[(seq+1)&(MAX_PS_EVENTS-1)]==EV_QCE_MELEE_STRIKE);
 assert(ps.ammo[WP_MACHINEGUN]==60 && !(ps.stats[STAT_QCE_GRENADES]&32));
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_MACHINEGUN,60);
 meleeWall=40;seq=ps.eventSequence;action(&ps,BUTTON_QCE_MELEE,WP_MACHINEGUN,16);
 assert(ps.velocity[0]<1 && ps.eventSequence==seq+1);meleeTarget=meleeWall=10000;

 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_SHOTGUN;
 BG_QceAddWeapon(&ps,WP_SHOTGUN,24);ps.stats[STAT_QCE_MAG0]=0;
 action(&ps,BUTTON_QCE_RELOAD,WP_SHOTGUN,16);
 assert(ps.weaponstate==WEAPON_RELOAD_ENTER);
 for(i=0;i<57;i++)action(&ps,0,WP_SHOTGUN,16);
 assert(BG_QceMagazine(&ps,WP_SHOTGUN)==1 && ps.weaponstate==WEAPON_RELOADING && ps.ammo[WP_SHOTGUN]==24);
 action(&ps,BUTTON_ATTACK,WP_SHOTGUN,16);
 assert(ps.weaponstate==WEAPON_RELOAD_EXIT && ps.ammo[WP_SHOTGUN]==24);
 assert(BG_QceMagazine(&ps,WP_SHOTGUN)==1); /* closing must complete before firing */
 for(i=0;i<51;i++)action(&ps,BUTTON_ATTACK,WP_SHOTGUN,16);
 assert(BG_QceMagazine(&ps,WP_SHOTGUN)==0 && ps.ammo[WP_SHOTGUN]==23 && ps.weaponstate==WEAPON_FIRING);
 /* A reload switch cancels insertion and follows normal drop/raise timing. */
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;
 BG_QceAddWeapon(&ps,WP_MACHINEGUN,120);BG_QceAddWeapon(&ps,WP_SHOTGUN,24);ps.stats[STAT_QCE_MAG0]=10;
 action(&ps,BUTTON_QCE_RELOAD,WP_MACHINEGUN,16);assert(ps.weaponstate==WEAPON_RELOADING);
 action(&ps,0,WP_SHOTGUN,16);assert(ps.weaponstate==WEAPON_DROPPING && ps.stats[STAT_QCE_MAG0]==10);
 for(i=0;i<100;i++)action(&ps,0,WP_SHOTGUN,16);
 assert(ps.weapon==WP_SHOTGUN && ps.weaponstate==WEAPON_READY && ps.stats[STAT_QCE_MAG0]==10);
 /* Reloading a nearly full tube closes once and never mints reserve ammunition. */
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_SHOTGUN;BG_QceAddWeapon(&ps,WP_SHOTGUN,24);ps.stats[STAT_QCE_MAG0]=11;
 predicted=ps;
 action(&ps,BUTTON_QCE_RELOAD,WP_SHOTGUN,16);action(&predicted,BUTTON_QCE_RELOAD,WP_SHOTGUN,16);
 for(i=0;i<60;i++) {action(&ps,0,WP_SHOTGUN,16);action(&predicted,0,WP_SHOTGUN,16);assert(!memcmp(&ps,&predicted,sizeof(ps)));}
 assert(BG_QceMagazine(&ps,WP_SHOTGUN)==12 && ps.weaponstate==WEAPON_RELOAD_EXIT && ps.ammo[WP_SHOTGUN]==24);
 for(i=0;i<60;i++)action(&ps,0,WP_SHOTGUN,16);assert(ps.weaponstate==WEAPON_READY);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;ps.weapon=WP_PLASMAGUN;
 BG_QceAddWeapon(&ps,WP_PLASMAGUN,200);ps.stats[STAT_QCE_MAG0]=150;
 assert(!BG_QceCanReload(&ps));action(&ps,BUTTON_QCE_RELOAD,WP_PLASMAGUN,16);
 assert(ps.weaponstate!=WEAPON_RELOADING && ps.ammo[WP_PLASMAGUN]==200);
 init(&ps,0);ps.stats[STAT_QCE_COMBAT]=1;BG_QceAddWeapon(&ps,WP_MACHINEGUN,1000);
 assert(ps.ammo[WP_MACHINEGUN]==660 && BG_QceMagazine(&ps,WP_MACHINEGUN)==60);
 action(&ps,BUTTON_ATTACK,WP_MACHINEGUN,16);assert(BG_QceAmmoLimit(&ps,WP_MACHINEGUN)==659);
 BG_QceAddWeapon(&ps,WP_MACHINEGUN,20);assert(ps.ammo[WP_MACHINEGUN]==659);
 item.modelindex=BG_FindItemForWeapon(WP_MACHINEGUN)-bg_itemlist;item.time=0;
 assert(!BG_CanItemBeGrabbed(GT_FFA,&item,&ps));
 assert(BG_QceWeaponDef(WP_RAILGUN)->headshot_mode==2 && BG_QceWeaponDef(WP_BFG)->headshot_mode==1);
 {
  vec3_t start={10,20,30},end={10010,10020,30},direction;
  BG_QceRayEnd(start,end,WP_MACHINEGUN);VectorSubtract(end,start,direction);
  assert(fabs(VectorLength(direction)-3200)<0.01 && fabs(direction[0]-direction[1])<0.01);
  BG_QceRayEnd(start,end,WP_RAILGUN);VectorSubtract(end,start,direction);assert(fabs(VectorLength(direction)-80000)<0.1);
  VectorCopy(start,end);BG_QceRayEnd(start,end,WP_SHOTGUN);assert(VectorCompare(start,end));
 }
 puts("PASS: two-slot capacity, magazine/total ammo, reload timing, staged shotgun closing, switch cancellation, prediction and conservation, automatic reload, grenade press latch/count/events, slot removal, owner pickup delay, reload action lock, stock fallback, empty-ammo handling melee input/cooldown/action locks, delayed predicted lunge, blocked lunge and independent grenade types");
 return 0;
}
