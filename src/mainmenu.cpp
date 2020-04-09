/*
 Copyright (c) 2013-2019 Cong Xu
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */
#include "mainmenu.h"

#include <stdio.h>
#include <string.h>

#include <cdogs/config.h>
#include <cdogs/font.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/log.h>
#include <cdogs/music.h>
#include <cdogs/net_client.h>
#include <cdogs/net_server.h>

#include "autosave.h"
#include "briefing_screens.h"
#include "menu.h"
#include "prep.h"

typedef struct {
	MenuSystem ms;
	GraphicsDevice *graphics;
	credits_displayer_t creditsDisplayer;
	custom_campaigns_t campaigns;
	GameMode lastGameMode;
	bool wasClient;
} MainMenuData;
static void MenuCreateAll(MenuSystem *ms, LoopRunner *l,
		custom_campaigns_t *campaigns, EventHandlers *handlers,
		GraphicsDevice *graphics);
static void MainMenuTerminate(GameLoopData *data);
static void MainMenuOnEnter(GameLoopData *data);
static void MainMenuOnExit(GameLoopData *data);
static GameLoopResult MainMenuUpdate(GameLoopData *data, LoopRunner *l);
static void MainMenuDraw(GameLoopData *data);
GameLoopData* MainMenu(GraphicsDevice *graphics, LoopRunner *l) {
	MainMenuData *data;
	CMALLOC(data, sizeof *data);
	data->graphics = graphics;
	memset(&data->creditsDisplayer, 0, sizeof data->creditsDisplayer);
	LoadCredits(&data->creditsDisplayer, colorPurple, colorDarker);
	memset(&data->campaigns, 0, sizeof data->campaigns);
	LoadAllCampaigns(&data->campaigns);
	data->lastGameMode = GAME_MODE_QUICK_PLAY;
	data->wasClient = false;
	MenuCreateAll(&data->ms, l, &data->campaigns, &gEventHandlers,
			data->graphics);
	MenuSetCreditsDisplayer(&data->ms, &data->creditsDisplayer);
	return GameLoopDataNew(data, MainMenuTerminate, MainMenuOnEnter,
			MainMenuOnExit,
			NULL, MainMenuUpdate, MainMenuDraw);
}
static void MainMenuReset(MenuSystem *ms) {
	GrafxMakeRandomBackground(ms->graphics, &gCampaign, &gMission, &gMap);
	MenuResetSize(ms);
}
static void MainMenuTerminate(GameLoopData *data) {
	MainMenuData *mData = static_cast<MainMenuData*>(data->Data);

	MenuSystemTerminate(&mData->ms);
	UnloadCredits(&mData->creditsDisplayer);
	UnloadAllCampaigns(&mData->campaigns);
	CFREE(mData);
}
static menu_t* FindSubmenuByName(menu_t *menu, const char *name);
static void MainMenuOnEnter(GameLoopData *data) {
	MainMenuData *mData = static_cast<MainMenuData*>(data->Data);

	if (gCampaign.IsLoaded) {
		// Loaded game already; skip menus and go straight to game
		return;
	}

	MusicPlay(&gSoundDevice, MUSIC_MENU, NULL, NULL);

	MainMenuReset(&mData->ms);
	NetClientDisconnect(&gNetClient);
	NetServerClose(&gNetServer);
	GameEventsTerminate(&gGameEvents);
	// Reset config - could have been set to other values by server
	ConfigResetChanged(&gConfig);
	CampaignSettingTerminate(&gCampaign.Setting);

	// Auto-enter the submenu corresponding to the last game mode
	menu_t *startMenu = FindSubmenuByName(mData->ms.root, "Start");
	if (mData->wasClient) {
		mData->ms.current = startMenu;
	} else {
		switch (mData->lastGameMode) {
		case GAME_MODE_NORMAL:
			mData->ms.current = FindSubmenuByName(startMenu, "Campaign");
			break;
		case GAME_MODE_DOGFIGHT:
			mData->ms.current = FindSubmenuByName(startMenu, "Dogfight");
			break;
		case GAME_MODE_DEATHMATCH:
			mData->ms.current = FindSubmenuByName(startMenu, "Deathmatch");
			break;
		default:
			mData->ms.current = mData->ms.root;
			break;
		}
	}
}
static menu_t* FindSubmenuByName(menu_t *menu, const char *name) {
	CASSERT(menu->type == MENU_TYPE_NORMAL, "invalid menu type");
	CA_FOREACH(menu_t, submenu, menu->u.normal.subMenus)
		if (strcmp(submenu->name, name) == 0)
			return submenu;CA_FOREACH_END()
	return menu;
}
static void MainMenuOnExit(GameLoopData *data) {
	MainMenuData *mData = static_cast<MainMenuData*>(data->Data);

	// Reset player datas
	PlayerDataTerminate(&gPlayerDatas);
	PlayerDataInit(&gPlayerDatas);
	// Initialise game events; we need this for init as well as the game
	GameEventsInit(&gGameEvents);

	mData->lastGameMode = gCampaign.Entry.Mode;
	mData->wasClient = gCampaign.IsClient;
}
static GameLoopResult MainMenuUpdate(GameLoopData *data, LoopRunner *l) {
	MainMenuData *mData = static_cast<MainMenuData*>(data->Data);

	if (gCampaign.IsLoaded) {
		// Loaded game already; skip menus and go straight to game
		LoopRunnerPush(l, ScreenCampaignIntro(&gCampaign.Setting));
		return UPDATE_RESULT_OK;
	}

	const GameLoopResult result = MenuUpdate(&mData->ms);
	if (result == UPDATE_RESULT_OK) {
		if (gCampaign.IsLoaded) {
			LoopRunnerPush(l, ScreenCampaignIntro(&gCampaign.Setting));
		} else {
			LoopRunnerPop(l);
		}
	}
	if (gEventHandlers.HasResolutionChanged) {
		MainMenuReset(&mData->ms);
	}
	return result;
}
static void MainMenuDraw(GameLoopData *data) {
	MainMenuData *mData = static_cast<MainMenuData*>(data->Data);
	MenuDraw(&mData->ms);
}

