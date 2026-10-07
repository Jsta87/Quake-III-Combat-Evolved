/* Experimental shield rules; provisional values, not retail Halo parity. */
#ifndef BG_QCE_SHIELD_H
#define BG_QCE_SHIELD_H
#define QCE_SHIELD_MAX 100
#define QCE_SHIELD_DELAY 5000
#define QCE_SHIELD_TICK 100
#define QCE_SHIELD_STEP 2
int QCE_ShieldAbsorb(int *shield, int damage);
void QCE_ShieldRecharge(int *shield, int *nextTick, int now, int alive);
#endif
