#include "g_local.h"
#include "bg_qce_shield.h"
#include <assert.h>
#include <stdio.h>
extern int CheckArmor(gentity_t *, int, int);
int main(void) {
 gentity_t ent;gclient_t client;int next=6000,save,remainder=0,shield=0,other=0,next2=6000,rem2=0,i;
 memset(&ent,0,sizeof(ent));memset(&client,0,sizeof(client));ent.client=&client;
 assert(QCE_SHIELD_MAX==75 && QCE_SHIELD_DELAY==6000);
 client.ps.stats[STAT_QCE_COMBAT]=1;client.ps.stats[STAT_QCE_SHIELD]=75;
 save=CheckArmor(&ent,25,0);assert(save==25 && client.ps.stats[STAT_QCE_SHIELD]==50);
 save=CheckArmor(&ent,70,0);assert(save==50 && 70-save==20);
 assert(CheckArmor(&ent,10,0)==0);
 QCE_ShieldRecharge(&shield,&next,&remainder,5999,1);assert(shield==0);
 QCE_ShieldRecharge(&shield,&next,&remainder,6000,1);assert(shield==0);
 QCE_ShieldRecharge(&shield,&next,&remainder,7000,1);assert(shield==18 && remainder==3000);
 QCE_ShieldRecharge(&shield,&next,&remainder,8000,1);assert(shield==37 && remainder==2000);
 for(i=6000;i<=8000;i+=20)QCE_ShieldRecharge(&other,&next2,&rem2,i,1);
 assert(other==shield && rem2==remainder); /* frame cadence cannot change the rate */
 QCE_ShieldRecharge(&shield,&next,&remainder,8500,0);assert(shield==37);
 QCE_ShieldRecharge(&shield,&next,&remainder,10000,1);assert(shield==75);
 shield=0;next=11000;remainder=0;QCE_ShieldRecharge(&shield,&next,&remainder,10000,1);assert(shield==0);
 QCE_ShieldRecharge(&shield,&next,&remainder,20000,1);assert(shield==75);
 client.ps.stats[STAT_QCE_SHIELD]=75;
 assert(CheckArmor(&ent,100,DAMAGE_NO_ARMOR)==0 && client.ps.stats[STAT_QCE_SHIELD]==75);
 assert(CheckArmor(&ent,0,0)==0);
 client.ps.stats[STAT_QCE_COMBAT]=0;client.ps.stats[STAT_ARMOR]=100;
 assert(CheckArmor(&ent,30,0)==20 && client.ps.stats[STAT_ARMOR]==80);
 puts("PASS: tag-derived 75 shields, six-second delay, exact four-second fractional recharge, frame-cadence independence, death, overflow, bypass and stock armor");
 return 0;
}
