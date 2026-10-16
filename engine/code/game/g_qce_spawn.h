/* SPDX-License-Identifier: GPL-3.0-only
 * Developer-only spawning at the server-authoritative crosshair target.
 * Included after CheatsOk/ConcatArgs in g_cmds.c; shared with the fixture.
 */
#ifndef G_QCE_SPAWN_H
#define G_QCE_SPAWN_H
static void QceSpawnRotatedBounds(const vec3_t mins,const vec3_t maxs,float yaw,vec3_t outMin,vec3_t outMax) {
 vec3_t angles={0,0,0},axis[3],point,world;int corner,i;
 angles[YAW]=yaw;AnglesToAxis(angles,axis);VectorSet(outMin,1e9f,1e9f,1e9f);VectorSet(outMax,-1e9f,-1e9f,-1e9f);
 for(corner=0;corner<8;corner++) {
  for(i=0;i<3;i++)point[i]=(corner&(1<<i))?maxs[i]:mins[i];
  VectorClear(world);for(i=0;i<3;i++)VectorMA(world,point[i],axis[i],world);
  for(i=0;i<3;i++){if(world[i]<outMin[i])outMin[i]=world[i];if(world[i]>outMax[i])outMax[i]=world[i];}
 }
}
static qboolean QceSpawnAimPoint(gentity_t *player,const vec3_t mins,const vec3_t maxs,vec3_t result) {
 vec3_t eye,end,forward,base,probe;trace_t hit,clear;float separation=1;int i,rise;
 VectorCopy(player->client->ps.origin,eye);eye[2]+=player->client->ps.viewheight;
 /* viewangles already contains the server's third-person crosshair correction. */
 AngleVectors(player->client->ps.viewangles,forward,NULL,NULL);VectorMA(eye,65536,forward,end);
 trap_Trace(&hit,eye,NULL,NULL,end,player->s.number,MASK_SOLID);
 if(hit.startsolid || hit.allsolid || hit.fraction==1 || (hit.surfaceFlags&SURF_SKY))return qfalse;
 for(i=0;i<3;i++)separation-=hit.plane.normal[i]*(hit.plane.normal[i]>=0?mins[i]:maxs[i]);
 VectorMA(hit.endpos,separation,hit.plane.normal,result);
 VectorCopy(result,base);VectorMA(hit.endpos,.5f,hit.plane.normal,probe);
 /* A terrain seam may rise above the tangent plane under a wide chassis.
    Lift locally by at most 32 units, with a clear path from the aim point. */
 for(rise=0;rise<=32;rise+=4) {
  VectorCopy(base,result);result[2]+=rise;
  trap_Trace(&clear,probe,NULL,NULL,result,player->s.number,MASK_SOLID);
  if(clear.startsolid || clear.allsolid || clear.fraction<1)continue;
  trap_Trace(&clear,result,mins,maxs,result,ENTITYNUM_NONE,MASK_PLAYERSOLID);
  if(!clear.startsolid && !clear.allsolid && clear.fraction==1)return qtrue;
 }
 return qfalse;
}
static void QceSpawnCommand(gentity_t *player,const char *argument) {
 static const char *weaponNames[]={"pistol","assaultrifle","shotgun","rocketlauncher","sniperrifle","plasmarifle","plasmapistol","needler"};
 static const int weaponIds[]={WP_BFG,WP_MACHINEGUN,WP_SHOTGUN,WP_ROCKET_LAUNCHER,WP_RAILGUN,WP_PLASMAGUN,WP_LIGHTNING,WP_GRENADE_LAUNCHER};
 char name[MAX_TOKEN_CHARS];const char *input=argument;int i,n=0,vehicle=0,freeCount=0;gitem_t *item=NULL;gentity_t *created;
 vec3_t mins={-ITEM_RADIUS,-ITEM_RADIUS,-ITEM_RADIUS},maxs={ITEM_RADIUS,ITEM_RADIUS,ITEM_RADIUS},worldMin,worldMax,point;
 if(!CheatsOk(player))return;
 if(player->client->sess.sessionTeam==TEAM_SPECTATOR || player->client->ps.qceVehicle){trap_SendServerCommand(player->s.number,"print \"Exit your vehicle and spawn as a player first.\n\"");return;}
 if(!Q_stricmpn(input,"weapon ",7))input+=7;else if(!Q_stricmpn(input,"vehicle ",8))input+=8;else if(!Q_stricmpn(input,"item ",5))input+=5;
 for(i=0;input[i] && n<(int)sizeof(name)-1;i++)if(input[i]!=' ' && input[i]!='_' && input[i]!='-')name[n++]=(input[i]>='A' && input[i]<='Z')?input[i]+'a'-'A':input[i];name[n]=0;
 if(!name[0] || !Q_stricmp(name,"help")) {
  trap_SendServerCommand(player->s.number,"print \"qce_spawn <name>: pistol, assault rifle, shotgun, rocket launcher, sniper rifle, plasma rifle, plasma pistol, needler, warthog, ghost, scorpion, or an item classname (item_health_large, ammo_shells, etc). Aim at a solid surface; sv_cheats required.\n\"");return;
 }
 if(!Q_stricmp(name,"warthog") || !Q_stricmp(name,"qcewarthog"))vehicle=1;
 else if(!Q_stricmp(name,"ghost") || !Q_stricmp(name,"qceghost"))vehicle=2;
 else if(!Q_stricmp(name,"scorpion") || !Q_stricmp(name,"qcescorpion"))vehicle=3;
 else {
  if(!Q_stricmp(name,"ar"))Q_strncpyz(name,"assaultrifle",sizeof(name));
  if(!Q_stricmp(name,"rocket"))Q_strncpyz(name,"rocketlauncher",sizeof(name));
  if(!Q_stricmp(name,"sniper"))Q_strncpyz(name,"sniperrifle",sizeof(name));
  for(i=0;i<8;i++)if(!Q_stricmp(name,weaponNames[i]))item=BG_FindItemForWeapon(weaponIds[i]);
  if(!item)for(i=1;i<bg_numItems;i++)if(!Q_stricmp(input,bg_itemlist[i].classname) || (bg_itemlist[i].pickup_name && !Q_stricmp(input,bg_itemlist[i].pickup_name))) {item=&bg_itemlist[i];break;}
  if(!item || item->giType==IT_TEAM || item->giType==IT_BAD || (item->giType==IT_WEAPON && (item->giTag==WP_GAUNTLET || item->giTag==WP_GRAPPLING_HOOK))) {
   trap_SendServerCommand(player->s.number,"print \"Unknown or unsupported spawn name. Use qce_spawn help.\n\"");return;
  }
 }
 /* Keep a small reserve so repeated development spawns cannot exhaust entities. */
 for(i=MAX_CLIENTS;i<ENTITYNUM_MAX_NORMAL;i++)if(!g_entities[i].inuse)freeCount++;
 if(freeCount<16){trap_SendServerCommand(player->s.number,"print \"Too many entities to spawn another object.\n\"");return;}
 if(vehicle==1){VectorSet(mins,-90,-50,-1);VectorSet(maxs,90,50,104);}
 if(vehicle==2){VectorSet(mins,-36,-48,-12);VectorSet(maxs,76,48,35);}
 if(vehicle==3){VectorSet(mins,-131,-95,-1);VectorSet(maxs,135,95,108);}
 QceSpawnRotatedBounds(mins,maxs,player->client->ps.viewangles[YAW],worldMin,worldMax);
 if(!QceSpawnAimPoint(player,worldMin,worldMax,point)){trap_SendServerCommand(player->s.number,"print \"Aim at a solid surface with enough free space for the object.\n\"");return;}
 if(vehicle) {
  created=G_Spawn();VectorCopy(point,created->s.origin);created->s.angles[YAW]=player->client->ps.viewangles[YAW];
  if(vehicle==1)SP_qce_warthog(created);else if(vehicle==2)SP_qce_ghost(created);else SP_qce_scorpion(created);
 } else {
  RegisterItem(item);SaveRegisteredItems();created=LaunchItem(item,point,vec3_origin);
  created->nextthink=0;created->think=NULL; /* Stay until picked up; no timed respawn. */
  if(item->giType==IT_WEAPON)created->count=BG_QceWeaponDef(item->giTag)->ammo_initial;
 }
 trap_SendServerCommand(player->s.number,va("print \"Spawned %s at %.1f %.1f %.1f\n\"",created->classname,point[0],point[1],point[2]));
}
#endif