static menu_t* MenuCreateStart(const char *name, MenuSystem *ms, LoopRunner *l,
		custom_campaigns_t *campaigns);
static menu_t* MenuCreateOptions(const char *name, MenuSystem *ms);
menu_t* MenuCreateQuit(const char *name);

static void MenuCreateAll(MenuSystem *ms, LoopRunner *l,
		custom_campaigns_t *campaigns, EventHandlers *handlers,
		GraphicsDevice *graphics) {
	MenuSystemInit(ms, handlers, graphics, svec2i_zero(),
			svec2i(graphics->cachedConfig.Res.x, graphics->cachedConfig.Res.y));
	ms->root = ms->current = MenuCreateNormal("", "", MENU_TYPE_NORMAL,
			MENU_DISPLAY_ITEMS_CREDITS | MENU_DISPLAY_ITEMS_AUTHORS);
	MenuAddSubmenu(ms->root, MenuCreateStart("Start", ms, l, campaigns));
	MenuAddSubmenu(ms->root, MenuCreateOptions("Options...", ms));
#ifndef __EMSCRIPTEN__
	MenuAddSubmenu(ms->root, MenuCreateQuit("Quit"));
	MenuAddExitType(ms, MENU_TYPE_QUIT);
#endif
	MenuAddExitType(ms, MENU_TYPE_RETURN);
}

typedef struct {
	// The index of the join game menu item
	// so we can enable it if LAN servers are found
	int MenuJoinIndex;
} CheckLANServerData;
static menu_t* MenuCreateContinue(const char *name, CampaignEntry *entry);
static menu_t* MenuCreateQuickPlay(const char *name, CampaignEntry *entry);
static menu_t* MenuCreateCampaigns(const char *name, const char *title,
		campaign_list_t *list, const GameMode mode);
