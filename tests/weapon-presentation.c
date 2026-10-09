/* No Halo cache needed: exercise the client timing helpers directly. */
#include <assert.h>
#include <stdio.h>
#include "../engine/code/cgame/cg_local.h"
#include "../engine/code/qcommon/qce_color.h"
#include "../engine/code/cgame/cg_halo_present.h"
#include "../engine/code/cgame/cg_qce_visual.h"
int main(void) {
 qceViewClip_t clips[QCE_VIEW_CLIPS];
 qceViewPlayback_t p;
 qceViewInput_t in;
 qceViewSound_t sound;
 byte rgb[4]={1,2,3,4};
 int i,oldframe,frame;
 float backlerp;
 assert(QCE_ParseRGB("255 0 127",rgb) && rgb[0]==255 && rgb[1]==0 && rgb[2]==127 && rgb[3]==255);
 assert(!QCE_ParseRGB("256 0 0",rgb) && !QCE_ParseRGB("-1 0 0",rgb) && !QCE_ParseRGB("1 2",rgb) && !QCE_ParseRGB("1 2 3 extra",rgb));
 assert(rgb[0]==255 && rgb[2]==127);
 memset(&p,0,sizeof(p));memset(&in,0,sizeof(in));memset(&sound,0,sizeof(sound));
 for(i=0;i<QCE_VIEW_CLIPS;i++) {clips[i].first=i*100;clips[i].count=30;clips[i].fps=30;clips[i].loop=0;}
 in.weapon=WP_MACHINEGUN;in.state=WEAPON_RELOADING;in.now=1000;in.magazine=0;in.weaponTime=3000;in.phaseMs=3000;in.reloadRounds=60;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_RELOAD_EMPTY);
 in.now=1100;in.magazine=60;in.weaponTime=2900;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_RELOAD_EMPTY && p.start==1000); /* latch empty reload */
 in.state=WEAPON_DROPPING;in.now=1200;in.weaponTime=233;in.phaseMs=233;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_PUTAWAY && p.start==1200);
 in.weapon=WP_SHOTGUN;in.state=WEAPON_RELOAD_ENTER;in.phaseMs=500;in.weaponTime=480;in.now=2000;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_RELOAD_ENTER && p.start==1980);
 in.state=WEAPON_RELOADING;in.phaseMs=400;in.weaponTime=400;in.now=2500;in.reloadRounds=1;in.magazine=1;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_RELOAD_FULL && p.start==2500);
 in.now=2900;in.magazine=2;in.weaponTime=400;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_RELOAD_FULL && p.start==2900); /* next insertion */
 in.state=WEAPON_RELOAD_EXIT;in.phaseMs=800;in.weaponTime=800;in.now=3000;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_RELOAD_EXIT);
 in.weapon=WP_LIGHTNING;in.state=WEAPON_READY;in.phaseMs=0;in.charge=300;in.chargeMs=600;in.now=4000;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_CHARGE_ENTER);
 in.charge=600;in.now=4300;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_CHARGE);
 in.charge=0;in.hot=1;in.chargedFire=1;in.fireTime=4400;in.now=4400;in.state=WEAPON_FIRING;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_CHARGED_HOT && p.start==4400); /* immediate cooling on final shot */
 sound.frame=0;sound.count=1;assert(QCE_ViewSoundDue(&p,0,&clips[QCE_VIEW_CHARGED_HOT],&sound,0));
 in.now=4500;in.state=WEAPON_READY;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_CHARGED_HOT && p.start==4400);
 assert(!QCE_ViewSoundDue(&p,100,&clips[QCE_VIEW_CHARGED_HOT],&sound,0)); /* firing cooldown cannot replay heat audio */
 in.now=6500;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_HOT_IDLE);
 in.hot=0;in.now=6600;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_RECOVER);
 in.now=7601;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_IDLE);
 in.moving=1;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_IDLE); /* movement is additive */
 in.moving=0;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_IDLE);
 QCE_ViewFrames(&clips[0],QCE_VIEW_IDLE,1017,0,&oldframe,&frame,&backlerp);
 assert(oldframe==0 && frame==1 && backlerp>0.48f && backlerp<0.5f);
 QCE_ViewFrames(&clips[1],QCE_VIEW_FIRE,9999,0,&oldframe,&frame,&backlerp);
 assert(oldframe==129 && frame==129 && backlerp==1);
 QCE_ViewFrames(&clips[QCE_VIEW_CHARGE],QCE_VIEW_CHARGE,10000,0,&oldframe,&frame,&backlerp);
 assert(oldframe==929 && frame==929 && backlerp==0); /* hold ending charge pose */
 QCE_ViewFrames(&clips[2],QCE_VIEW_READY,500,1000,&oldframe,&frame,&backlerp);
 assert(oldframe==214 && frame==215 && backlerp==0.5f);
 p.clip=QCE_VIEW_FIRE;p.start=8000;sound.frame=6;sound.count=1;
 assert(!QCE_ViewSoundDue(&p,199,&clips[1],&sound,0));
 assert(QCE_ViewSoundDue(&p,200,&clips[1],&sound,0));
 assert(!QCE_ViewSoundDue(&p,900,&clips[1],&sound,0));
 p.start=8100;assert(QCE_ViewSoundDue(&p,200,&clips[1],&sound,0)); /* repeated shot */
 sound.loop=1;p.start=8200;assert(!QCE_ViewSoundDue(&p,900,&clips[1],&sound,0));
 p.clip=QCE_VIEW_IDLE;sound.count=0;assert(!QCE_ViewSoundDue(&p,900,&clips[0],&sound,0));
 memset(&p,0,sizeof(p));memset(&in,0,sizeof(in));in.weapon=WP_SHOTGUN;in.state=WEAPON_READY;in.now=9000;
 assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_READY && p.start==9000);
 in.now=9016;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_READY && p.start==9000);
 in.now=10001;assert(QCE_ViewSelect(&p,&in,clips)==QCE_VIEW_IDLE);
 memset(&p,0,sizeof(p));QCE_ViewMovement(&p,1000,1,WEAPON_READY);assert(p.moveWeight==1 && p.moveElapsed==0);
 QCE_ViewMovement(&p,1100,1,WEAPON_RELOADING);assert(p.moveWeight==1 && p.moveElapsed==100);
 QCE_ViewMovement(&p,1150,0,WEAPON_READY);assert(p.moveWeight==0.75f && p.moveElapsed==100);
 QCE_ViewMovement(&p,1300,0,WEAPON_READY);assert(p.moveWeight==0);
 QCE_ViewMovement(&p,1400,1,WEAPON_READY);assert(p.moveWeight==1 && p.moveElapsed==0);
 QCE_ViewMovement(&p,1450,0,WEAPON_FIRING);assert(p.moveWeight==0);
 {
  qceViewPlayback_t a={0},b={0};vec3_t aim={0,0,0};int n;
  assert(QCE_ShieldFade(584,0)>.97f && QCE_ShieldFade(584,0)<1);
  assert(QCE_ShieldFade(300,0)==.5f && QCE_ShieldFade(450,1)==.5f);
  assert(QCE_ShieldFade(-1,0)==0 && QCE_ShieldFade(2000,1)==1);
  assert(QCE_DeathStart(1000,750)==750 && QCE_DeathStart(2000,750)==750);
  assert(QCE_DeathStart(1000,0)==1000 && QCE_DeathStart(1000,1001)==1000);
  QCE_ViewSway(&a,1000,aim);QCE_ViewSway(&b,1000,aim);
  for(n=1;n<=30;n++){aim[YAW]=n;QCE_ViewSway(&a,1000+n*32,aim);}
  for(n=1;n<=60;n++){aim[YAW]=n*.5f;QCE_ViewSway(&b,1000+n*16,aim);}
  assert(a.swayOffset[YAW]>0 && fabs(a.swayOffset[YAW]-b.swayOffset[YAW])<.001f);
  for(n=1;n<=30;n++)QCE_ViewSway(&a,1960+n*32,aim);
  assert(fabs(a.swayOffset[YAW])<.001f);
  QCE_ViewSway(&a,4000,aim);assert(a.swayOffset[YAW]==0); /* pause resets */
  aim[YAW]=179;QCE_ViewSway(&a,5000,aim);aim[YAW]=-179;QCE_ViewSway(&a,5016,aim);assert(a.swayOffset[YAW]>0); /* wrap */
 }
 puts("PASS: persistent shield fade, shared corpse death timeline, frame-independent directional sway, reload latching/insertion/cancellation, charge/fire/hot/recovery priority, frame interpolation and once-per-instance sound events");
 return 0;
}
