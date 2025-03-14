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

///////////////////////////////////////////////////////////////////////////////////////
// FILE: LanLobbyMenu.cpp
// Author: Chris Huybregts, October 2001
// Description: Lan Lobby Menu
///////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Lib/BaseType.h"
#include "Common/crc.h"
#include "Common/GameEngine.h"
#include "Common/GlobalData.h"
#include "Common/MultiplayerSettings.h"
#include "Common/NameKeyGenerator.h"
#include "Common/Player.h"
#include "Common/PlayerTemplate.h"
#include "Common/QuotedPrintable.h"
#include "Common/UserPreferences.h"
#include "GameClient/AnimateWindowManager.h"
#include "GameClient/GameText.h"
#include "GameClient/MapUtil.h"
#include "GameClient/Mouse.h"
#include "GameClient/WindowLayout.h"
#include "GameClient/Gadget.h"
#include "GameClient/Shell.h"
#include "GameClient/ShellHooks.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/GameInfoWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GadgetListBox.h"
#include "GameClient/GadgetTextEntry.h"
#include "GameClient/MessageBox.h"
#include "GameClient/GameWindowTransitions.h"
#include "GameLogic/GameLogic.h"
#include "GameNetwork/IPEnumeration.h"
#include "GameNetwork/LANAPICallbacks.h"
#include "GameNetwork/LANGameInfo.h"

Bool LANisShuttingDown = false;
Bool LANbuttonPushed = false;
Bool LANSocketErrorDetected = FALSE;
char *LANnextScreen = NULL;

static Int	initialGadgetDelay = 2;
static Bool justEntered = FALSE;



LANPreferences::LANPreferences( void )
{
	// note, the superclass will put this in the right dir automatically, this is just a leaf name
	load("Network.ini");
}

LANPreferences::~LANPreferences()
{
}

UnicodeString LANPreferences::getUserName(void)
{
	UnicodeString ret;
	LANPreferences::const_iterator it = find("UserName");
	if (it == end())
	{
		IPEnumeration IPs;
		ret.translate(IPs.getMachineName());
		return ret;
	}

	ret = QuotedPrintableToUnicodeString(it->second);
	ret.trim();
	if (ret.isEmpty())
	{
		IPEnumeration IPs;
		ret.translate(IPs.getMachineName());
		return ret;
	}
	
	return ret;
}

Int LANPreferences::getPreferredColor(void)
{
	Int ret;
	LANPreferences::const_iterator it = find("Color");
	if (it == end())
	{
		return -1;
	}

	ret = atoi(it->second.str());
	if (ret < -1 || ret >= TheMultiplayerSettings->getNumColors())
		ret = -1;

	return ret;
}

Int LANPreferences::getPreferredFaction(void)
{
	Int ret;
	LANPreferences::const_iterator it = find("PlayerTemplate");
	if (it == end())
	{
		return PLAYERTEMPLATE_RANDOM;
	}

	ret = atoi(it->second.str());
	if (ret == PLAYERTEMPLATE_OBSERVER || ret < PLAYERTEMPLATE_MIN || ret >= ThePlayerTemplateStore->getPlayerTemplateCount())
		ret = PLAYERTEMPLATE_RANDOM;

	if (ret >= 0)
	{
		const PlayerTemplate *fac = ThePlayerTemplateStore->getNthPlayerTemplate(ret);
		if (!fac)
			ret = PLAYERTEMPLATE_RANDOM;
		else if (fac->getStartingBuilding().isEmpty())
			ret = PLAYERTEMPLATE_RANDOM;
	}

	return ret;
}

Bool LANPreferences::usesSystemMapDir(void)
{
	OptionPreferences::const_iterator it = find("UseSystemMapDir");
	if (it == end())
		return TRUE;

	if (strcasecmp(it->second.str(), "yes") == 0) {
		return TRUE;
	}
	return FALSE;
}

AsciiString LANPreferences::getPreferredMap(void)
{
	AsciiString ret;
	LANPreferences::const_iterator it = find("Map");
	if (it == end())
	{
		ret = getDefaultMap(TRUE);
		return ret;
	}

	ret = QuotedPrintableToAsciiString(it->second);
	ret.trim();
	if (ret.isEmpty() || !isValidMap(ret, TRUE))
	{
		ret = getDefaultMap(TRUE);
		return ret;
	}
	
	return ret;
}

