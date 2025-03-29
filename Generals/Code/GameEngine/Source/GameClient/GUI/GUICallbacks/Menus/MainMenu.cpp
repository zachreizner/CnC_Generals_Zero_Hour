/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: MainMenu.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Colin Day, October 2001
// Description: Main menu window callbacks
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

// #include "GameSpy/ghttp/ghttp.h"

#include "Lib/BaseType.h"
#include "Common/GameEngine.h"
#include "Common/GameState.h"
#include "Common/GlobalData.h"
#include "Common/NameKeyGenerator.h"
#include "Common/RandomValue.h"
#include "Common/UserPreferences.h"
#include "Common/version.h"
#include "GameClient/AnimateWindowManager.h"
#include "GameClient/ExtendedMessageBox.h"
#include "GameClient/MessageBox.h"
#include "GameClient/Display.h"
#include "GameClient/WindowLayout.h"
#include "GameClient/Gadget.h"
#include "GameClient/GameText.h"
#include "GameClient/HeaderTemplate.h"
#include "GameClient/MapUtil.h"
#include "GameClient/Shell.h"
#include "GameClient/ShellHooks.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/Mouse.h"
#include "GameClient/WindowVideoManager.h"
#include "GameClient/CampaignManager.h"
#include "GameClient/HotKey.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/ScriptEngine.h"
#include "GameNetwork/GameSpyOverlay.h"
#include "GameClient/GameWindowTransitions.h"

#include "GameNetwork/GameSpy/PeerDefs.h"
#include "GameNetwork/GameSpy/PeerThread.h"
#include "GameNetwork/GameSpy/BuddyThread.h"

#include "GameNetwork/DownloadManager.h"
#include "GameNetwork/GameSpy/MainMenuUtils.h"

#include "GameClient/CDCheck.h"
//Added By Saad
//for accessing the InGameUI
#include "GameClient/InGameUI.h"

// This define removes the code that could get you to solo and LAN games.
//#if !defined(_DEBUG) && !defined(_INTERNAL)
//#define _PLAYTEST
//#endif
// 10-20  GS  Made this a project setting so we can set it in one place.  (It has spread to several files, inclding MessageStream.h)
#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif


// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////

enum
{
	DROPDOWN_NONE = 0,
	DROPDOWN_SINGLE,
	DROPDOWN_MULTIPLAYER,
	DROPDOWN_MAIN,
	DROPDOWN_LOADREPLAY,
	DROPDOWN_DIFFICULTY,
	
	DROPDOWN_COUNT // keep last
};

static Bool raiseMessageBoxes = TRUE;
static Bool campaignSelected = FALSE;
#if defined _DEBUG || defined _INTERNAL
static NameKeyType campaignID = NAMEKEY_INVALID;
static GameWindow *buttonCampaign = NULL;
#ifdef TEST_COMPRESSION
static GameWindow *buttonCompressTest = NULL;
void DoCompressTest( void );
#endif // TEST_COMPRESSION
#endif

// window ids -------------------------------------------------------------------------------------
static NameKeyType mainMenuID = NAMEKEY_INVALID;
static NameKeyType skirmishID = NAMEKEY_INVALID;
static NameKeyType onlineID = NAMEKEY_INVALID;
static NameKeyType networkID = NAMEKEY_INVALID;
static NameKeyType optionsID = NAMEKEY_INVALID;
static NameKeyType exitID = NAMEKEY_INVALID;
static NameKeyType motdID = NAMEKEY_INVALID;
static NameKeyType worldBuilderID = NAMEKEY_INVALID;
static NameKeyType getUpdateID = NAMEKEY_INVALID;
static NameKeyType buttonTRAININGID = NAMEKEY_INVALID;
static NameKeyType buttonUSAID = NAMEKEY_INVALID;
static NameKeyType buttonGLAID = NAMEKEY_INVALID;
static NameKeyType buttonChinaID = NAMEKEY_INVALID;
static NameKeyType buttonUSARecentSaveID = NAMEKEY_INVALID;
static NameKeyType buttonUSALoadGameID = NAMEKEY_INVALID;
static NameKeyType buttonGLARecentSaveID = NAMEKEY_INVALID;
static NameKeyType buttonGLALoadGameID = NAMEKEY_INVALID;
static NameKeyType buttonChinaRecentSaveID = NAMEKEY_INVALID;
static NameKeyType buttonChinaLoadGameID = NAMEKEY_INVALID;
static NameKeyType buttonSinglePlayerID = NAMEKEY_INVALID;
static NameKeyType buttonMultiPlayerID = NAMEKEY_INVALID;
static NameKeyType buttonMultiBackID = NAMEKEY_INVALID;
static NameKeyType buttonSingleBackID = NAMEKEY_INVALID;
static NameKeyType buttonLoadReplayBackID = NAMEKEY_INVALID;
static NameKeyType buttonReplayID = NAMEKEY_INVALID;
static NameKeyType buttonLoadReplayID = NAMEKEY_INVALID;
static NameKeyType buttonLoadID = NAMEKEY_INVALID;
static NameKeyType buttonCreditsID = NAMEKEY_INVALID;
static NameKeyType buttonEasyID = NAMEKEY_INVALID;
static NameKeyType buttonMediumID = NAMEKEY_INVALID;
static NameKeyType buttonHardID = NAMEKEY_INVALID;
static NameKeyType buttonDiffBackID = NAMEKEY_INVALID;


// window pointers --------------------------------------------------------------------------------
static GameWindow *parentMainMenu = NULL;
static GameWindow *buttonSinglePlayer = NULL;
static GameWindow *buttonMultiPlayer = NULL;
static GameWindow *buttonSkirmish = NULL;
static GameWindow *buttonOnline = NULL;
static GameWindow *buttonNetwork = NULL;
static GameWindow *buttonOptions = NULL;
static GameWindow *buttonExit = NULL;
static GameWindow *buttonMOTD = NULL;
static GameWindow *buttonWorldBuilder = NULL;
static GameWindow *mainMenuMovie = NULL;
static GameWindow *getUpdate = NULL;
static GameWindow *buttonTRAINING = NULL;
static GameWindow *buttonUSA = NULL;
static GameWindow *buttonGLA = NULL;
static GameWindow *buttonChina = NULL;
static GameWindow *buttonUSARecentSave = NULL;
static GameWindow *buttonUSALoadGame = NULL;
static GameWindow *buttonGLARecentSave = NULL;
static GameWindow *buttonGLALoadGame = NULL;
static GameWindow *buttonChinaRecentSave = NULL;
static GameWindow *buttonChinaLoadGame = NULL;
static GameWindow *buttonReplay = NULL;
static GameWindow *buttonLoadReplay = NULL;
static GameWindow *buttonLoad = NULL;
static GameWindow *buttonCredits = NULL;
static GameWindow *buttonEasy = NULL;
static GameWindow *buttonMedium = NULL;
static GameWindow *buttonHard = NULL;
static GameWindow *buttonDiffBack = NULL;
static GameWindow *dropDownWindows[DROPDOWN_COUNT];

static Bool buttonPushed = FALSE;
static Bool isShuttingDown = FALSE;
static Bool startGame = FALSE;
static Int	initialGadgetDelay = 210;
enum
{
	SHOW_NONE = 0,
	SHOW_TRAINING,
	SHOW_USA,
	SHOW_GLA,
	SHOW_CHINA,
	SHOW_SKIRMISH,
	SHOW_FRAMES_LIMIT = 20
};