static menu_t* CreateJoinLANGame(const char *name, const char *title,
		MenuSystem *ms, LoopRunner *l);
static void CheckLANServers(menu_t *menu, void *data);
static menu_t* MenuCreateStart(const char *name, MenuSystem *ms, LoopRunner *l,
		custom_campaigns_t *campaigns) {
	menu_t *menu = MenuCreateNormal(name, "Start:", MENU_TYPE_NORMAL, 0);
	MenuAddSubmenu(menu,
			MenuCreateContinue("Continue", &gAutosave.LastMission.Campaign));
	const int menuContinueIndex = (int) menu->u.normal.subMenus.size - 1;
	MenuAddSubmenu(menu,
			MenuCreateCampaigns("Campaign", "Select a campaign:",
					&campaigns->campaignList, GAME_MODE_NORMAL));
	MenuAddSubmenu(menu,
			MenuCreateQuickPlay("Quick Play", &campaigns->quickPlayEntry));
	MenuAddSubmenu(menu,
			MenuCreateCampaigns("Dogfight", "Select a scenario:",
					&campaigns->dogfightList, GAME_MODE_DOGFIGHT));
	MenuAddSubmenu(menu,
			MenuCreateCampaigns("Deathmatch", "Select a scenario:",
					&campaigns->dogfightList, GAME_MODE_DEATHMATCH));
	MenuAddSubmenu(menu,
			CreateJoinLANGame("Join LAN game", "Choose LAN server", ms, l));
	CheckLANServerData *cdata;
	CMALLOC(cdata, sizeof *cdata);
	cdata->MenuJoinIndex = (int) menu->u.normal.subMenus.size - 1;
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));

	if (strlen(gAutosave.LastMission.Password) == 0
			|| !gAutosave.LastMission.IsValid
			|| strlen(gAutosave.LastMission.Campaign.Path) == 0) {
		MenuDisableSubmenu(menu, menuContinueIndex);
	}

	MenuDisableSubmenu(menu, cdata->MenuJoinIndex);
	// Periodically check if LAN servers are available
	MenuSetPostUpdateFunc(menu, CheckLANServers, cdata, true);

	return menu;
}

typedef struct {
	enum GameMode GameMode;
	CampaignEntry *Entry;
} StartGameModeData;
static void StartGameMode(menu_t *menu, void *data);

static menu_t* CreateStartGameMode(const char *name, GameMode mode,
		CampaignEntry *entry) {
	menu_t *menu = MenuCreate(name, MENU_TYPE_RETURN);
	menu->enterSound = MENU_SOUND_START;
	StartGameModeData *data;
	CCALLOC(data, sizeof *data);
	data->GameMode = mode;
	data->Entry = entry;
	MenuSetPostEnterFunc(menu, StartGameMode, data, true);
	return menu;
}
static void StartGameMode(menu_t *menu, void *data) {
	UNUSED(menu);
	StartGameModeData *mData = static_cast<StartGameModeData*>(data);
	gCampaign.Entry.Mode = mData->GameMode;
	if (!CampaignLoad(&gCampaign, mData->Entry)) {
		// Failed to load
		printf("Error: cannot load campaign %s\n", mData->Entry->Info);
	}
}
static menu_t* MenuCreateContinue(const char *name, CampaignEntry *entry) {
	return CreateStartGameMode(name, GAME_MODE_NORMAL, entry);
}
static menu_t* MenuCreateQuickPlay(const char *name, CampaignEntry *entry) {
	return CreateStartGameMode(name, GAME_MODE_QUICK_PLAY, entry);
}

static menu_t* MenuCreateCampaignItem(CampaignEntry *entry,
		const GameMode mode);