Int LANPreferences::getNumRemoteIPs(void)
{
	Int ret;
	LANPreferences::const_iterator it = find("NumRemoteIPs");
	if (it == end())
	{
		ret = 0;
		return ret;
	}

	ret = atoi(it->second.str());
	return ret;
}

UnicodeString LANPreferences::getRemoteIPEntry(Int i)
{
	UnicodeString ret;
	AsciiString key;
	key.format("RemoteIP%d", i);

	AsciiString ipstr;
	AsciiString asciientry;

	LANPreferences::const_iterator it = find(key.str());
	if (it == end())
	{
		asciientry = "";
		return ret;
	}

	asciientry = it->second;

	asciientry.nextToken(&ipstr, ":");
	asciientry.set(asciientry.str() + 1); // skip the ':'

	ret.translate(ipstr);
	if (asciientry.getLength() > 0)
	{
		ret.concat(L"(");
		ret.concat(QuotedPrintableToUnicodeString(asciientry));
		ret.concat(L")");
	}

	return ret;
}

// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////


// window ids ------------------------------------------------------------------------------
static NameKeyType parentLanLobbyID = NAMEKEY_INVALID;
static NameKeyType buttonBackID = NAMEKEY_INVALID;
static NameKeyType buttonClearID = NAMEKEY_INVALID;
static NameKeyType buttonHostID = NAMEKEY_INVALID;
static NameKeyType buttonJoinID = NAMEKEY_INVALID;
static NameKeyType buttonDirectConnectID = NAMEKEY_INVALID;
static NameKeyType buttonEmoteID = NAMEKEY_INVALID;
static NameKeyType staticToolTipID = NAMEKEY_INVALID;
static NameKeyType textEntryPlayerNameID = NAMEKEY_INVALID;
static NameKeyType textEntryChatID = NAMEKEY_INVALID;
static NameKeyType listboxPlayersID = NAMEKEY_INVALID;
static NameKeyType staticTextGameInfoID = NAMEKEY_INVALID;


// Window Pointers ------------------------------------------------------------------------
static GameWindow *parentLanLobby = NULL;
static GameWindow *buttonBack = NULL;
static GameWindow *buttonClear = NULL;
static GameWindow *buttonHost = NULL;
static GameWindow *buttonJoin = NULL;
static GameWindow *buttonDirectConnect = NULL;
static GameWindow *buttonEmote = NULL;
static GameWindow *staticToolTip = NULL;
static GameWindow *textEntryPlayerName = NULL;
static GameWindow *textEntryChat = NULL;
static GameWindow *staticTextGameInfo = NULL;

//external declarations of the Gadgets the callbacks can use
NameKeyType listboxChatWindowID = NAMEKEY_INVALID;
GameWindow *listboxChatWindow = NULL;
GameWindow *listboxPlayers = NULL;
NameKeyType listboxGamesID = NAMEKEY_INVALID;
GameWindow *listboxGames = NULL;

// hack to disable framerate limiter in LAN games
//static Bool shellmapOn;
static Bool useFpsLimit;
static UnicodeString defaultName;

static void playerTooltip(GameWindow *window,
													WinInstanceData *instData,
													UnsignedInt mouse)
{
	Int x, y, row, col;
	x = LOLONGTOSHORT(mouse);
	y = HILONGTOSHORT(mouse);

	GadgetListBoxGetEntryBasedOnXY(window, x, y, row, col);

	if (row == -1 || col == -1)
	{
		//TheMouse->setCursorTooltip( TheGameText->fetch("TOOLTIP:LobbyPlayers") );
		return;
	}

	UnsignedInt playerIP = (UnsignedInt)GadgetListBoxGetItemData( window, row, col );
	LANPlayer *player = TheLAN->LookupPlayer(playerIP);
	if (!player)
	{
		DEBUG_CRASH(("No player info in listbox!"));
		//TheMouse->setCursorTooltip( TheGameText->fetch("TOOLTIP:LobbyPlayers") );
		return;
	}
	UnicodeString tooltip;
	tooltip.format(TheGameText->fetch("TOOLTIP:LANPlayer"), player->getName().str(), player->getLogin().str(), player->getHost().str());
	TheMouse->setCursorTooltip( tooltip );
}