static Bool showFade = FALSE;
static Int dropDown = DROPDOWN_NONE;
static Int pendingDropDown = DROPDOWN_NONE;
static AnimateWindowManager *localAnimateWindowManager = NULL;
static Bool notShown = TRUE;
static Bool FirstTimeRunningTheGame = TRUE;

static Bool showLogo = FALSE;
static Int  showFrames = 0;
static Int  showSide = SHOW_NONE;
static Bool logoIsShown = FALSE;
static Bool justEntered = FALSE;

static Bool dontAllowTransitions = FALSE;

//Added by Saad
const /*Int TIME_OUT = 15,*/ Int CORNER = 10;
void AcceptResolution();
void DeclineResolution();
GameWindow *resAcceptMenu = NULL;
extern DisplaySettings oldDispSettings, newDispSettings;
extern Bool dispChanged;
//static time_t timeStarted = 0, currentTime = 0;
//

void diffReverseSide( void );
void HandleCanceledDownload( Bool resetDropDown )
{
	buttonPushed = FALSE;
	if (resetDropDown)
	{
		dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
		TheTransitionHandler->setGroup("MainMenuDefaultMenuLogoFade");
	}
}

//-------------------------------------------------------------------------------------------------
/** This is called when a shutdown is complete for this menu */
//-------------------------------------------------------------------------------------------------

static void showSelectiveButtons( Int show )
{
	buttonUSARecentSave->winHide(!(show == SHOW_USA ));
	buttonUSALoadGame->winHide(!(show == SHOW_USA ));
	buttonGLARecentSave->winHide(!(show == SHOW_GLA ));
	buttonGLALoadGame->winHide(!(show == SHOW_GLA ));
	buttonChinaRecentSave->winHide(!(show == SHOW_CHINA ));
	buttonChinaLoadGame->winHide(!(show == SHOW_CHINA ));
}

static void quitCallback( void )
{
	buttonPushed = TRUE;
	TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_EXIT_SELECTED]);
	TheShell->pop();
	TheGameEngine->setQuitting( TRUE );

	if (TheGameLogic->isInGame())
		TheMessageStream->appendMessage( GameMessage::MSG_CLEAR_GAME_DATA );
}


void setupGameStart(AsciiString mapName, GameDifficulty diff)
{
	startGame = TRUE;
	TheCampaignManager->setGameDifficulty(diff);
	TheWritableGlobalData->m_pendingFile = mapName;
	TheShell->reverseAnimatewindow();
	TheTransitionHandler->setGroup("FadeWholeScreen");
}

void prepareCampaignGame(GameDifficulty diff)
{
	dontAllowTransitions = TRUE;
	OptionPreferences pref;
	pref.setCampaignDifficulty(diff);
	pref.write();
	TheScriptEngine->setGlobalDifficulty(diff);

	buttonPushed = FALSE;
	TheTransitionHandler->reverse("MainMenuDifficultyMenuBack");
	setupGameStart(TheCampaignManager->getCurrentMap(), diff );
}

static MessageBoxReturnType cancelStartBecauseOfNoCD( void *userData )
{
	return MB_RETURN_CLOSE;
}

static MessageBoxReturnType checkCDCallback( void *userData )
{
	if (!IsFirstCDPresent())
	{
		return MB_RETURN_KEEPOPEN;
	}
	else
	{
		prepareCampaignGame((GameDifficulty)(Int)(uintptr_t)userData);
		return MB_RETURN_CLOSE;
	}
}

static void doGameStart( void )
{
#if !defined(_PLAYTEST)
	startGame = FALSE;

	if (TheGameLogic->isInGame())
		TheGameLogic->clearGameData();

	// send a message to the logic for a new game
	GameMessage *msg = TheMessageStream->appendMessage( GameMessage::MSG_NEW_GAME );
	msg->appendIntegerArgument(GAME_SINGLE_PLAYER);
	msg->appendIntegerArgument(TheCampaignManager->getGameDifficulty());
	msg->appendIntegerArgument(TheCampaignManager->getRankPoints());
	InitRandom(0);

	isShuttingDown = TRUE;
#endif
}

static void checkCDBeforeCampaign(GameDifficulty diff)
{
	if (!IsFirstCDPresent())
	{
		// popup a dialog asking for a CD
		ExMessageBoxOkCancel(TheGameText->fetch("GUI:InsertCDPrompt"), TheGameText->fetch("GUI:InsertCDMessage"),
			(void *)diff, checkCDCallback, cancelStartBecauseOfNoCD);
	}
	else
	{
		prepareCampaignGame(diff);
	}
}

static void shutdownComplete( WindowLayout *layout )
{
	isShuttingDown = FALSE;
	
	// hide the layout
	layout->hide( TRUE );

	// our shutdown is complete
	TheShell->shutdownComplete( layout );

}  // end if



/*
static void TimetToFileTime( time_t t, LPFILETIME pft )
{
	LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD) ll;
	pft->dwHighDateTime = ll >>32;
}
*/

