/* Vitality/recharge parameters come from the generated shared player profile. */
#ifndef BG_QCE_SHIELD_H
#define BG_QCE_SHIELD_H
#define QCE_SHIELD_MAX (BG_QcePlayerDef()->shield)
#define QCE_SHIELD_DELAY (BG_QcePlayerDef()->shield_delay_ms)
int QCE_ShieldAbsorb(int *shield, int damage);
void QCE_ShieldRecharge(int *shield, int *nextTick, int *remainder, int now, int alive);
#endif