//-------------------------------------------------------------------------------------------------
/** Initialize the Lan Lobby Menu */
//-------------------------------------------------------------------------------------------------
void LanLobbyMenuInit( WindowLayout *layout, void *userData )
{
	LANnextScreen = NULL;
	LANbuttonPushed = false;
	LANisShuttingDown = false;

	// get the ids for our controls
	parentLanLobbyID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:LanLobbyMenuParent" ) );
	buttonBackID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ButtonBack" ) );
	buttonClearID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ButtonClear" ) );
	buttonHostID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ButtonHost" ) );
	buttonJoinID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ButtonJoin" ) );
	buttonDirectConnectID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ButtonDirectConnect" ) );
	buttonEmoteID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ButtonEmote" ) );
	staticToolTipID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:StaticToolTip" ) );
	textEntryPlayerNameID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:TextEntryPlayerName" ) );
	textEntryChatID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:TextEntryChat" ) );
	listboxPlayersID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ListboxPlayers" ) );
	listboxChatWindowID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ListboxChatWindowLanLobby" ) );
	listboxGamesID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:ListboxGames" ) );
	staticTextGameInfoID = TheNameKeyGenerator->nameToKey( AsciiString( "LanLobbyMenu.wnd:StaticTextGameInfo" ) );


	// Get pointers to the window buttons
	parentLanLobby = TheWindowManager->winGetWindowFromId( NULL, parentLanLobbyID );
	buttonBack = TheWindowManager->winGetWindowFromId( NULL,  buttonBackID);
	buttonClear = TheWindowManager->winGetWindowFromId( NULL,  buttonClearID);
	buttonHost = TheWindowManager->winGetWindowFromId( NULL, buttonHostID );
	buttonJoin = TheWindowManager->winGetWindowFromId( NULL, buttonJoinID );
	buttonDirectConnect = TheWindowManager->winGetWindowFromId( NULL, buttonDirectConnectID );
	buttonEmote = TheWindowManager->winGetWindowFromId( NULL,buttonEmoteID  );
	staticToolTip = TheWindowManager->winGetWindowFromId( NULL, staticToolTipID );
	textEntryPlayerName = TheWindowManager->winGetWindowFromId( NULL, textEntryPlayerNameID );
	textEntryChat = TheWindowManager->winGetWindowFromId( NULL, textEntryChatID );
	listboxPlayers = TheWindowManager->winGetWindowFromId( NULL, listboxPlayersID );
	listboxChatWindow = TheWindowManager->winGetWindowFromId( NULL, listboxChatWindowID );
	listboxGames = TheWindowManager->winGetWindowFromId( NULL, listboxGamesID );
	staticTextGameInfo = TheWindowManager->winGetWindowFromId( NULL, staticTextGameInfoID );
	listboxPlayers->winSetTooltipFunc(playerTooltip);

	// Show Menu
	layout->hide( FALSE );

	// Init LAN API Singleton
	if (!TheLAN)
	{
		TheLAN = NEW LANAPI();	/// @todo clh delete TheLAN and 
		useFpsLimit = TheGlobalData->m_useFpsLimit;
	}
	else
	{
		TheWritableGlobalData->m_useFpsLimit = useFpsLimit;
		TheLAN->reset();
	}

	// Choose an IP address, then initialize the LAN singleton
	UnsignedInt IP = TheGlobalData->m_defaultIP;
	IPEnumeration IPs;

	if (!IP)
	{
		EnumeratedIP *IPlist = IPs.getAddresses();
		/*
		while (IPlist && IPlist->getNext())
		{
			IPlist = IPlist->getNext();
		}
		*/
		DEBUG_ASSERTCRASH(IPlist, ("No IP addresses found!"));
		if (!IPlist)
		{
			/// @todo: display error and exit lan lobby if no IPs are found
		}

		//UnicodeString str;
		//str.format(L"Local IP chosen: %hs", IPlist->getIPstring().str());
		//GadgetListBoxAddEntryText(listboxChatWindow, str, chatSystemColor, -1, 0);
		IP = IPlist->getIP();
	}
	else
	{
		/*
		UnicodeString str;
		str.format(L"Default local IP: %d.%d.%d.%d",
			(IP >> 24),
			(IP >> 16) & 0xFF,
			(IP >> 8) & 0xFF,
			IP & 0xFF);
		GadgetListBoxAddEntryText(listboxChatWindow, str, chatSystemColor, -1, 0);
		*/
	}

	// TheLAN->init() sets us to be in a LAN menu screen automatically.
	TheLAN->init();
	if (TheLAN->SetLocalIP(IP) == FALSE) {
		LANSocketErrorDetected = TRUE;
	}

	//Initialize the gadgets on the window
	//UnicodeString	txtInput;
	//txtInput.translate(IPs.getMachineName());
	LANPreferences prefs;
	defaultName = prefs.getUserName();
	while (defaultName.getLength() > g_lanPlayerNameLength)
		defaultName.removeLastChar();
	GadgetTextEntrySetText( textEntryPlayerName, defaultName);
	// Clear the text entry line
	GadgetTextEntrySetText(textEntryChat, UnicodeString::TheEmptyString);

	GadgetListBoxReset(listboxPlayers);
	GadgetListBoxReset(listboxGames);

	while (defaultName.getLength() > g_lanPlayerNameLength)
		defaultName.removeLastChar();
	TheLAN->RequestSetName(defaultName);
	TheLAN->RequestLocations();

	/*
	UnicodeString unicodeChat;

	unicodeChat = L"Local IP list:";
	GadgetListBoxAddEntryText(listboxChatWindow, unicodeChat, chatSystemColor, -1, 0);

	IPlist = IPs.getAddresses();
	while (IPlist)
	{
		unicodeChat.translate(IPlist->getIPstring());
		GadgetListBoxAddEntryText(listboxChatWindow, unicodeChat, chatSystemColor, -1, 0);
		IPlist = IPlist->getNext();
	}
	*/

	// Set Keyboard to Main Parent
	//TheWindowManager->winSetFocus( parentLanLobby );
	TheWindowManager->winSetFocus( textEntryChat );
	CreateLANGameInfoWindow(staticTextGameInfo);

	//TheShell->showShellMap(FALSE);
	//shellmapOn = FALSE;
	// coming out of a game, re-load the shell map
	TheShell->showShellMap(TRUE);
		
	// check for MOTD
	TheLAN->checkMOTD();
	layout->hide(FALSE);
	layout->bringForward();

	justEntered = TRUE;
	initialGadgetDelay = 2;
	GameWindow *win = TheWindowManager->winGetWindowFromId(NULL, TheNameKeyGenerator->nameToKey("LanLobbyMenu.wnd:GadgetParent"));
	if(win)
		win->winHide(TRUE);

	
	// animate controls
	//TheShell->registerWithAnimateManager(parentLanLobby, WIN_ANIMATION_SLIDE_TOP, TRUE);
//	TheShell->registerWithAnimateManager(buttonHost, WIN_ANIMATION_SLIDE_LEFT, TRUE, 600);
//	TheShell->registerWithAnimateManager(buttonJoin, WIN_ANIMATION_SLIDE_LEFT, TRUE, 400);
//	TheShell->registerWithAnimateManager(buttonDirectConnect, WIN_ANIMATION_SLIDE_LEFT, TRUE, 200);
//	//TheShell->registerWithAnimateManager(buttonOptions, WIN_ANIMATION_SLIDE_LEFT, TRUE, 1);
//	TheShell->registerWithAnimateManager(buttonBack, WIN_ANIMATION_SLIDE_RIGHT, TRUE, 1);

} // GameLobbyMenuInit