void initialHide( void )
{
GameWindow *win = NULL;
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionGLA"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionChina"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionUS"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinGrowMarker"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionTraining"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionTrainingSmall"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionTrainingMedium"));
	if(win)
		win->winHide(TRUE);

	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionSkirmish"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionSkirmishSmall"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionSkirmishMedium"));
	if(win)
		win->winHide(TRUE);

	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionUS"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionUSSmall"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionUSMedium"));
	if(win)
		win->winHide(TRUE);

	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionGLA"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionGLASmall"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionGLAMedium"));
	if(win)
		win->winHide(TRUE);

	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionChina"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionChinaSmall"));
	if(win)
		win->winHide(TRUE);
	win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionChinaMedium"));
	if(win)
		win->winHide(TRUE);

}
//-------------------------------------------------------------------------------------------------
/** Initialize the main menu */
//-------------------------------------------------------------------------------------------------
void MainMenuInit( WindowLayout *layout, void *userData )
{
	TheWritableGlobalData->m_breakTheMovie = FALSE;

	TheShell->showShellMap(TRUE);
	TheMouse->setVisibility(TRUE);
	//winVidManager = NEW WindowVideoManager;
	buttonPushed = FALSE;
	isShuttingDown = FALSE;
	startGame = FALSE;
	dropDown = DROPDOWN_NONE;
	pendingDropDown = DROPDOWN_NONE;
	for(Int i = 0; i < DROPDOWN_COUNT; ++i)
		dropDownWindows[i] = NULL;

	// get ids for our windows
	mainMenuID = TheNameKeyGenerator->nameToKey( AsciiString( "MainMenu.wnd:MainMenuParent" ) );
//	campaignID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonCampaign") );
	skirmishID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonSkirmish") );
	onlineID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonOnline") );
	networkID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonNetwork") );
	optionsID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonOptions") );
	exitID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonExit") );
	motdID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonMOTD") );
	worldBuilderID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonWorldBuilder") );
//	NameKeyType versionID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:LabelVersion") );
	getUpdateID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonGetUpdate") );
	buttonTRAININGID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonTRAINING") );
	buttonUSAID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonUSA") );
	buttonGLAID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonGLA") );
	buttonChinaID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonChina") );
	buttonUSARecentSaveID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonUSARecentSave") );
	buttonUSALoadGameID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonUSALoadGame") );
	buttonGLARecentSaveID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonGLARecentSave") );
	buttonGLALoadGameID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonGLALoadGame") );
	buttonChinaRecentSaveID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonChinaRecentSave") );
	buttonChinaLoadGameID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonChinaLoadGame") );
	buttonSinglePlayerID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonSinglePlayer") );
	buttonMultiPlayerID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonMultiplayer") );
	buttonMultiBackID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonMultiBack") );
	buttonSingleBackID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonSingleBack") );
	buttonLoadReplayBackID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonLoadReplayBack") );
	buttonReplayID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonReplay") );
	buttonLoadReplayID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonLoadReplay") );
	buttonLoadID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonLoadGame") );
	buttonCreditsID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonCredits") );

	buttonEasyID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonEasy") );
	buttonMediumID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonMedium") );
	buttonHardID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonHard") );
	buttonDiffBackID = TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:ButtonDiffBack") );

	// get pointers to the window buttons
	parentMainMenu = TheWindowManager->winGetWindowFromId( NULL, mainMenuID );
	//buttonCampaign = TheWindowManager->winGetWindowFromId( parentMainMenu, campaignID );
	buttonSinglePlayer = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonSinglePlayerID );
	buttonMultiPlayer = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonMultiPlayerID );
	buttonSkirmish = TheWindowManager->winGetWindowFromId( parentMainMenu, skirmishID );
	buttonOnline = TheWindowManager->winGetWindowFromId( parentMainMenu, onlineID );
	buttonNetwork = TheWindowManager->winGetWindowFromId( parentMainMenu, networkID );
	buttonOptions = TheWindowManager->winGetWindowFromId( parentMainMenu, optionsID );
	buttonExit = TheWindowManager->winGetWindowFromId( parentMainMenu, exitID );
	buttonMOTD = TheWindowManager->winGetWindowFromId( parentMainMenu, motdID );
	buttonWorldBuilder = TheWindowManager->winGetWindowFromId( parentMainMenu, worldBuilderID );
	buttonReplay = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonReplayID );
	buttonLoadReplay = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonLoadReplayID );
	buttonLoad = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonLoadID );
	buttonCredits = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonCreditsID );

	buttonEasy = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonEasyID );
	buttonMedium = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonMediumID );
	buttonHard = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonHardID );
	buttonDiffBack = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonDiffBackID );


