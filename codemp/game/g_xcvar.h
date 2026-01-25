/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, SerenityJediEngine2026 contributors

This file is part of the SerenityJediEngine2026 source code.

SerenityJediEngine2026 is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#ifdef XCVAR_PROTO
#define XCVAR_DEF( name, defVal, update, flags, announce ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
#define XCVAR_DEF( name, defVal, update, flags, announce ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
#define XCVAR_DEF( name, defVal, update, flags, announce ) { & name , #name , defVal , update , flags , announce },
#endif

XCVAR_DEF(bg_fighterAltControl, "0", NULL, CVAR_SYSTEMINFO, qtrue)
XCVAR_DEF(capturelimit, "8", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue)
XCVAR_DEF(com_optvehtrace, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(d_altRoutes, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_asynchronousGroupAI, "1", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_break, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_JediAI, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_noGroupAI, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_noroam, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_npcai, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_npcaiming, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_npcfreeze, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_noIntermissionWait, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(d_patched, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_perPlayerGhoul2, "0", NULL, CVAR_CHEAT, qtrue)
XCVAR_DEF(d_powerDuelPrint, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(d_projectileGhoul2Collision, "1", NULL, CVAR_CHEAT, qtrue)
XCVAR_DEF(d_saberAlwaysBoxTrace, "0", NULL, CVAR_CHEAT, qtrue)
XCVAR_DEF(d_saberBoxTraceSize, "0", NULL, CVAR_CHEAT, qtrue)
XCVAR_DEF(d_saberCombat, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(d_saberGhoul2Collision, "1", NULL, CVAR_CHEAT, qtrue)
XCVAR_DEF(d_saberInterpolate, "2", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(d_saberKickTweak, "1", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(d_saberStanceDebug, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(d_siegeSeekerNPC, "0", NULL, CVAR_CHEAT, qtrue)
XCVAR_DEF(dedicated, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(developer, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(dmflags, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue)
XCVAR_DEF(duel_fraglimit, "10", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue)
XCVAR_DEF(fraglimit, "20", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue)
XCVAR_DEF(g_adaptRespawn, "1", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_allowDuelSuicide, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_allowHighPingDuelist, "1", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_allowNPC, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_allowTeamVote, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_allowVote, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_antiFakePlayer, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_armBreakage, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_austrian, "0", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_autoMapCycle, "0", NULL, CVAR_ARCHIVE | CVAR_NORESTART, qtrue)
XCVAR_DEF(g_banIPs, "", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_charRestrictRGB, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_duelWeaponDisable, "1", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qtrue)
XCVAR_DEF(g_debugAlloc, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_debugDamage, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_debugMove, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_debugSaberLocks, "0", NULL, CVAR_CHEAT, qfalse)
XCVAR_DEF(g_debugServerSkel, "0", NULL, CVAR_CHEAT, qfalse)
#ifdef _DEBUG
XCVAR_DEF(g_disableServerG2, "0", NULL, CVAR_NONE, qtrue)
#endif
XCVAR_DEF(g_dismember, "80", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_doWarmup, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_corpseRemovalTime, "10", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_ff_objectives, "0", NULL, CVAR_CHEAT | CVAR_NORESTART, qtrue)
XCVAR_DEF(g_filterBan, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_forceBasedTeams, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qfalse)
XCVAR_DEF(g_forceClientUpdateRate, "250", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_forceDodge, "1", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_forcePowerDisable, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qtrue)
XCVAR_DEF(g_forceRegenTime, "400", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_forceRespawn, "60", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_fraglimitVoteCorrection, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_friendlyFire, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_friendlySaber, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_g2TraceLod, "3", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_gametype, "0", NULL, CVAR_SERVERINFO | CVAR_LATCH, qfalse)
XCVAR_DEF(g_gravity, "800", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_inactivity, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_jediVmerc, "0", NULL, CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_knockback, "1000", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_locationBasedDamage, "1", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_log, "games.log", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_logClientInfo, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_logSync, "0", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_maxConnPerIP, "3", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_maxForceRank, "7", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qfalse)
XCVAR_DEF(g_maxGameClients, "0", NULL, CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_maxHolocronCarry, "3", NULL, CVAR_LATCH, qfalse)
XCVAR_DEF(g_motd, "", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_needpass, "0", NULL, CVAR_SERVERINFO | CVAR_ROM, qfalse)
XCVAR_DEF(g_noSpecMove, "0", NULL, CVAR_SERVERINFO, qtrue)
XCVAR_DEF(g_npcspskill, "5", NULL, CVAR_ARCHIVE | CVAR_INTERNAL, qfalse)
XCVAR_DEF(g_password, "", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_powerDuelEndHealth, "90", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_powerDuelStartHealth, "150", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_privateDuel, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_randFix, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_restarted, "0", NULL, CVAR_ROM, qfalse)
XCVAR_DEF(g_saberDamageScale, "1", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue)
#ifndef FINAL_BUILD
XCVAR_DEF(g_saberDebugPrint, "0", NULL, CVAR_CHEAT, qfalse)
#endif
XCVAR_DEF(g_saberDmgDelay_Idle, "350", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_saberDmgDelay_Wound, "100", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_saberDmgVelocityScale, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_saberLockFactor, "2", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_saberLocking, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_saberRealisticCombat, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_saberRestrictForce, "0", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_saberTraceSaberFirst, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_saberWallDamageScale, "0.4", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_securityLog, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_showDuelHealths, "1", NULL, CVAR_SERVERINFO, qfalse)
XCVAR_DEF(g_siegeRespawn, "20", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_siegeTeam1, "none", NULL, CVAR_ARCHIVE | CVAR_SERVERINFO, qfalse)
XCVAR_DEF(g_siegeTeam2, "none", NULL, CVAR_ARCHIVE | CVAR_SERVERINFO, qfalse)
XCVAR_DEF(g_siegeTeamSwitch, "1", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_slowmoDuelEnd, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_smoothClients, "1", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_spawnInvulnerability, "3000", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_speed, "250", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_statLog, "0", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_statLogFile, "statlog.log", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_stepSlideFix, "1", NULL, CVAR_SERVERINFO, qtrue)
XCVAR_DEF(g_synchronousClients, "0", NULL, CVAR_SYSTEMINFO, qfalse)
XCVAR_DEF(g_teamAutoJoin, "0", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_teamForceBalance, "0", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_timeouttospec, "70", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_userinfoValidate, "25165823", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_useWhileThrowing, "1", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_voteDelay, "3000", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_warmup, "20", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_weaponDisable, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qtrue)
XCVAR_DEF(g_weaponRespawn, "5", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(gamedate, SOURCE_DATE, NULL, CVAR_ROM, qfalse)
XCVAR_DEF(gamename, GAMEVERSION, NULL, CVAR_SERVERINFO | CVAR_ROM, qfalse)
XCVAR_DEF(pmove_fixed, "0", NULL, CVAR_SYSTEMINFO | CVAR_ARCHIVE, qtrue)
XCVAR_DEF(pmove_float, "0", NULL, CVAR_SYSTEMINFO | CVAR_ARCHIVE, qtrue)
XCVAR_DEF(pmove_msec, "8", NULL, CVAR_SYSTEMINFO | CVAR_ARCHIVE, qtrue)
XCVAR_DEF(RMG, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(sv_cheats, "1", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(sv_fps, "40", NULL, CVAR_ARCHIVE | CVAR_SERVERINFO, qtrue)
XCVAR_DEF(sv_maxclients, "8", NULL, CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, qfalse)
XCVAR_DEF(timelimit, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue)
XCVAR_DEF(g_dodgeRegenTime, "1000", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_mishapRegenTime, "3000", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_debugviewlock, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_saberAnimSpeed, "1.0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(bot_thinklevel, "3", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(m_nerf, "0", NULL, CVAR_INTERNAL | CVAR_SERVERINFO, qtrue)
XCVAR_DEF(g_vehAutoAimLead, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_hookChangeProtectTime, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_grapple_shoot_speed, "900", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_allowROQ, "0", NULL, CVAR_NONE, qtrue)

XCVAR_DEF(storyhead, "", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(tier_storyinfo, "0", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(tiers_complete, "", NULL, CVAR_ARCHIVE, qfalse)

XCVAR_DEF(g_skipcutscenes, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_spmodel, "jan", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_spmodelrgb, "255 255 255", NULL, CVAR_ARCHIVE, qtrue)

XCVAR_DEF(g_lms, "0", NULL, CVAR_LATCH, qfalse)
XCVAR_DEF(g_lmslives, "1", NULL, CVAR_LATCH, qfalse)
XCVAR_DEF(g_liveExp, "5", NULL, CVAR_LATCH, qfalse)
XCVAR_DEF(g_dodgemulti, "10.0", NULL, CVAR_LATCH, qfalse)

XCVAR_DEF(g_ffaRespawnTimer, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_deathfx, "1", NULL, CVAR_ARCHIVE, qtrue)

XCVAR_DEF(g_Enhanced_SaberDamage, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_Enhanced_saberTweaks, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_Enhanced_saberBlockChanceMax, "0.75", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_Enhanced_saberBlockChanceMin, "0.0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_Enhanced_saberBlockChanceScale, "0.5", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_Enhanced_saberBlockStanceParity, "3.0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_Enhanced_SaberBlocking, "0", NULL, CVAR_NONE, qtrue)

XCVAR_DEF(SJE_clientMOTD, "", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(SJE_MOTD, "", NULL, CVAR_ARCHIVE, qfalse)

XCVAR_DEF(g_remove_duel_radius, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_AllowKnockDownPull, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_spectate_keep_score, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_pushitems, "1", NULL, CVAR_ARCHIVE, qtrue)

XCVAR_DEF(g_forcekickflip, "1", NULL, CVAR_ARCHIVE, qtrue)

XCVAR_DEF(g_AllowBotBuyItem, "1", NULL, CVAR_NONE, qtrue)
XCVAR_DEF(g_allowturret, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(d_combatinfo, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(d_blockinfo, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(d_saberInfo, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(d_attackinfo, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_DebugSaberCombat, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(d_attackinfo, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_WeaponRemovalTime, "10", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_remove_unused_weapons, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(com_outcast, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_AllowLedgeGrab, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_attackskill, "3", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_saberLockCinematicCamera, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_saberdebug, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_cheatoverride, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF(g_noPadawanNames, "1", NULL, CVAR_ARCHIVE, qfalse)
XCVAR_DEF(g_ffaRespawn, "10", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_adminpassword, "", NULL, CVAR_INTERNAL, qfalse)
XCVAR_DEF(g_adminlogin_saying, "has logged in as admin", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_adminlogout_saying, "has logged out as admin", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_chat_protection, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_adminpunishment_saying, "is being punished", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_AllowWeather, "1", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(com_rend2, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(r_Turn_Off_dynamiclight, "0", NULL, CVAR_ARCHIVE, qtrue)
XCVAR_DEF(g_VaderBreath, "1", NULL, CVAR_ARCHIVE, qtrue)

#undef XCVAR_DEF