//-------------------------------------------------------------------------------------------------
/** This is called when a shutdown is complete for this menu */
//-------------------------------------------------------------------------------------------------
static void shutdownComplete( WindowLayout *layout )
{

	LANisShuttingDown = false;

	// hide the layout
	layout->hide( TRUE );

	// our shutdown is complete
	TheShell->shutdownComplete( layout, (LANnextScreen != NULL) );

	if (LANnextScreen != NULL)
	{
		TheShell->push(LANnextScreen);
	}

	LANnextScreen = NULL;

}  // end if

//-------------------------------------------------------------------------------------------------
/** Lan Lobby menu shutdown method */
//-------------------------------------------------------------------------------------------------
void LanLobbyMenuShutdown( WindowLayout *layout, void *userData )
{
	LANPreferences prefs;
	prefs["UserName"] = UnicodeStringToQuotedPrintable(GadgetTextEntryGetText( textEntryPlayerName ));
	prefs.write();

	DestroyGameInfoWindow();
	// hide menu
	//layout->hide( TRUE );

	TheLAN->RequestLobbyLeave( true );

	// Reset the LAN singleton
	//TheLAN->reset();

	// our shutdown is complete
	//TheShell->shutdownComplete( layout );
	TheWritableGlobalData->m_useFpsLimit = useFpsLimit;

	LANisShuttingDown = true;

	// if we are shutting down for an immediate pop, skip the animations
	Bool popImmediate = *(Bool *)userData;

	LANSocketErrorDetected = FALSE;

	if( popImmediate )
	{

		shutdownComplete( layout );
		return;

	}  //end if

	TheShell->reverseAnimatewindow();
	TheTransitionHandler->reverse("LanLobbyFade");
	//if(	shellmapOn)
//		TheShell->showShellMap(TRUE);
}  // LanLobbyMenuShutdown