//	GameWindow *labelVersion = TheWindowManager->winGetWindowFromId( parentMainMenu, versionID );
	
	getUpdate = TheWindowManager->winGetWindowFromId( parentMainMenu, getUpdateID );
	buttonTRAINING = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonTRAININGID );
	buttonUSA = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonUSAID );
	buttonGLA = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonGLAID );
	buttonChina = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonChinaID );
	buttonUSARecentSave = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonUSARecentSaveID );
	buttonUSALoadGame = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonUSALoadGameID );
	buttonGLARecentSave = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonGLARecentSaveID );
	buttonGLALoadGame = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonGLALoadGameID );
	buttonChinaRecentSave = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonChinaRecentSaveID );
	buttonChinaLoadGame = TheWindowManager->winGetWindowFromId( parentMainMenu, buttonChinaLoadGameID );

	dropDownWindows[DROPDOWN_SINGLE] = TheWindowManager->winGetWindowFromId( parentMainMenu, TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:MapBorder") ));
	dropDownWindows[DROPDOWN_MULTIPLAYER] = TheWindowManager->winGetWindowFromId( parentMainMenu, TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:MapBorder1") ) );
	dropDownWindows[DROPDOWN_MAIN] = TheWindowManager->winGetWindowFromId( parentMainMenu, TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:MapBorder2") ) );
	dropDownWindows[DROPDOWN_LOADREPLAY] = TheWindowManager->winGetWindowFromId( parentMainMenu, TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:MapBorder3") ) );
	dropDownWindows[DROPDOWN_DIFFICULTY] = TheWindowManager->winGetWindowFromId( parentMainMenu, TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:MapBorder4") ) );
	for(Int i = 1; i < DROPDOWN_COUNT; ++i)
		dropDownWindows[i]->winHide(TRUE);
	
	initialHide();
	
	showSelectiveButtons(SHOW_NONE);
	// Set up the version number
#if defined _DEBUG || defined _INTERNAL
	WinInstanceData instData;
#ifdef TEST_COMPRESSION
	instData.init();
	BitSet( instData.m_style, GWS_PUSH_BUTTON | GWS_MOUSE_TRACK );
	instData.m_textLabelString = "Debug: Compress/Decompress Maps";
	instData.setTooltipText(UnicodeString(L"Only Used in Debug and Internal!"));
	buttonCompressTest = TheWindowManager->gogoGadgetPushButton( parentMainMenu, 
																									 WIN_STATUS_ENABLED | WIN_STATUS_IMAGE, 
																									 25, 175, 
																									 400, 400, 
																									 &instData, NULL, TRUE );
#endif // TEST_COMPRESSION

	instData.init();
	BitSet( instData.m_style, GWS_PUSH_BUTTON | GWS_MOUSE_TRACK );
	instData.m_textLabelString = "Debug: Load Map";
	
	instData.setTooltipText(UnicodeString(L"Only Used in Debug and Internal!"));
	buttonCampaign = TheWindowManager->gogoGadgetPushButton( parentMainMenu, 
																									 WIN_STATUS_ENABLED, 
																									 25, 54, 
																									 180, 26, 
																									 &instData, NULL, TRUE );
	
//	if (TheVersion)
//	{
//		UnicodeString version;
//		version.format(L"%s\n%s", TheVersion->getUnicodeVersion().str(), TheVersion->getUnicodeBuildTime().str());
//		GadgetStaticTextSetText( labelVersion, version );
//	}
//	else
//	{
//		labelVersion->winHide( TRUE );
//	}
//#else
	
//	GadgetStaticTextSetText( labelVersion, TheVersion->getUnicodeVersion() );
#endif

	//TheShell->registerWithAnimateManager(buttonCampaign, WIN_ANIMATION_SLIDE_LEFT, TRUE, 800);
	//TheShell->registerWithAnimateManager(buttonSkirmish, WIN_ANIMATION_SLIDE_LEFT, TRUE, 600);
//	TheShell->registerWithAnimateManager(buttonSinglePlayer, WIN_ANIMATION_SLIDE_LEFT, TRUE, 400);
//	TheShell->registerWithAnimateManager(buttonMultiPlayer, WIN_ANIMATION_SLIDE_LEFT, TRUE, 200);
//	TheShell->registerWithAnimateManager(buttonOptions, WIN_ANIMATION_SLIDE_LEFT, TRUE, 1);
//	TheShell->registerWithAnimateManager(buttonExit, WIN_ANIMATION_SLIDE_RIGHT, TRUE, 1);
//	
	layout->hide( FALSE );

	/*
	if (!checkedForUpdate)
	{
		DWORD state = 0;
		Bool isConnected = InternetGetConnectedState(&state, 0);
		if (isConnected && !(state & INTERNET_CONNECTION_MODEM_BUSY))
		{
			// wohoo - we're connected!  fire off a check for updates
			checkedForUpdate = TRUE;
			DEBUG_LOG(("Looking for a patch for productID=%d, versionStr=%s, distribution=%d\n",
				gameProductID, gameVersionUniqueIDStr, gameDistributionID));
			ptCheckForPatch( gameProductID, gameVersionUniqueIDStr, gameDistributionID, patchAvailableCallback, PTFalse, NULL );
			//ptCheckForPatch( productID, versionUniqueIDStr, distributionID, mapPackAvailableCallback, PTFalse, NULL );
		}
	}
	if (getUpdate != NULL)
	{
		getUpdate->winHide( TRUE );
		//getUpdate->winEnable( FALSE );
	}
	/**/

	if (TheGameSpyPeerMessageQueue && !TheGameSpyPeerMessageQueue->isConnected())
	{
		DEBUG_LOG(("Tearing down GameSpy from MainMenuInit()\n"));
		TearDownGameSpy();
	}
	if (TheMapCache)
		TheMapCache->updateCache();

	/*
	if (MOTDBuffer && buttonMOTD)
	{
		buttonMOTD->winHide(FALSE);
	}
	*/
	
	TheShell->loadScheme("MainMenu");
	raiseMessageBoxes = TRUE;

//	if(!localAnimateWindowManager)
//		localAnimateWindowManager = NEW AnimateWindowManager;

	//pendingDropDown =DROPDOWN_MAIN;
	
#if defined(_PLAYTEST)
	// force these buttons enabled in DEBUG and INTERNAL, so we can still work. :-)
	buttonNetwork->winEnable(FALSE);
	buttonSinglePlayer->winEnable(FALSE);
	buttonReplay->winEnable(FALSE);
#endif

#if defined(_PLAYTEST)

	static Bool didMinSpecCheck = false;

	if (!didMinSpecCheck)
	{
		Bool videoPassed, cpuPassed, memPassed;
		if (!TheDisplay->testMinSpecRequirements(&videoPassed, &cpuPassed, &memPassed))
		{	//system failed the test
			MessageBoxOk(TheGameText->fetch("GUI:MinSpecFailedTitle"), TheGameText->fetch("GUI:MinSpecFailedMessage"),NULL);		
		}
		didMinSpecCheck = true;
	}
#endif
	GameWindow *rule = TheWindowManager->winGetWindowFromId( parentMainMenu, TheNameKeyGenerator->nameToKey( AsciiString("MainMenu.wnd:MainMenuRuler") ) );
	if(rule)
		rule->winHide(TRUE);
	campaignSelected = FALSE;
//	dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
	if(FirstTimeRunningTheGame)
	{
		TheMouse->setVisibility(FALSE);

		TheTransitionHandler->reverse("FadeWholeScreen");
		FirstTimeRunningTheGame  = FALSE;
	}
	else
	{
		showFade = TRUE;
		justEntered = TRUE;
		initialGadgetDelay = 2;
		if(rule)
		rule->winHide(FALSE);
	}

	layout->bringForward();	
	// set keyboard focus to main parent
	TheWindowManager->winSetFocus( parentMainMenu );
	
	
}  // end MainMenuInit

//-------------------------------------------------------------------------------------------------
/** Main menu shutdown method */
//-------------------------------------------------------------------------------------------------
void MainMenuShutdown( WindowLayout *layout, void *userData )
{
	if (!startGame)
		isShuttingDown = TRUE;

	CancelPatchCheckCallback();

	// if we are shutting down for an immediate pop, skip the animations
	Bool popImmediate = *(Bool *)userData;
	
//	if(winVidManager)
	//		delete winVidManager;
	//	winVidManager = NULL;
	

	if( popImmediate )
	{
//		if(localAnimateWindowManager)
//		{
//			delete localAnimateWindowManager;
//			localAnimateWindowManager = NULL;
//		}
		shutdownComplete( layout );
		return;

	}  //end if

	if (!startGame)
		TheShell->reverseAnimatewindow();
	//TheShell->reverseAnimatewindow();
//	if(localAnimateWindowManager && dropDown != DROPDOWN_NONE)
//		localAnimateWindowManager->reverseAnimateWindow();
}  // end MainMenuShutdown

extern Bool DontShowMainMenu;

////////////////////////////////////////////////////////////////////////////
//Added By Sadullah Nader
//Added as a fix to the resolution change
//Allows the user to confirm the change, goes back to the previous mode 
//if the time to change expires.
////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
// Accept Resolution callback method
//-------------------------------------------------------------------------------------------------
void AcceptResolution()
{
	//Keep new settings and bail with setting the display changed flag
	//set to off
	oldDispSettings = newDispSettings;
	dispChanged = FALSE;
}

//-------------------------------------------------------------------------------------------------
// Decline Resolution callback method
//-------------------------------------------------------------------------------------------------
void DeclineResolution()
{
	//Revert back to old resolution and reset all necessary 
	//parts of the shell

	if (TheDisplay->setDisplayMode(oldDispSettings.xRes, oldDispSettings.yRes, 
										oldDispSettings.bitDepth, oldDispSettings.windowed))
	{
		dispChanged = FALSE;
		newDispSettings = oldDispSettings;

		TheWritableGlobalData->m_xResolution = newDispSettings.xRes;
		TheWritableGlobalData->m_yResolution = newDispSettings.yRes;
		
		TheHeaderTemplateManager->headerNotifyResolutionChange();
		TheMouse->mouseNotifyResolutionChange();
				
		AsciiString prefString;
		prefString.format("%d %d", newDispSettings.xRes, newDispSettings.yRes);
		
		OptionPreferences optionPref;
		optionPref["Resolution"] = prefString;
		optionPref.write();

		// delete the shell
		delete TheShell;
		TheShell = NULL;

		// create the shell
		TheShell = MSGNEW("GameClientSubsystem") Shell;
		if( TheShell )
			TheShell->init();
		
		TheInGameUI->recreateControlBar();

		TheShell->push( AsciiString("Menus/MainMenu.wnd") );
	}
}

//-------------------------------------------------------------------------------------------------
// Accept/Decline Resolution dialog method
//-------------------------------------------------------------------------------------------------
void DoResolutionDialog()
{
	//Bring up a dialog to accept the resolution chosen in the options menu
	UnicodeString resolutionNew;
	
	UnicodeString resTimerString = TheGameText->fetch("GUI:Resolution");
	
	resolutionNew.format(L": %dx%d\n", newDispSettings.xRes , newDispSettings.yRes);
	
	resTimerString.concat(resolutionNew);
		
	
	resAcceptMenu = TheWindowManager->gogoMessageBox( CORNER, CORNER, -1, -1,MSG_BOX_OK | MSG_BOX_CANCEL , 
																									 TheGameText->fetch("GUI:Resolution"), 
																									 resTimerString, NULL, NULL, AcceptResolution, 
																									 DeclineResolution);
}

/* This function is not being currently used because we do not need a timer on the 
// dialog box.
//-------------------------------------------------------------------------------------------------
//ResolutionDialogUpdate() - if resolution dialog box is shown, this must count 10 seconds for 
//	accepting resolution changes otherwise we go back to previous display settings 
//-------------------------------------------------------------------------------------------------
void ResolutionDialogUpdate()
{
	if (timeStarted == 0 && currentTime == 0)
	{
		timeStarted = currentTime = time(NULL);
	}
	else 
	{
		currentTime = time(NULL);
	}
	
	if ( ( currentTime - timeStarted ) >= TIME_OUT)
	{
		currentTime = timeStarted = 0;
		DeclineResolution();
	}
	//------------------------------------------------------------------------------------------------------
	// Used for debugging purposes
	//------------------------------------------------------------------------------------------------------
	DEBUG_LOG(("Resolution Timer :  started at %d,  current time at %d, frameTicker is %d\n", timeStarted, 
							time(NULL) , currentTime));
}
*/

//-------------------------------------------------------------------------------------------------
/** Main menu update method */
//-------------------------------------------------------------------------------------------------
void DownloadMenuUpdate( WindowLayout *layout, void *userData );
void MainMenuUpdate( WindowLayout *layout, void *userData )
{
	if( TheGameLogic->isInGame() && !TheGameLogic->isInShellGame() )
	{
		return;
	}
	if(DontShowMainMenu && justEntered)
		justEntered = FALSE;
	
	if (TheDownloadManager && !TheDownloadManager->isDone())
	{
		TheDownloadManager->update();
		DownloadMenuUpdate(layout, userData);
	}

	// Added by Saad to the confirmation or decline of the resoluotion change
	// dialog box.
	/* This is also commented for the same reason as the top
	if (dispChanged)
	{
		ResolutionDialogUpdate();
		return;
	}
	*/

	if(justEntered)
	{
		if(initialGadgetDelay == 1)
		{
			TheTransitionHandler->setGroup("MainMenuDefaultMenuLogoFade");
			TheWindowManager->winSetFocus( parentMainMenu );
			initialGadgetDelay = 2;
			justEntered = FALSE;
		}
		else
			initialGadgetDelay--;
	}
	
	if(dontAllowTransitions && TheTransitionHandler->isFinished())
		dontAllowTransitions = FALSE;

	if(showLogo && dontAllowTransitions == FALSE)
	{
//		if(showFrames == SHOW_FRAMES_LIMIT)
//		{
//			TheTransitionHandler->remove("MainMenuSinglePlayerMenu");
			switch (showSide) {
			case SHOW_TRAINING:
				TheTransitionHandler->setGroup("MainMenuFactionTraining");
				break;
			case SHOW_CHINA:
				TheTransitionHandler->setGroup("MainMenuFactionChina");
				break;
			case SHOW_GLA:
				TheTransitionHandler->setGroup("MainMenuFactionGLA");
				break;
			case SHOW_USA:
				TheTransitionHandler->setGroup("MainMenuFactionUS");
				break;
			case SHOW_SKIRMISH:
				TheTransitionHandler->setGroup("MainMenuFactionSkirmish");
				break;
			}
			showLogo = FALSE;
//			showFrames = 0;	
//			logoIsShown = TRUE;
//		}
//		else
//			showFrames++;
	}

//	if(showFade)
//	{
//		showFade = FALSE;
//		TheTransitionHandler->reverse("FadeWholeScreen");
//	}
////
//	if (notShown)
//	{
//		if(initialGadgetDelay == 1)
//		{
//			dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
//			TheTransitionHandler->setGroup("MainMenuFade", TRUE);
//			TheTransitionHandler->setGroup("MainMenuDefaultMenu");
//			TheMouse->setVisibility(TRUE);
//			initialGadgetDelay = 2;
//			notShown = FALSE;
//		}
//		else
//			initialGadgetDelay--;
//	}

	if (raiseMessageBoxes)
	{
		RaiseGSMessageBox();
		raiseMessageBoxes = FALSE;
	}

	HTTPThinkWrapper();
	GameSpyUpdateOverlays();
//	if(localAnimateWindowManager)
//		localAnimateWindowManager->update();
//	if(localAnimateWindowManager && pendingDropDown != DROPDOWN_NONE && localAnimateWindowManager->isFinished())
//	{
//		localAnimateWindowManager->reset();
//		if(dropDown != DROPDOWN_NONE)
//			dropDownWindows[dropDown]->winHide(TRUE);
//		dropDown = pendingDropDown;
//		dropDownWindows[dropDown]->winHide(FALSE);
//		localAnimateWindowManager->registerGameWindow(dropDownWindows[dropDown],WIN_ANIMATION_SLIDE_TOP_FAST,TRUE,1,1);						
//		//buttonPushed = FALSE;
//		pendingDropDown = DROPDOWN_NONE;
//	}
//	else if(localAnimateWindowManager && dropDown == DROPDOWN_NONE && pendingDropDown == DROPDOWN_NONE && localAnimateWindowManager->isReversed() && localAnimateWindowManager->isFinished())
//	{
//		localAnimateWindowManager->reset();
//		for(Int i = 1; i < DROPDOWN_COUNT; ++i)
//			dropDownWindows[i]->winHide(TRUE);
//	}
	
	
	
	

	if (startGame && TheShell->isAnimFinished() && TheTransitionHandler->isFinished())
	{
		doGameStart();
	}

	// We'll only be successful if we've requested to 
	if(isShuttingDown && TheShell->isAnimFinished() && TheTransitionHandler->isFinished())
	{
		shutdownComplete(layout);
	}
	

	// We'll only be successful if we've requested to 
//	if(TheShell->isAnimReversed() && TheShell->isAnimFinished())
//		shutdownComplete( layout );

//	if(winVidManager)
//		winVidManager->update();

}  // end MainMenuUpdate

//-------------------------------------------------------------------------------------------------
/** Main menu input callback */
//-------------------------------------------------------------------------------------------------
WindowMsgHandledType MainMenuInput( GameWindow *window, UnsignedInt msg,
																		WindowMsgData mData1, WindowMsgData mData2 )
{

	if(!notShown)
		return MSG_IGNORED;
	
	switch( msg ) 
	{

		// --------------------------------------------------------------------------------------------
		case GWM_MOUSE_POS:
		{
			ICoord2D mouse;
			mouse.x = mData1 & 0xFFFF;
			mouse.y = mData1 >> 16;
			if( mouse.x == 0 && mouse.y == 0)
				break;

			static Int mousePosX = mouse.x;
			static Int mousePosY = mouse.y;
			if(abs(mouse.x - mousePosX) > 20 || abs(mouse.y - mousePosY) > 20)
			{
			
				DEBUG_LOG(("Mouse X:%d, Y:%d\n", mouse.x, mouse.y));
				if(notShown)
				{
					initialGadgetDelay = 1;
					dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
					TheTransitionHandler->setGroup("MainMenuFade", TRUE);
					TheTransitionHandler->setGroup("MainMenuDefaultMenu");
					TheMouse->setVisibility(TRUE);
					notShown = FALSE;
					return MSG_HANDLED;
				}
			}
			
		}  // end char
		break;
		case GWM_CHAR:
		{
			if(notShown)
			{
				initialGadgetDelay = 1;
				dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
				TheTransitionHandler->setGroup("MainMenuFade", TRUE);
				TheTransitionHandler->setGroup("MainMenuDefaultMenu");
				TheMouse->setVisibility(TRUE);
				notShown = FALSE;
				return MSG_HANDLED;
			}
			
		}  // end char

	}  // end switch( msg )
	

	return MSG_IGNORED;

}  // end MainMenuInput
void PrintOffsetsFromControlBarParent( void );
//-------------------------------------------------------------------------------------------------
/** Main menu window system callback */
//-------------------------------------------------------------------------------------------------
WindowMsgHandledType MainMenuSystem( GameWindow *window, UnsignedInt msg, 
										 WindowMsgData mData1, WindowMsgData mData2 )
{
	static Bool triedToInitWOLAPI = FALSE;
	static Bool canInitWOLAPI = FALSE;
	
	switch( msg ) 
	{

		//---------------------------------------------------------------------------------------------
		case GWM_CREATE:
		{
			// ghttpStartup();
			break;
		}  // end case

		//---------------------------------------------------------------------------------------------
		case GWM_DESTROY:
		{
			// ghttpCleanup();
			DEBUG_LOG(("Tearing down GameSpy from MainMenuSystem(GWM_DESTROY)\n"));
			TearDownGameSpy();
			StopAsyncDNSCheck(); // kill off the async DNS check thread in case it is still running
			break;

		}  // end case

		// --------------------------------------------------------------------------------------------
		case GWM_INPUT_FOCUS:
		{

			// if we're givin the opportunity to take the keyboard focus we must say we want it
			if( mData1 == TRUE )
				*(Bool *)mData2 = TRUE;

			break;

		}  // end input
#ifndef _PLAYTEST
		//---------------------------------------------------------------------------------------------
		case GBM_MOUSE_ENTERING:
		{
			GameWindow *control = (GameWindow *)mData1;
			Int controlID = control->winGetWindowId();
			if(controlID == onlineID)
			{
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_ONLINE_HIGHLIGHTED]);
			}
			else if(controlID == networkID)
			{
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_NETWORK_HIGHLIGHTED]);
			}
			else if(controlID == optionsID)
			{
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_OPTIONS_HIGHLIGHTED]);
			}
			else if(controlID == exitID)
			{
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_EXIT_HIGHLIGHTED]);
			}

			else if(controlID == buttonTRAININGID)
			{
				if(dontAllowTransitions && !campaignSelected)
				{
					showLogo = TRUE;
					showSide = SHOW_TRAINING;
				}

				if(campaignSelected || dontAllowTransitions)
					break;
				//TheTransitionHandler->remove("MainMenuSinglePlayerMenu");

				TheTransitionHandler->setGroup("MainMenuFactionTraining");

				//showSelectiveButtons(SHOW_NONE);
			}
			else if(controlID == skirmishID)
			{
				if(dontAllowTransitions && !campaignSelected)
				{
					showLogo = TRUE;
					showSide = SHOW_SKIRMISH;
				}

				if(campaignSelected || dontAllowTransitions)
					break;
				//TheTransitionHandler->remove("MainMenuSinglePlayerMenu");

				TheTransitionHandler->setGroup("MainMenuFactionSkirmish");
				//showSelectiveButtons(SHOW_NONE);
			}
			
			else if(controlID == buttonUSAID)
			{
				if(dontAllowTransitions && !campaignSelected)
				{
					showLogo = TRUE;
					showSide = SHOW_USA;
				}

				if(campaignSelected || dontAllowTransitions)
					break;
				//TheTransitionHandler->remove("MainMenuSinglePlayerMenu");

				TheTransitionHandler->setGroup("MainMenuFactionUS");
//				showLogo = TRUE;
//				showFrames = 0;
//				showSide = SHOW_USA;

			}
			else if(controlID == buttonGLAID)
			{
				if(dontAllowTransitions && !campaignSelected)
				{
					showLogo = TRUE;
					showSide = SHOW_GLA;
				}

				if(campaignSelected || dontAllowTransitions)
					break;
				//TheTransitionHandler->remove("MainMenuSinglePlayerMenu");
				TheTransitionHandler->setGroup("MainMenuFactionGLA");
//				showLogo = TRUE;
//				showFrames = 0;
//				showSide = SHOW_GLA;
			}
			else if(controlID == buttonChinaID)
			{
				if(dontAllowTransitions && !campaignSelected)
				{
					showLogo = TRUE;
					showSide = SHOW_CHINA;
				}
				if(campaignSelected || dontAllowTransitions)
					break;
				//TheTransitionHandler->remove("MainMenuSinglePlayerMenu");
				TheTransitionHandler->setGroup("MainMenuFactionChina");
//				showLogo = TRUE;
//				showFrames = 0;
//				showSide = SHOW_CHINA;
			}
			
		break;
		}
		//---------------------------------------------------------------------------------------------
		case GBM_MOUSE_LEAVING:
		{
			GameWindow *control = (GameWindow *)mData1;
			Int controlID = control->winGetWindowId();

			if(controlID == onlineID)
			{
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_ONLINE_UNHIGHLIGHTED]);
			}
			else if(controlID == networkID)
			{
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_NETWORK_UNHIGHLIGHTED]);
			}
			else if(controlID == optionsID)
			{
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_OPTIONS_UNHIGHLIGHTED]);
			}
			else if(controlID == exitID)
			{
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_EXIT_UNHIGHLIGHTED]);
			}
			else if(controlID == buttonTRAININGID)
			{

				if(dontAllowTransitions && !campaignSelected && showLogo)
				{
					showLogo = FALSE;
					showSide = SHOW_NONE;
				}

				if(campaignSelected || dontAllowTransitions)
					break;
				TheTransitionHandler->reverse("MainMenuFactionTraining");
				
				//showSelectiveButtons(SHOW_NONE);
			}
			else if(controlID == skirmishID)
			{
				if(dontAllowTransitions && !campaignSelected && showLogo)
				{
					showLogo = FALSE;
					showSide = SHOW_NONE;
				}
				if(campaignSelected || dontAllowTransitions)
					break;
				TheTransitionHandler->reverse("MainMenuFactionSkirmish");
				//showSelectiveButtons(SHOW_NONE);
			}
			else if(controlID == buttonUSAID)
			{
				if(dontAllowTransitions && !campaignSelected && showLogo)
				{
					showLogo = FALSE;
					showSide = SHOW_NONE;
				}
				if(campaignSelected || dontAllowTransitions)
					break;
				TheTransitionHandler->reverse("MainMenuFactionUS");
				
				//showSelectiveButtons(SHOW_NONE);
			}
			else if(controlID == buttonGLAID)
			{
				if(dontAllowTransitions && !campaignSelected && showLogo)
				{
					showLogo = FALSE;
					showSide = SHOW_NONE;
				}
				if(campaignSelected || dontAllowTransitions)
					break;
				TheTransitionHandler->reverse("MainMenuFactionGLA");
				//showSelectiveButtons(SHOW_NONE);
			}
			else if(controlID == buttonChinaID)
			{
				if(dontAllowTransitions && !campaignSelected && showLogo)
				{
					showLogo = FALSE;
					showSide = SHOW_NONE;
				}
				if(campaignSelected || dontAllowTransitions)
					break;
				TheTransitionHandler->reverse("MainMenuFactionChina");
				//showSelectiveButtons(SHOW_NONE);
			}	
		break;
		}