static void CampaignsDisplayFilename(const menu_t *menu, GraphicsDevice *g,
		const struct vec2i pos, const struct vec2i size, const void *data) {
	const menu_t *subMenu = static_cast<const menu_t*>(CArrayGet(
			&menu->u.normal.subMenus, menu->u.normal.index));
	UNUSED(g);
	UNUSED(data);
	if (subMenu->type != MENU_TYPE_BASIC || subMenu->customPostInputData == NULL) {
		return;
	}
	const StartGameModeData *mData =
			static_cast<const StartGameModeData*>(subMenu->customPostInputData);
	char s[CDOGS_FILENAME_MAX];
	sprintf(s, "( %s )", mData->Entry->Filename);

	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.VAlign = ALIGN_END;
	opts.Area = size;
	opts.Pad.x = size.x / 12;
	FontStrOpt(s, pos, opts);
}
static menu_t* MenuCreateCampaigns(const char *name, const char *title,
		campaign_list_t *list, const GameMode mode) {
	menu_t *menu = MenuCreateNormal(name, title, MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 20;
	menu->u.normal.align = MENU_ALIGN_CENTER;
	CA_FOREACH(campaign_list_t, subList, list->subFolders)
		char folderName[CDOGS_FILENAME_MAX];
		sprintf(folderName, "%s/", subList->Name);
		MenuAddSubmenu(menu,
				MenuCreateCampaigns(folderName, title, subList, mode));
	CA_FOREACH_END()
	CA_FOREACH(CampaignEntry, e, list->list)
		MenuAddSubmenu(menu, MenuCreateCampaignItem(e, mode));
	CA_FOREACH_END()
	MenuSetCustomDisplay(menu, CampaignsDisplayFilename, NULL);
	return menu;
}

static menu_t* MenuCreateCampaignItem(CampaignEntry *entry,
		const GameMode mode) {
	menu_t *menu = CreateStartGameMode(entry->Info, mode, entry);
	// Special colors:
	// - Green for new campaigns
	// - White (normal) for in-progress campaigns
	// - Grey for complete campaigns
	MissionSave m;
	AutosaveLoadMission(&gAutosave, &m, entry->Path);
	if (m.MissionsCompleted == entry->NumMissions) {
		// Completed campaign
		menu->color = colorGray;
	} else if (m.MissionsCompleted > 0) {
		// Campaign in progress
		menu->color = colorYellow;
	}

	return menu;
}

typedef struct {
	MenuSystem *MS;
	LoopRunner *L;
} CreateJoinLANGameData;
static void CreateLANServerMenuItems(menu_t *menu, void *data);
static menu_t* CreateJoinLANGame(const char *name, const char *title,
		MenuSystem *ms, LoopRunner *l) {
	menu_t *menu = MenuCreateNormal(name, title, MENU_TYPE_NORMAL, 0);
	// We'll create our menu items dynamically after entering
	// Creating an item for each scanned server address
	CreateJoinLANGameData *data;
	CMALLOC(data, sizeof *data);
	data->MS = ms;
	data->L = l;
	MenuSetPostEnterFunc(menu, CreateLANServerMenuItems, data, true);
	return menu;
}
typedef struct {
	MenuSystem *MS;
	LoopRunner *L;
	int AddrIndex;
} JoinLANGameData;
static void JoinLANGame(menu_t *menu, void *data);
static void CreateLANServerMenuItems(menu_t *menu, void *data) {
	CreateJoinLANGameData *cData = static_cast<CreateJoinLANGameData*>(data);

	// Clear and recreate all menu items
	MenuClearSubmenus(menu);
	CA_FOREACH(ScanInfo, si, gNetClient.ScannedAddrs)
		char buf[512];
		char ipbuf[256];
		if (enet_address_get_host_ip(&si->Addr, ipbuf, sizeof ipbuf) < 0) {
			LOG(LM_MAIN, LL_WARN, "cannot find host ip");
			ipbuf[0] = '?';
			ipbuf[1] = '\0';
		};
		// e.g. "Bob's Server (123.45.67.89:12345) - Campaign: Ogre Rampage #4, p: 4/16 350ms"
		sprintf(buf, "%s (%s:%u) - %s: %s (# %d), p: %d/%d %dms",
				si->ServerInfo.Hostname, ipbuf, si->Addr.port,
				GameModeStr(static_cast<GameMode>(si->ServerInfo.GameMode)),
				si->ServerInfo.CampaignName, si->ServerInfo.MissionNumber,
				si->ServerInfo.NumPlayers, si->ServerInfo.MaxPlayers,
				si->LatencyMS);
		menu_t *serverMenu = MenuCreate(buf, MENU_TYPE_RETURN);
		serverMenu->enterSound = MENU_SOUND_START;
		JoinLANGameData *jdata;
		CMALLOC(jdata, sizeof *jdata);
		jdata->MS = cData->MS;
		jdata->L = cData->L;
		jdata->AddrIndex = _ca_index;
		MenuSetPostEnterFunc(serverMenu, JoinLANGame, jdata, true);
		MenuAddSubmenu(menu, serverMenu);
	CA_FOREACH_END()
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));
}
static void JoinLANGame(menu_t *menu, void *data) {
	JoinLANGameData *jdata = static_cast<JoinLANGameData*>(data);
	ScanInfo *sinfo;
	if (jdata->AddrIndex >= (int) gNetClient.ScannedAddrs.size) {
		goto bail;
	}
	sinfo = static_cast<ScanInfo*>(CArrayGet(&gNetClient.ScannedAddrs,
			jdata->AddrIndex));
	LOG(LM_MAIN, LL_INFO, "joining LAN game...");
	if (NetClientTryConnect(&gNetClient, sinfo->Addr)) {
		LoopRunnerPush(jdata->L, ScreenWaitForCampaignDef());
		goto bail;
	} else {
		LOG(LM_MAIN, LL_INFO, "failed to connect to LAN game");
	}
	return;

	bail:
	// Don't activate the menu item
	jdata->MS->current = menu->parentMenu;
}
static void CheckLANServers(menu_t *menu, void *data) {
	CheckLANServerData *cdata = static_cast<CheckLANServerData*>(data);
	if (gNetClient.ScannedAddrs.size > 0) {
		MenuEnableSubmenu(menu, cdata->MenuJoinIndex);
	} else {
		// We haven't found any LAN servers in the latest scan
		MenuDisableSubmenu(menu, cdata->MenuJoinIndex);
	}
	if (gNetClient.ScanTicks <= 0) {
		LOG(LM_MAIN, LL_DEBUG, "finding LAN server...");
		NetClientFindLANServers(&gNetClient);
	}
}