//-------------------------------------------------------------------------------------------------
/** Lan Lobby menu update method */
//-------------------------------------------------------------------------------------------------
void LanLobbyMenuUpdate( WindowLayout * layout, void *userData)
{
	if (TheGameLogic->isInShellGame() && TheGameLogic->getFrame() == 1)
	{
		SignalUIInteraction(SHELL_SCRIPT_HOOK_LAN_ENTERED_FROM_GAME);
	}

	if(justEntered)
	{
		if(initialGadgetDelay == 1)
		{
			TheTransitionHandler->setGroup("LanLobbyFade");
			initialGadgetDelay = 2;
			justEntered = FALSE;
		}
		else
			initialGadgetDelay--;
	}

	if(LANisShuttingDown && TheShell->isAnimFinished() && TheTransitionHandler->isFinished())
		shutdownComplete(layout);

	if (TheShell->isAnimFinished() && !LANbuttonPushed && TheLAN)
		TheLAN->update();

	if (LANSocketErrorDetected == TRUE) {
		LANSocketErrorDetected = FALSE;
		DEBUG_LOG(("SOCKET ERROR!  BAILING!\n"));
		MessageBoxOk(TheGameText->fetch("GUI:NetworkError"), TheGameText->fetch("GUI:SocketError"), NULL);

		// we have a socket problem, back out to the main menu.
		TheWindowManager->winSendSystemMsg(buttonBack->winGetParent(), GBM_SELECTED,
																			 (WindowMsgData)buttonBack, buttonBackID);
	}

	
}// LanLobbyMenuUpdate

//-------------------------------------------------------------------------------------------------
/** Lan Lobby menu input callback */
//-------------------------------------------------------------------------------------------------
WindowMsgHandledType LanLobbyMenuInput( GameWindow *window, UnsignedInt msg,
																			 WindowMsgData mData1, WindowMsgData mData2 )
{
	switch( msg ) 
	{

		// --------------------------------------------------------------------------------------------
		case GWM_CHAR:
		{
			UnsignedByte key = mData1;
			UnsignedByte state = mData2;
			if (LANbuttonPushed)
				break;

			switch( key )
			{

				// ----------------------------------------------------------------------------------------
				case KEY_ESC:
				{
					
					//
					// send a simulated selected event to the parent window of the
					// back/exit button
					//
					if( BitTest( state, KEY_STATE_UP ) )
					{
						TheWindowManager->winSendSystemMsg( window, GBM_SELECTED, 
																							(WindowMsgData)buttonBack, buttonBackID );

					}  // end if

					// don't let key fall through anywhere else
					return MSG_HANDLED;

				}  // end escape

			}  // end switch( key )

		}  // end char

	}  // end switch( msg )

	return MSG_IGNORED;
}// LanLobbyMenuInput