#endif _PLAYTEST
		//---------------------------------------------------------------------------------------------
		case GBM_SELECTED:
		{
			
			GameWindow *control = (GameWindow *)mData1;
			Int controlID = control->winGetWindowId();
			
			if(buttonPushed)
				break;
#if defined _DEBUG || defined _INTERNAL
			if( control == buttonCampaign )
			{
				buttonPushed = TRUE;
				TheShell->push(AsciiString( "Menus/MapSelectMenu.wnd" ));
				// As soon as we have a campaign, add it in here!;
			}
#ifdef TEST_COMPRESSION
			else if( control == buttonCompressTest )
			{
				DoCompressTest();
			}
#endif // TEST_COMPRESSION
			else 
#endif
			if( controlID == buttonSinglePlayerID )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				//buttonPushed = TRUE;
				buttonPushed = FALSE;
				dropDownWindows[DROPDOWN_SINGLE]->winHide(FALSE);
				TheTransitionHandler->remove("MainMenuDefaultMenu");
				TheTransitionHandler->reverse("MainMenuDefaultMenuBack");
				TheTransitionHandler->setGroup("MainMenuSinglePlayerMenu");
			}  // end if
			else if( controlID == buttonSingleBackID )
			{
				if(campaignSelected || dontAllowTransitions)
					break;
				buttonPushed = FALSE;
				dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
				TheTransitionHandler->remove("MainMenuSinglePlayerMenu");
				TheTransitionHandler->reverse("MainMenuSinglePlayerMenuBack");
				TheTransitionHandler->setGroup("MainMenuDefaultMenu");
				dontAllowTransitions = TRUE;
			}  // end if
			else if( controlID == buttonMultiBackID )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				buttonPushed = FALSE;
				dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
				TheTransitionHandler->remove("MainMenuMultiPlayerMenu");
				TheTransitionHandler->reverse("MainMenuMultiPlayerMenuReverse");
				TheTransitionHandler->setGroup("MainMenuDefaultMenu");
			}  // end if
			else if( controlID == buttonLoadReplayBackID )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				buttonPushed = FALSE;
				dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
				TheTransitionHandler->remove("MainMenuLoadReplayMenu");
				TheTransitionHandler->reverse("MainMenuLoadReplayMenuBack");
				TheTransitionHandler->setGroup("MainMenuDefaultMenu");
			}  // end if
			
			else if( control == buttonCredits )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				buttonPushed = TRUE;
				TheShell->push("Menus/CreditsMenu.wnd" );
				dropDownWindows[DROPDOWN_MAIN]->winHide(FALSE);
				TheTransitionHandler->reverse("MainMenuDefaultMenu");
			}
			else if( controlID == buttonMultiPlayerID)
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				//buttonPushed = TRUE;
				buttonPushed = FALSE;
				dropDownWindows[DROPDOWN_MULTIPLAYER]->winHide(FALSE);
				TheTransitionHandler->remove("MainMenuDefaultMenu");
				TheTransitionHandler->reverse("MainMenuDefaultMenuBack");
				TheTransitionHandler->setGroup("MainMenuMultiPlayerMenu");		
			}
			else if( controlID == buttonLoadReplayID)
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				//buttonPushed = TRUE;
				buttonPushed = FALSE;
				dropDownWindows[DROPDOWN_LOADREPLAY]->winHide(FALSE);
				TheTransitionHandler->remove("MainMenuDefaultMenu");
				TheTransitionHandler->reverse("MainMenuDefaultMenuBack");
				TheTransitionHandler->setGroup("MainMenuLoadReplayMenu");
			}
			else if( controlID == buttonLoadID )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