static menu_t* MenuCreateOptionsGraphics(const char *name, MenuSystem *ms);
#if !defined(__ANDROID__) && !defined(__GCWZERO__)
static menu_t* MenuCreateOptionsControls(const char *name, MenuSystem *ms);
#endif

static menu_t* MenuCreateOptions(const char *name, MenuSystem *ms) {
	menu_t *menu = MenuCreateNormal(name, "Options:", MENU_TYPE_NORMAL, 0);
	MenuAddSubmenu(menu,
			MenuCreateConfigOptions("Game...", "Game Options:",
					ConfigGet(&gConfig, "Game"), ms, true));
	MenuAddSubmenu(menu, MenuCreateOptionsGraphics("Graphics...", ms));
	MenuAddSubmenu(menu,
			MenuCreateConfigOptions("Interface...", "Interface Options:",
					ConfigGet(&gConfig, "Interface"), ms, true));
#if !defined(__ANDROID__) && !defined(__GCWZERO__)
	MenuAddSubmenu(menu, MenuCreateOptionsControls("Controls...", ms));
#endif
	MenuAddSubmenu(menu,
			MenuCreateConfigOptions("Sound...", "Configure Sound:",
					ConfigGet(&gConfig, "Sound"), ms, true));
	MenuAddSubmenu(menu,
			MenuCreateConfigOptions("Quick Play...", "Quick Play Options:",
					ConfigGet(&gConfig, "QuickPlay"), ms, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));
	return menu;
}

