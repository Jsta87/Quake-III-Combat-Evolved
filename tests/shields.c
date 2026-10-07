#include "g_local.h"
#include "bg_qce_shield.h"
#include <assert.h>
#include <stdio.h>
extern int CheckArmor(gentity_t *, int, int);
int main(void) {
 gentity_t ent;gclient_t client;int next=5000,save;
 memset(&ent,0,sizeof(ent));memset(&client,0,sizeof(client));ent.client=&client;
 client.ps.stats[STAT_QCE_COMBAT]=1;client.ps.stats[STAT_QCE_SHIELD]=100;
 save=CheckArmor(&ent,30,0);assert(save==30 && client.ps.stats[STAT_QCE_SHIELD]==70);
 save=CheckArmor(&ent,90,0);assert(save==70 && 90-save==20);
 assert(CheckArmor(&ent,10,0)==0);
 QCE_ShieldRecharge(&client.ps.stats[STAT_QCE_SHIELD],&next,4999,1);
 assert(client.ps.stats[STAT_QCE_SHIELD]==0);
 QCE_ShieldRecharge(&client.ps.stats[STAT_QCE_SHIELD],&next,5000,1);
 assert(client.ps.stats[STAT_QCE_SHIELD]==2 && next==5100);
 QCE_ShieldRecharge(&client.ps.stats[STAT_QCE_SHIELD],&next,6000,1);
 assert(client.ps.stats[STAT_QCE_SHIELD]==22 && next==6100);
 next=11000; /* a new hit restarts the recharge delay */
 QCE_ShieldRecharge(&client.ps.stats[STAT_QCE_SHIELD],&next,10000,1);
 assert(client.ps.stats[STAT_QCE_SHIELD]==22);
 QCE_ShieldRecharge(&client.ps.stats[STAT_QCE_SHIELD],&next,12000,0);
 assert(client.ps.stats[STAT_QCE_SHIELD]==22);
 QCE_ShieldRecharge(&client.ps.stats[STAT_QCE_SHIELD],&next,20000,1);
 assert(client.ps.stats[STAT_QCE_SHIELD]==100);
 assert(CheckArmor(&ent,100,DAMAGE_NO_ARMOR)==0 && client.ps.stats[STAT_QCE_SHIELD]==100);
 assert(CheckArmor(&ent,0,0)==0);
 client.ps.stats[STAT_QCE_COMBAT]=0;client.ps.stats[STAT_ARMOR]=100;
 assert(CheckArmor(&ent,30,0)==20 && client.ps.stats[STAT_ARMOR]==80);
 puts("PASS: actual CheckArmor shield absorption, health overflow, delay, recharge cadence/cap, death, bypass and stock armor fallback");
 return 0;
}