//				SaveLoadLayoutType layoutType = SLLT_LOAD_ONLY;
//        WindowLayout *saveLoadMenuLayout = TheShell->getSaveLoadMenuLayout();
//				DEBUG_ASSERTCRASH( saveLoadMenuLayout, ("Unable to get save load menu layout.\n") );
//				saveLoadMenuLayout->runInit( &layoutType );
//				saveLoadMenuLayout->hide( FALSE );
//				saveLoadMenuLayout->bringForward();
				buttonPushed = TRUE;
				dropDownWindows[DROPDOWN_LOADREPLAY]->winHide(FALSE);
				TheTransitionHandler->reverse("MainMenuLoadReplayMenuBackTransition");
				TheShell->push(AsciiString("Menus/SaveLoad.wnd"));

			}
			else if( controlID == buttonReplayID )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				buttonPushed = TRUE;
				dropDownWindows[DROPDOWN_LOADREPLAY]->winHide(FALSE);
				TheTransitionHandler->reverse("MainMenuLoadReplayMenuBackTransition");
				TheShell->push(AsciiString("Menus/ReplayMenu.wnd"));
			}
			else if( controlID == skirmishID )
			{
				if(campaignSelected || dontAllowTransitions)
					break;
				buttonPushed = TRUE;
				campaignSelected = TRUE;
				dropDownWindows[DROPDOWN_SINGLE]->winHide(FALSE);
				TheTransitionHandler->remove("MainMenuFactionSkirmish");

				TheTransitionHandler->reverse("MainMenuSinglePlayerMenuBackSkirmish");
				TheShell->push( AsciiString("Menus/SkirmishGameOptionsMenu.wnd") );
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_SKIRMISH_SELECTED]);
			}
			else if( controlID == onlineID )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				buttonPushed = TRUE;
				dropDownWindows[DROPDOWN_MULTIPLAYER]->winHide(FALSE);
				TheTransitionHandler->reverse("MainMenuMultiPlayerMenuTransitionToNext");

				StartPatchCheck();
