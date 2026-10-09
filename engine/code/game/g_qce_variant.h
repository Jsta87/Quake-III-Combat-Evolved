/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef G_QCE_VARIANT_H
#define G_QCE_VARIANT_H
enum {
 GV_MOVE, GV_JUMP, GV_GRAVITY, GV_SHIELD, GV_HEALTH, GV_DAMAGE, GV_MELEE, GV_GRENADE_DAMAGE,
 GV_RECHARGE_DELAY, GV_RECHARGE_RATE, GV_FRAGS, GV_PLASMAS, GV_GRENADES, GV_INFINITE_AMMO,
 GV_INFINITE_GRENADES, GV_PICKUP, GV_DROP, GV_RESPAWN, GV_SUICIDE_PENALTY, GV_WEAPON_RESPAWN,
 GV_FRIENDLY_FIRE, GV_SCORE_LIMIT, GV_TIME_LIMIT, GV_INVISIBLE, GV_MAX_HELD, GV_COUNT
};
extern float qceVariantValues[GV_COUNT];
#define GV(i) (qceVariantValues[(i)])
void G_QceVariantRegister(void);
void G_QceVariantUpdate(void);
qboolean G_QceVariantCommand(const char *command);
void G_QceVariantSpawn(playerState_t *ps);
void G_QceVariantPlayer(playerState_t *ps);
gitem_t *G_QceVariantMapWeapon(gitem_t *item);
void G_QceVariantRecharge(gentity_t *ent);
#endif