//-------------------------------------------------------------------------------------------------
/** Lan Lobby menu window system callback */
//-------------------------------------------------------------------------------------------------
WindowMsgHandledType LanLobbyMenuSystem( GameWindow *window, UnsignedInt msg, 
														 WindowMsgData mData1, WindowMsgData mData2 )
{
	UnicodeString txtInput;

	switch( msg )
	{
		
		
		case GWM_CREATE:
			{
				SignalUIInteraction(SHELL_SCRIPT_HOOK_LAN_OPENED);
				break;
			} // case GWM_DESTROY:

		case GWM_DESTROY:
			{
				SignalUIInteraction(SHELL_SCRIPT_HOOK_LAN_CLOSED);
				break;
			} // case GWM_DESTROY:

		case GWM_INPUT_FOCUS:
			{	
				// if we're givin the opportunity to take the keyboard focus we must say we want it
				if( mData1 == TRUE )
					*(Bool *)mData2 = TRUE;

				return MSG_HANDLED;
			}//case GWM_INPUT_FOCUS:
		case GLM_DOUBLE_CLICKED:
			{
				if (LANbuttonPushed)
					break;
				GameWindow *control = (GameWindow *)mData1;
				Int controlID = control->winGetWindowId();
				if( controlID == listboxGamesID ) 
				{
					int rowSelected = mData2;
				
					if (rowSelected >= 0)
					{
						LANGameInfo * theGame = TheLAN->LookupGameByListOffset(rowSelected);
						if (theGame)
						{
							TheLAN->RequestGameJoin(theGame);
						}
					}
				}
				break;
			}
		case GLM_SELECTED:
			{
				if (LANbuttonPushed)
					break;
				GameWindow *control = (GameWindow *)mData1;
				Int controlID = control->winGetWindowId();
				if( controlID == listboxGamesID ) 
				{
					int rowSelected = mData2;
					if( rowSelected < 0 )
					{
						HideGameInfoWindow(TRUE);
						break;
					}
					LANGameInfo * theGame = TheLAN->LookupGameByListOffset(rowSelected);
					if (theGame)
						RefreshGameInfoWindow(theGame, theGame->getName());
					else
						HideGameInfoWindow(TRUE);

				}
				break;
			}
		case GBM_SELECTED:
			{
				if (LANbuttonPushed)
					break;
				GameWindow *control = (GameWindow *)mData1;
				Int controlID = control->winGetWindowId();

				if ( controlID == buttonBackID )
				{
					//shellmapOn = TRUE;
					LANbuttonPushed = true;
					DEBUG_LOG(("Back was hit - popping to main menu\n"));
					TheShell->pop();
					delete TheLAN;
					TheLAN = NULL;
					//TheTransitionHandler->reverse("LanLobbyFade");

				} //if ( controlID == buttonBack )
				else if ( controlID == buttonHostID )
				{
					TheLAN->RequestGameCreate( UnicodeString(L""), FALSE);
					
				}//else if ( controlID == buttonHostID )
				else if ( controlID == buttonClearID )
				{
					GadgetTextEntrySetText(textEntryPlayerName, UnicodeString::TheEmptyString);
					TheWindowManager->winSendSystemMsg( window, 
																						GEM_UPDATE_TEXT,
																						(WindowMsgData)textEntryPlayerName, 
																						0 );

				}
				else if ( controlID == buttonJoinID )
				{

					//TheShell->push( AsciiString("Menus/LanGameOptionsMenu.wnd") );

					int rowSelected = -1;
					GadgetListBoxGetSelected( listboxGames, &rowSelected );

					if (rowSelected >= 0)
					{
						LANGameInfo * theGame = TheLAN->LookupGameByListOffset(rowSelected);
						if (theGame)
						{
							TheLAN->RequestGameJoin(theGame);
						}
					}
					else
					{
						GadgetListBoxAddEntryText(listboxChatWindow, TheGameText->fetch("LAN:ErrorNoGameSelected") , chatSystemColor, -1, 0);
					}

				} //else if ( controlID == buttonJoinID )
				else if ( controlID == buttonEmoteID )
				{
					// read the user's input
					txtInput.set(GadgetTextEntryGetText( textEntryChat ));
					// Clear the text entry line
					GadgetTextEntrySetText(textEntryChat, UnicodeString::TheEmptyString);
					// Clean up the text (remove leading/trailing chars, etc)
					txtInput.trim();
					// Echo the user's input to the chat window
					if (!txtInput.isEmpty()) {
//						TheLAN->RequestChat(txtInput, LANAPIInterface::LANCHAT_EMOTE);
						TheLAN->RequestChat(txtInput, LANAPIInterface::LANCHAT_NORMAL);
					}
				} //if ( controlID == buttonEmote )
				else if (controlID == buttonDirectConnectID)
				{
					TheLAN->RequestLobbyLeave( false );
					TheShell->push(AsciiString("Menus/NetworkDirectConnect.wnd"));
				}
				
				break;
			}// case GBM_SELECTED:
	
		case GEM_UPDATE_TEXT:
			{
				if (LANbuttonPushed)
					break;
				GameWindow *control = (GameWindow *)mData1;
				Int controlID = control->winGetWindowId();

				if ( controlID == textEntryPlayerNameID )
				{
					// grab the user's name
					txtInput.set(GadgetTextEntryGetText( textEntryPlayerName ));

					// Clean up the text (remove leading/trailing chars, etc)
					const WideChar *c = txtInput.str();
					while (c && (iswspace(*c)))
						c++;

					if (c)
						txtInput = UnicodeString(c);
					else
						txtInput = UnicodeString::TheEmptyString;

					while (txtInput.getLength() > g_lanPlayerNameLength)
						txtInput.removeLastChar();
					
					if (!txtInput.isEmpty() && txtInput.getCharAt(txtInput.getLength()-1) == L',')
						txtInput.removeLastChar(); // we use , for strtok's so we can't allow them in names.  :(

					if (!txtInput.isEmpty() && txtInput.getCharAt(txtInput.getLength()-1) == L':')
						txtInput.removeLastChar(); // we use : for strtok's so we can't allow them in names.  :(

					if (!txtInput.isEmpty() && txtInput.getCharAt(txtInput.getLength()-1) == L';')
						txtInput.removeLastChar(); // we use ; for strtok's so we can't allow them in names.  :(

					// send it over the network
					if (!txtInput.isEmpty())
						TheLAN->RequestSetName(txtInput);
					else
						{
							TheLAN->RequestSetName(defaultName);
						}

					// Put the whitespace-free version in the box
					GadgetTextEntrySetText( textEntryPlayerName, txtInput );

				}// if ( controlID == textEntryPlayerNameID )
				break;
			}//case GEM_UPDATE_TEXT:
		case GEM_EDIT_DONE:
			{
				if (LANbuttonPushed)
					break;
				GameWindow *control = (GameWindow *)mData1;
				Int controlID = control->winGetWindowId();

				// Take the user's input and echo it into the chat window as well as
				// send it to the other clients on the lan
				if ( controlID == textEntryChatID )
				{
					
					// read the user's input
					txtInput.set(GadgetTextEntryGetText( textEntryChat ));
					// Clear the text entry line
					GadgetTextEntrySetText(textEntryChat, UnicodeString::TheEmptyString);
					// Clean up the text (remove leading/trailing chars, etc)
					while (!txtInput.isEmpty() && iswspace(txtInput.getCharAt(0)))
						txtInput = UnicodeString(txtInput.str()+1);

					// Echo the user's input to the chat window
					if (!txtInput.isEmpty())
						TheLAN->RequestChat(txtInput, LANAPIInterface::LANCHAT_NORMAL);

				}// if ( controlID == textEntryChatID )
				/*
				else if ( controlID == textEntryPlayerNameID )
				{
					// grab the user's name
					txtInput.set(GadgetTextEntryGetText( textEntryPlayerName ));

					// Clean up the text (remove leading/trailing chars, etc)
					txtInput.trim();

					// send it over the network
					if (!txtInput.isEmpty())
						TheLAN->RequestSetName(txtInput);

					// Put the whitespace-free version in the box
					GadgetTextEntrySetText( textEntryPlayerName, txtInput );

				}// if ( controlID == textEntryPlayerNameID )
				*/
				break;
			}
		default:
			return MSG_IGNORED;

	}//Switch

	return MSG_HANDLED;
}// LanLobbyMenuSystem