//				localAnimateWindowManager->reverseAnimateWindow();
				dropDown = DROPDOWN_NONE;

			}  // end else if
			else if( controlID == networkID )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				buttonPushed = TRUE;
				dropDownWindows[DROPDOWN_MULTIPLAYER]->winHide(FALSE);
				TheTransitionHandler->reverse("MainMenuMultiPlayerMenuTransitionToNext");
				TheShell->push( AsciiString("Menus/LanLobbyMenu.wnd") );

				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_NETWORK_SELECTED]);
			}  // end else if
			else if( controlID == optionsID )
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				//buttonPushed = TRUE;
				TheScriptEngine->signalUIInteract(TheShellHookNames[SHELL_SCRIPT_HOOK_MAIN_MENU_OPTIONS_SELECTED]);

				// load the options menu
				WindowLayout *optLayout = TheShell->getOptionsLayout(TRUE);
				DEBUG_ASSERTCRASH(optLayout != NULL, ("unable to get options menu layout"));
				optLayout->runInit();
				optLayout->hide(FALSE);
				optLayout->bringForward();
			}  // end else if
			else if( controlID == worldBuilderID )
			{
#if 0
#if defined _DEBUG
				if(_spawnl(_P_NOWAIT,"WorldBuilderD.exe","WorldBuilderD.exe", NULL) < 0)
					MessageBoxOk(TheGameText->fetch("GUI:WorldBuilder"), TheGameText->fetch("GUI:WorldBuilderLoadFailed"),NULL);
#elif defined  _INTERNAL
				if(_spawnl(_P_NOWAIT,"WorldBuilderI.exe","WorldBuilderI.exe", NULL) < 0)
					MessageBoxOk(TheGameText->fetch("GUI:WorldBuilder"), TheGameText->fetch("GUI:WorldBuilderLoadFailed"),NULL);
#else
				// if(_spawnl(_P_NOWAIT,"WorldBuilder.exe","WorldBuilder.exe", NULL) < 0)
				// 	MessageBoxOk(TheGameText->fetch("GUI:WorldBuilder"), TheGameText->fetch("GUI:WorldBuilderLoadFailed"),NULL);
#endif
#endif
			}
			else if( controlID == getUpdateID )
			{
				StartDownloadingPatches();
			}
			else if( controlID == exitID )
			{
				// If we ever want to add a dialog before we exit out of the game, uncomment this line and kill the quitCallback() line below.
//#if defined(_DEBUG) || defined(_INTERNAL)
				
				//Added By Sadullah Nader
				//Changed the preprocessing code to normal code
				if (TheGlobalData->m_windowed)
				{
					quitCallback();
//#else	
				}
				else
				{
					QuitMessageBoxYesNo(TheGameText->fetch("GUI:QuitPopupTitle"), TheGameText->fetch("GUI:QuitPopupMessage"),quitCallback,NULL);
				}
				//
//#endif
				
			}  // end else if