menu_t* MenuCreateOptionsGraphics(const char *name, MenuSystem *ms) {
	menu_t *menu = MenuCreateNormal(name, "Graphics Options:",
			MENU_TYPE_OPTIONS, 0);
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Brightness"));
#ifndef __GCWZERO__
#ifndef __ANDROID__
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Fullscreen"));
	// TODO: fix second window rendering
	// MenuAddConfigOptionsItem(
	//	menu, ConfigGet(&gConfig, "Graphics.SecondWindow"));
#endif	// ANDROID

	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.ScaleFactor"));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.ScaleMode"));
#endif	// GCWZERO
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Shadows"));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Gore"));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Brass"));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}

menu_t* MenuCreateKeys(const char *name, MenuSystem *ms);

#if !defined(__ANDROID__) && !defined(__GCWZERO__)
menu_t* MenuCreateOptionsControls(const char *name, MenuSystem *ms) {
	menu_t *menu = MenuCreateNormal(name, "Configure Controls:",
			MENU_TYPE_OPTIONS, 0);
	MenuAddSubmenu(menu, MenuCreateKeys("Redefine keys...", ms));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}
#endif

menu_t* MenuCreateQuit(const char *name) {
	return MenuCreate(name, MENU_TYPE_QUIT);
}

static void MenuCreateKeysSingleSection(menu_t *menu, const char *sectionName,
		const int playerIndex);
static menu_t* MenuCreateOptionChangeKey(const key_code_e code,
		const int playerIndex, const bool isOptional);

menu_t* MenuCreateKeys(const char *name, MenuSystem *ms) {
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_KEYS, 0);
	MenuCreateKeysSingleSection(menu, "Keyboard 1", 0);
	MenuCreateKeysSingleSection(menu, "Keyboard 2", 1);
	MenuAddSubmenu(menu, MenuCreateOptionChangeKey(KEY_CODE_MAP, 0, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}

static void MenuCreateKeysSingleSection(menu_t *menu, const char *sectionName,
		const int playerIndex) {
	MenuAddSubmenu(menu, MenuCreateSeparator(sectionName));
	MenuAddSubmenu(menu,
			MenuCreateOptionChangeKey(KEY_CODE_LEFT, playerIndex, false));
	MenuAddSubmenu(menu,
			MenuCreateOptionChangeKey(KEY_CODE_RIGHT, playerIndex, false));
	MenuAddSubmenu(menu,
			MenuCreateOptionChangeKey(KEY_CODE_UP, playerIndex, false));
	MenuAddSubmenu(menu,
			MenuCreateOptionChangeKey(KEY_CODE_DOWN, playerIndex, false));
	MenuAddSubmenu(menu,
			MenuCreateOptionChangeKey(KEY_CODE_BUTTON1, playerIndex, false));
	MenuAddSubmenu(menu,
			MenuCreateOptionChangeKey(KEY_CODE_BUTTON2, playerIndex, false));
	MenuAddSubmenu(menu,
			MenuCreateOptionChangeKey(KEY_CODE_GRENADE, playerIndex, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
}

static menu_t* MenuCreateOptionChangeKey(const key_code_e code,
		const int playerIndex, const bool isOptional) {
	menu_t *menu = MenuCreate(KeycodeStr(code),
			MENU_TYPE_SET_OPTION_CHANGE_KEY);
	menu->u.changeKey.code = code;
	menu->u.changeKey.playerIndex = playerIndex;
	menu->u.changeKey.isOptional = isOptional;
	return menu;
}