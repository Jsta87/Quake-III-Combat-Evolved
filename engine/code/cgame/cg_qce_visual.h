/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CG_QCE_VISUAL_H
#define CG_QCE_VISUAL_H
float QCE_ShieldFade(int remaining,int broken);
int QCE_DeathStart(int now,int serverTime);
void QCE_ViewSway(qceViewPlayback_t *p,int now,const vec3_t angles);
#endif