#ifndef _PLAYTEST
			else if(controlID == buttonTRAININGID)
			{
				if(campaignSelected || dontAllowTransitions)
					break;
				TheCampaignManager->setCampaign( "TRAINING" );
				TheTransitionHandler->setGroup("MainMenuFactionTraining");
				TheTransitionHandler->remove("MainMenuFactionTraining", TRUE);
				GameWindow *win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionTraining"));
				if(win)
					win->winHide(TRUE);
				TheTransitionHandler->reverse("MainMenuSinglePlayerMenuBackTraining");
				TheTransitionHandler->setGroup("MainMenuDifficultyMenuTraining");
				campaignSelected = TRUE;
				showLogo = FALSE;
				showSide = SHOW_TRAINING;

//				setupGameStart(TheCampaignManager->getCurrentMap());
			}
			else if(controlID == buttonUSAID)
			{
				if(campaignSelected || dontAllowTransitions)
					break;
				TheCampaignManager->setCampaign( "USA" );
				TheTransitionHandler->setGroup("MainMenuFactionUS");
				TheTransitionHandler->remove("MainMenuFactionUS", TRUE);
				GameWindow *win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionUS"));
				if(win)
					win->winHide(TRUE);
				TheTransitionHandler->reverse("MainMenuSinglePlayerMenuBackUS");
				TheTransitionHandler->setGroup("MainMenuDifficultyMenuUS");
				campaignSelected = TRUE;
				logoIsShown = FALSE;
				showLogo = FALSE;
				showSide = SHOW_USA;
//				WindowLayout *layout = NULL;
//				layout = TheWindowManager->winCreateLayout( AsciiString( "Menus/DifficultySelect.wnd" ) );
//				layout->runInit();
//				layout->hide( FALSE );
//				layout->bringForward();

//				setupGameStart(TheCampaignManager->getCurrentMap());
			}
			else if(controlID == buttonGLAID)
			{
				if(campaignSelected || dontAllowTransitions)
					break;
				TheCampaignManager->setCampaign( "GLA" );
				TheTransitionHandler->setGroup("MainMenuFactionGLA");
				TheTransitionHandler->remove("MainMenuFactionGLA", TRUE);
				GameWindow *win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionGLA"));
				if(win)
					win->winHide(TRUE);				
				TheTransitionHandler->reverse("MainMenuSinglePlayerMenuBackGLA");
				TheTransitionHandler->setGroup("MainMenuDifficultyMenuGLA");
				campaignSelected = TRUE;
				logoIsShown = FALSE;
				showLogo = FALSE;
				showSide = SHOW_GLA;
//				WindowLayout *layout = NULL;
//				layout = TheWindowManager->winCreateLayout( AsciiString( "Menus/DifficultySelect.wnd" ) );
//				layout->runInit();
//				layout->hide( FALSE );
//				layout->bringForward();

//				setupGameStart(TheCampaignManager->getCurrentMap());
			}
			else if(controlID == buttonChinaID)
			{
				if(campaignSelected || dontAllowTransitions)
					break;
				TheCampaignManager->setCampaign( "China" );
				TheTransitionHandler->setGroup("MainMenuFactionChina");
				TheTransitionHandler->remove("MainMenuFactionChina", TRUE);
				GameWindow *win = TheWindowManager->winGetWindowFromId(parentMainMenu, TheNameKeyGenerator->nameToKey("MainMenu.wnd:WinFactionChina"));
				if(win)
					win->winHide(TRUE);				
				TheTransitionHandler->reverse("MainMenuSinglePlayerMenuBackChina");
				TheTransitionHandler->setGroup("MainMenuDifficultyMenuChina");
				campaignSelected = TRUE;
				logoIsShown = FALSE;
				showLogo = FALSE;
				showSide = SHOW_CHINA;

//				WindowLayout *layout = NULL;
//				layout = TheWindowManager->winCreateLayout( AsciiString( "Menus/DifficultySelect.wnd" ) );
//				layout->runInit();
//				layout->hide( FALSE );
//				layout->bringForward();

//				setupGameStart(TheCampaignManager->getCurrentMap());
			}// end else if
			else if(controlID == buttonEasyID)
			{
				if(dontAllowTransitions)
					break;

				checkCDBeforeCampaign(DIFFICULTY_EASY);
			}
			else if(controlID == buttonMediumID)
			{
				if(dontAllowTransitions)
					break;

				checkCDBeforeCampaign(DIFFICULTY_NORMAL);
			}
			else if(controlID == buttonHardID)
			{
				if(dontAllowTransitions)
					break;

				checkCDBeforeCampaign(DIFFICULTY_HARD);
			}
			else if(controlID == buttonDiffBackID)
			{
				if(dontAllowTransitions)
					break;
				dontAllowTransitions = TRUE;
				TheCampaignManager->setCampaign( AsciiString::TheEmptyString );
				diffReverseSide();
				campaignSelected = FALSE;
			}

#endif _PLAYTEST

			break;

		}  // end selected
		
		//---------------------------------------------------------------------------------------------
		default:
			return MSG_IGNORED;

	}  // end switch

	return MSG_HANDLED;

}  // end MainMenuSystem

void diffReverseSide( void )
{
	switch (showSide) {
	case SHOW_TRAINING:
		TheTransitionHandler->reverse("MainMenuDifficultyMenuTrainingBack");
		TheTransitionHandler->setGroup("MainMenuSinglePlayerTrainingMenuFromDiff");
		break;
	case SHOW_USA:
		TheTransitionHandler->reverse("MainMenuDifficultyMenuUSBack");
		TheTransitionHandler->setGroup("MainMenuSinglePlayerUSAMenuFromDiff");
		break;
	case SHOW_GLA:
		TheTransitionHandler->reverse("MainMenuDifficultyMenuGLABack");
		TheTransitionHandler->setGroup("MainMenuSinglePlayerGLAMenuFromDiff");
		break;
	case SHOW_CHINA:
		TheTransitionHandler->reverse("MainMenuDifficultyMenuChinaBack");
		TheTransitionHandler->setGroup("MainMenuSinglePlayerChinaMenuFromDiff");

		break;
	}
}
