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

// InGameUI.cpp ///////////////////////////////////////////////////////////////////////////////////
// Implementation of in-game user interface singleton inteface
// Author: Michael S. Booth, March 2001
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_SHADOW_NAMES

#include "Common/ActionManager.h"
#include "Common/GameAudio.h"
#include "Common/GameEngine.h"
#include "Common/GameType.h"
#include "Common/MessageStream.h"
#include "Common/PerfTimer.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Radar.h"
#include "Common/Team.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/BuildAssistant.h"
#include "Common/Recorder.h"
#include "Common/BuildAssistant.h"
#include "Common/SpecialPower.h"

#include "GameClient/Anim2D.h"
#include "GameClient/ControlBar.h"
#include "GameClient/DisplayStringManager.h"
#include "GameClient/Diplomacy.h"
#include "GameClient/GameText.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/Drawable.h"
#include "GameClient/GadgetPushButton.h"
#include "GameClient/GameClient.h"
#include "GameClient/GameWindowGlobal.h"
#include "GameClient/GameWindowID.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/InGameUI.h"
#include "GameClient/VideoPlayer.h"
#include "GameClient/Mouse.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/View.h"
#include "GameClient/TerrainVisual.h"	
#include "GameClient/ControlBar.h"
#include "GameClient/Display.h"
#include "GameClient/WindowLayout.h"
#include "GameClient/LookAtXlat.h"
#include "GameClient/SelectionXlat.h"
#include "GameClient/Shadow.h"
#include "GameClient/GlobalLanguage.h"

#include "GameLogic/AIGuard.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Object.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/SpecialPowerModule.h"
#include "GameLogic/Module/StealthUpdate.h"
#include "GameLogic/Module/SupplyWarehouseDockUpdate.h"
#include "GameLogic/Module/MobMemberSlavedUpdate.h"//ML

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif


// ------------------------------------------------------------------------------------------------
static const Real placementOpacity = 0.45f;
static const RGBColor illegalBuildColor = { 1.0, 0.0, 0.0 };

//-------------------------------------------------------------------------------------------------
/// The InGameUI singleton instance.
InGameUI *TheInGameUI = NULL;

GameWindow *m_replayWindow = NULL;

// ------------------------------------------------------------------------------------------------
struct TypeSelectionData
{
	GameMessage *m_message;
	const ThingTemplate *m_template;
};

// ------------------------------------------------------------------------------------------------
struct SelectionData
{
	const ThingTemplate *templateToSelect;
	DrawableList newlySelectedDrawables;
	Bool isCarBomb;
};
// ------------------------------------------------------------------------------------------------
static Bool similarUnitSelection( Drawable *test, void *userData )
{
	SelectionData *data = (SelectionData *) userData;
	const ThingTemplate *selectedType = data->templateToSelect;

	if( test )
	{
		const Object *object = test->getObject();
		// Only things with objects can be selected, and the code below isn't 
		// safe unless you've verified that there is a valid object.
		if (!object)
			return FALSE;

		Bool isEquivalent = object->getTemplate()->isEquivalentTo( selectedType );
		if( data->isCarBomb && !isEquivalent && object->testStatus( OBJECT_STATUS_IS_CARBOMB ) )
		{
			isEquivalent = TRUE;
		}

		// only select objects if not already selected
		if( object && isEquivalent 
			  && object->isLocallyControlled() 
				&& !object->isContained()
				&& !( object->getDrawable()->isSelected() ) 
				&& object->isMassSelectable() // And only if they can be multiply selected. (otherwise the drawable will be, but the object will not be)
				)
		{
			// enforce optional unit cap
			if (TheInGameUI->getMaxSelectCount() > 0 && TheInGameUI->getSelectCount() >= TheInGameUI->getMaxSelectCount())
			{
				if ( !TheInGameUI->getDisplayedMaxWarning() )
				{
					TheInGameUI->setDisplayedMaxWarning( TRUE );
					UnicodeString msg;
					msg.format(TheGameText->fetch("GUI:MaxSelectionSize").str(), TheInGameUI->getMaxSelectCount());
					TheInGameUI->message(msg);
				}
			}
			else
			{
				TheInGameUI->selectDrawable( test );
				TheInGameUI->setDisplayedMaxWarning( FALSE );
				data->newlySelectedDrawables.push_back(test);
				return TRUE;
			}	
		}
	}
	return FALSE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void showReplayControls( void )
{
	if (m_replayWindow)
	{
#if !defined(_PLAYTEST)
		Bool show = TheGameLogic->isInReplayGame();
		m_replayWindow->winHide(!show);
#else
		m_replayWindow->winHide(TRUE);
#endif
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void hideReplayControls( void )
{
	if (m_replayWindow)
	{
		m_replayWindow->winHide(TRUE);
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void toggleReplayControls( void )
{
	if (m_replayWindow)
	{
#if !defined(_PLAYTEST)
		Bool show = TheGameLogic->isInReplayGame() && m_replayWindow->winIsHidden();
		m_replayWindow->winHide(!show);
#else
		m_replayWindow->winHide(TRUE);
#endif
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SuperweaponInfo::SuperweaponInfo(
	ObjectID id,
	UnsignedInt timestamp,
	Bool hiddenByScript,
	Bool hiddenByScience,
	Bool ready,
	const AsciiString& superweaponNormalFont, 
	Int superweaponNormalPointSize, 
	Bool superweaponNormalBold,
	Color c, 
	const SpecialPowerTemplate* spt
) :
	m_id(id),
	m_timestamp(timestamp),
	m_hiddenByScript(hiddenByScript),
	m_hiddenByScience(hiddenByScience),
	m_ready(ready),
	m_forceUpdateText(false),
	m_nameDisplayString(NULL),
	m_timeDisplayString(NULL),
	m_color(c),
	m_powerTemplate(spt)
{
	m_nameDisplayString = TheDisplayStringManager->newDisplayString();
	m_nameDisplayString->reset();
	m_nameDisplayString->setText( UnicodeString::TheEmptyString );

	m_timeDisplayString = TheDisplayStringManager->newDisplayString();
	m_timeDisplayString->reset();
	m_timeDisplayString->setText( UnicodeString::TheEmptyString );

	setFont( superweaponNormalFont, superweaponNormalPointSize, superweaponNormalBold );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SuperweaponInfo::~SuperweaponInfo()
{
	if (m_nameDisplayString)
		TheDisplayStringManager->freeDisplayString( m_nameDisplayString );
	m_nameDisplayString = NULL;

	if (m_timeDisplayString)
		TheDisplayStringManager->freeDisplayString( m_timeDisplayString );
	m_timeDisplayString = NULL;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void SuperweaponInfo::setFont(const AsciiString& superweaponNormalFont, Int superweaponNormalPointSize, Bool superweaponNormalBold)
{
	m_nameDisplayString->setFont( TheFontLibrary->getFont( superweaponNormalFont, 
		TheGlobalLanguageData->adjustFontSize(superweaponNormalPointSize), superweaponNormalBold ) );
	m_timeDisplayString->setFont( TheFontLibrary->getFont( superweaponNormalFont, 
		TheGlobalLanguageData->adjustFontSize(superweaponNormalPointSize), superweaponNormalBold ) );
}

// ------------------------------------------------------------------------------------------------
void SuperweaponInfo::setText(const UnicodeString& name, const UnicodeString& time)
{
	m_nameDisplayString->setText(name);
	m_timeDisplayString->setText(time);
}

// ------------------------------------------------------------------------------------------------
void SuperweaponInfo::drawName(Int x, Int y, Color color, Color dropColor)
{
	if (color == 0)
		color = m_color;
 	m_nameDisplayString->draw(x - m_nameDisplayString->getWidth(), y, color, dropColor);
}

// ------------------------------------------------------------------------------------------------
void SuperweaponInfo::drawTime(Int x, Int y, Color color, Color dropColor)
{
	if (color == 0)
		color = m_color;
 	m_timeDisplayString->draw(x, y, color, dropColor);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Real SuperweaponInfo::getHeight() const
{
	return m_nameDisplayString->getFont()->height;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void InGameUI::crc( Xfer *xfer )
{

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version 
	* 2: Save NamedTimers, but not specifically their Info structs.  We'll recreate them.
*/
// ------------------------------------------------------------------------------------------------
void InGameUI::xfer( Xfer *xfer )
{
	// version
	const XferVersion currentVersion = 2;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	if( version >= 2 )
	{
		// Saving the named timer infos and their friends so we get script timers back after we load
		xfer->xferInt(&m_namedTimerLastFlashFrame);
		xfer->xferBool(&m_namedTimerUsedFlashColor);
		xfer->xferBool(&m_showNamedTimers);

		// For the timers themselves, all I need to save is the things that are used in the call to addNamedTimer.
		// It is okay to do this, because SuperweaponInfos pushes things on to a map; addNamedTimer is just a more
		// organized way to push things on the namedTimer Map.
		// addNamedTimer needs (const AsciiString& timerName, const UnicodeString& text, Bool isCountdown)
		if (xfer->getXferMode() == XFER_SAVE)
		{
			Int timerCount = m_namedTimers.size();
			xfer->xferInt( &timerCount );
			for( NamedTimerMapIt timerIter = m_namedTimers.begin(); timerIter != m_namedTimers.end(); ++timerIter )
			{
				xfer->xferAsciiString( &(timerIter->second->m_timerName) );
				xfer->xferUnicodeString( &(timerIter->second->timerText) );
				xfer->xferBool( &(timerIter->second->isCountdown) );
			}
		}
		else // iz a Load
		{
			Int timerCount;
			xfer->xferInt( &timerCount );
			for( Int timerIndex = 0; timerIndex < timerCount; ++timerIndex )
			{
				AsciiString timerName;
				UnicodeString timerText;
				Bool isCountdown;
				xfer->xferAsciiString( &timerName );
				xfer->xferUnicodeString( &timerText );
				xfer->xferBool( &isCountdown );

				addNamedTimer( timerName, timerText, isCountdown );
			}
		}
	}

	xfer->xferBool(&m_superweaponHiddenByScript);
	//xfer->xferBool(&m_inputEnabled);	// no, don't save this yet. somewhat problematic.

	if (xfer->getXferMode() == XFER_SAVE)
	{
		for (Int playerIndex = 0; playerIndex < MAX_PLAYER_COUNT; ++playerIndex)
		{
			for (SuperweaponMap::iterator mapIt = m_superweapons[playerIndex].begin(); mapIt != m_superweapons[playerIndex].end(); ++mapIt)
			{
				AsciiString powerName = mapIt->first;
				SuperweaponList& swList = mapIt->second;
				for (SuperweaponList::iterator listIt = swList.begin(); listIt != swList.end(); ++listIt)
				{
					SuperweaponInfo* swInfo = *listIt;

					// since this list tends to be somewhat sparse, we write stuff out pretty explicitly.
					xfer->xferInt(&playerIndex);
					
					AsciiString templateName = swInfo->getSpecialPowerTemplate()->getName();

					xfer->xferAsciiString(&templateName);
					xfer->xferAsciiString(&powerName);
					xfer->xferObjectID(&swInfo->m_id);
					xfer->xferUnsignedInt(&swInfo->m_timestamp);
					xfer->xferBool(&swInfo->m_hiddenByScript);
					xfer->xferBool(&swInfo->m_hiddenByScience);
					xfer->xferBool(&swInfo->m_ready);
				}
			}
		}
		Int noMorePlayers = -1;		// our "done" sentinel
		xfer->xferInt(&noMorePlayers);
	}
	else if (xfer->getXferMode() == XFER_LOAD)
	{
		for (;;)
		{
			Int playerIndex;
			xfer->xferInt(&playerIndex);

			if (playerIndex == -1)
			{
				break;	// our "done" sentinel
			}
			else if (playerIndex < 0 || playerIndex >= MAX_PLAYER_COUNT)
			{
				DEBUG_CRASH(("SWInfo bad plyrindex\n"));
				throw INI_INVALID_DATA;
			}

			AsciiString templateName;
			xfer->xferAsciiString(&templateName);
			const SpecialPowerTemplate* powerTemplate = TheSpecialPowerStore->findSpecialPowerTemplate(templateName);
			if (powerTemplate == NULL)
			{
				DEBUG_CRASH(("power %s not found\n",templateName.str()));
				throw INI_INVALID_DATA;
			}

			AsciiString powerName;
			ObjectID id;
			UnsignedInt timestamp;
			Bool hiddenByScript, hiddenByScience, ready;

			xfer->xferAsciiString(&powerName);
			xfer->xferObjectID(&id);
			xfer->xferUnsignedInt(&timestamp);
			xfer->xferBool(&hiddenByScript);
			xfer->xferBool(&hiddenByScience);
			xfer->xferBool(&ready);

			// srj sez: due to order-of-operation stuff, sometimes these will already exist,
			// sometimes not. not sure why. so handle both cases. 
			SuperweaponInfo* swInfo = findSWInfo(playerIndex, powerName, id, powerTemplate);
			if (swInfo == NULL)
			{
				const Player* player = ThePlayerList->getNthPlayer(playerIndex);
				swInfo = newInstance(SuperweaponInfo)(
					id,
					timestamp,
					hiddenByScript,
					hiddenByScience,
					ready,
					m_superweaponNormalFont, 
					m_superweaponNormalPointSize, 
					m_superweaponNormalBold, 
					player->getPlayerColor(), 
					powerTemplate);
				m_superweapons[playerIndex][powerName].push_back(swInfo);
			}
			else
			{
				// swInfo->m_id = id;	// redundant, already matches
				swInfo->m_timestamp = timestamp;
				swInfo->m_hiddenByScript = hiddenByScript;
				swInfo->m_hiddenByScience = hiddenByScience;
				swInfo->m_ready = ready;
			}
			swInfo->m_forceUpdateText = true;
		
		}
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void InGameUI::loadPostProcess( void )
{

}  // end loadPostProcess

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::setMouseCursor(Mouse::MouseCursor c)
{
	if (!TheMouse)
		return;

	TheMouse->setCursor(c);

	if (m_mouseMode == MOUSEMODE_GUI_COMMAND && c != Mouse::ARROW && c != Mouse::SCROLL)
		m_mouseModeCursor = c;

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SuperweaponInfo* InGameUI::findSWInfo(Int playerIndex, const AsciiString& powerName, ObjectID id, const SpecialPowerTemplate *powerTemplate)
{
	SuperweaponMap::iterator mapIt = m_superweapons[playerIndex].find(powerName);
	if (mapIt != m_superweapons[playerIndex].end())
	{
		for (SuperweaponList::iterator listIt = mapIt->second.begin(); listIt != mapIt->second.end(); ++listIt)
		{
			if ((*listIt)->m_id == id)
			{
				return *listIt;
			}
		}
	}
	return NULL;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::addSuperweapon(Int playerIndex, const AsciiString& powerName, ObjectID id, const SpecialPowerTemplate *powerTemplate)
{
	if (powerTemplate == NULL)
		return;

	// srj sez: don't allow adding the same superweapon more than once. it can happen. not sure how. (srj)
	SuperweaponInfo* swInfo = findSWInfo(playerIndex, powerName, id, powerTemplate);
	if (swInfo != NULL)
		return;

	const Player* player = ThePlayerList->getNthPlayer(playerIndex);
	Bool hiddenByScience = (powerTemplate->getRequiredScience() != SCIENCE_INVALID) && (player->hasScience(powerTemplate->getRequiredScience()) == false);

	DEBUG_LOG(("Adding superweapon UI timer\n"));
	SuperweaponInfo *info = newInstance(SuperweaponInfo)(
					id,
					-1,			// timestamp
					FALSE,	// hiddenByScript
					hiddenByScience,//Aaayeeee! This is meaningless and just clogs up the works, sez srj, nuke or repair or SHIP WITH(tm), ASAP
													// THe trouble is: There is no mechanism to clear this bit when the science is granted, thus,
													// the timer never, ever, ever get drawn.... unless the owning object is post-science constructed.
					FALSE,	// ready
					m_superweaponNormalFont, 
					m_superweaponNormalPointSize, 
					m_superweaponNormalBold, 
					player->getPlayerColor(), 
					powerTemplate);

	m_superweapons[playerIndex][powerName].push_back(info);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool InGameUI::removeSuperweapon(Int playerIndex, const AsciiString& powerName, ObjectID id, const SpecialPowerTemplate *powerTemplate)
{
	DEBUG_LOG(("Removing superweapon UI timer\n"));
	SuperweaponMap::iterator mapIt = m_superweapons[playerIndex].find(powerName);
	if (mapIt != m_superweapons[playerIndex].end())
	{
		SuperweaponList& swList = mapIt->second;
		for (SuperweaponList::iterator listIt = swList.begin(); listIt != swList.end(); ++listIt)
		{
			if ((*listIt)->m_id == id)
			{
				SuperweaponInfo *info = *listIt;
				swList.erase(listIt);
				info->deleteInstance();
				if (swList.size() == 0)
				{
					m_superweapons[playerIndex].erase(mapIt);
				}
				return TRUE;
			}
		}
	}

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::objectChangedTeam(const Object *obj, Int oldPlayerIndex, Int newPlayerIndex)
{
	// if we already had it listed, remove and re-add it
	if (obj && oldPlayerIndex >= 0 && newPlayerIndex >= 0)
	{
		ObjectID id = obj->getID();
		AsciiString powerName;
		for (BehaviorModule** m = obj->getBehaviorModules(); *m; ++m)
		{
			SpecialPowerModuleInterface* sp = (*m)->getSpecialPower();
			if (!sp)
				continue;

			const SpecialPowerTemplate *powerTemplate = sp->getSpecialPowerTemplate();
			powerName = powerTemplate->getName();

			SuperweaponMap::iterator mapIt = m_superweapons[oldPlayerIndex].find(powerName);
			Bool found = false;
			if (mapIt != m_superweapons[oldPlayerIndex].end())
			{
				for (SuperweaponList::iterator listIt = mapIt->second.begin(); listIt != mapIt->second.end(); ++listIt)
				{
					if ((*listIt)->m_id == id)
					{
						removeSuperweapon(oldPlayerIndex, powerName, id, powerTemplate);
						addSuperweapon(newPlayerIndex, powerName, id, powerTemplate);
						found = true;
						break;
					}
				}
			}
			if (!found)
			{
				if( TheGameLogic->getFrame() == 0 && BitTest( obj->getStatusBits(), OBJECT_STATUS_UNDER_CONSTRUCTION ) == FALSE &&
					obj->isKindOf( KINDOF_COMMANDCENTER ) == FALSE )
					addSuperweapon(newPlayerIndex, powerName, id, powerTemplate);
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::hideObjectSuperweaponDisplayByScript(const Object *obj)
{
	ObjectID objID = obj->getID();
	for (Int playerIndex = 0; playerIndex < MAX_PLAYER_COUNT; ++playerIndex)
	{
		for (SuperweaponMap::iterator mapIt = m_superweapons[playerIndex].begin(); mapIt != m_superweapons[playerIndex].end(); ++mapIt)
		{
			for (SuperweaponList::iterator listIt = mapIt->second.begin(); listIt != mapIt->second.end(); ++listIt)
			{
				if ((*listIt)->m_id == objID)
				{
					(*listIt)->m_hiddenByScript = TRUE;
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::showObjectSuperweaponDisplayByScript(const Object *obj)
{
	ObjectID objID = obj->getID();
	for (Int playerIndex = 0; playerIndex < MAX_PLAYER_COUNT; ++playerIndex)
	{
		for (SuperweaponMap::iterator mapIt = m_superweapons[playerIndex].begin(); mapIt != m_superweapons[playerIndex].end(); ++mapIt)
		{
			for (SuperweaponList::iterator listIt = mapIt->second.begin(); listIt != mapIt->second.end(); ++listIt)
			{
				if ((*listIt)->m_id == objID)
				{
					(*listIt)->m_hiddenByScript = FALSE;
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::setSuperweaponDisplayEnabledByScript(Bool enable)
{
	m_superweaponHiddenByScript = !enable;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool InGameUI::getSuperweaponDisplayEnabledByScript(void) const
{
	return m_superweaponHiddenByScript;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::addNamedTimer( const AsciiString& timerName, const UnicodeString& text, Bool isCountdown )
{
	NamedTimerInfo *info = newInstance( NamedTimerInfo );	
	info->m_timerName = timerName;
	info->color = m_namedTimerNormalColor;
	info->timerText = text;
	info->displayString = TheDisplayStringManager->newDisplayString();
	info->displayString->reset();
	info->displayString->setFont( TheFontLibrary->getFont( m_namedTimerNormalFont, 
		TheGlobalLanguageData->adjustFontSize(m_namedTimerNormalPointSize), m_namedTimerNormalBold ) );
	info->displayString->setText( UnicodeString::TheEmptyString );
	info->timestamp = -1;
	info->isCountdown = isCountdown;

//	GameFont *font = info->displayString->getFont();

	removeNamedTimer(timerName);
	m_namedTimers[timerName] = info;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::removeNamedTimer( const AsciiString& timerName )
{
	NamedTimerMapIt mapIt = m_namedTimers.find(timerName);
	if (mapIt != m_namedTimers.end())
	{
		TheDisplayStringManager->freeDisplayString( mapIt->second->displayString );
		mapIt->second->deleteInstance();
		m_namedTimers.erase(mapIt);
		return;
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::showNamedTimerDisplay( Bool show )
{
	m_showNamedTimers = show;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const FieldParse InGameUI::s_fieldParseTable[] = 
{
	{ "MaxSelectionSize",								INI::parseInt,					NULL,		offsetof( InGameUI, m_maxSelectCount ) },

	{ "MessageColor1",									INI::parseColorInt,			NULL,		offsetof( InGameUI, m_messageColor1 ) },
	{ "MessageColor2",									INI::parseColorInt,			NULL,		offsetof( InGameUI, m_messageColor2 ) },
	{ "MessagePosition",								INI::parseICoord2D,			NULL,		offsetof( InGameUI, m_messagePosition ) },
	{ "MessageFont",										INI::parseAsciiString,	NULL,		offsetof( InGameUI, m_messageFont ) },
	{ "MessagePointSize",								INI::parseInt,					NULL,		offsetof( InGameUI, m_messagePointSize ) },
	{ "MessageBold",										INI::parseBool,					NULL,		offsetof( InGameUI, m_messageBold ) },
	{ "MessageDelayMS",									INI::parseInt,					NULL,		offsetof( InGameUI, m_messageDelayMS ) },

	{ "MilitaryCaptionColor",						INI::parseRGBAColorInt,	NULL,		offsetof( InGameUI, m_militaryCaptionColor ) },
	{ "MilitaryCaptionPosition",				INI::parseICoord2D,			NULL,		offsetof( InGameUI, m_militaryCaptionPosition ) },

	{ "MilitaryCaptionTitleFont",				INI::parseAsciiString,	NULL,		offsetof( InGameUI, m_militaryCaptionTitleFont ) },
	{ "MilitaryCaptionTitlePointSize",	INI::parseInt,					NULL,		offsetof( InGameUI, m_militaryCaptionTitlePointSize ) },
	{ "MilitaryCaptionTitleBold",				INI::parseBool,					NULL,		offsetof( InGameUI, m_militaryCaptionTitleBold ) },

	{ "MilitaryCaptionFont",						INI::parseAsciiString,	NULL,		offsetof( InGameUI, m_militaryCaptionFont ) },
	{ "MilitaryCaptionPointSize",				INI::parseInt,					NULL,		offsetof( InGameUI, m_militaryCaptionPointSize ) },
	{ "MilitaryCaptionBold",						INI::parseBool,					NULL,		offsetof( InGameUI, m_militaryCaptionBold ) },

	{ "MilitaryCaptionRandomizeTyping",	INI::parseBool,					NULL,		offsetof( InGameUI, m_militaryCaptionRandomizeTyping ) },
	{ "MilitaryCaptionSpeed",						INI::parseInt,					NULL,		offsetof( InGameUI, m_militaryCaptionSpeed ) },
	{ "MilitaryCaptionDelayMS",					INI::parseInt,					NULL,		offsetof( InGameUI, m_militaryCaptionDelayMS ) },

	{ "MilitaryCaptionPosition",				INI::parseICoord2D,			NULL,		offsetof( InGameUI, m_militaryCaptionPosition ) },

	{ "SuperweaponCountdownPosition",					INI::parseCoord2D,			NULL,		offsetof( InGameUI, m_superweaponPosition ) },
	{ "SuperweaponCountdownFlashDuration",		INI::parseDurationReal,	NULL,		offsetof( InGameUI, m_superweaponFlashDuration ) },
	{ "SuperweaponCountdownFlashColor",				INI::parseColorInt,			NULL,		offsetof( InGameUI, m_superweaponFlashColor ) },

	{ "SuperweaponCountdownNormalFont",				INI::parseAsciiString,	NULL,		offsetof( InGameUI, m_superweaponNormalFont ) },
	{ "SuperweaponCountdownNormalPointSize",	INI::parseInt,					NULL,		offsetof( InGameUI, m_superweaponNormalPointSize ) },
	{ "SuperweaponCountdownNormalBold",				INI::parseBool,					NULL,		offsetof( InGameUI, m_superweaponNormalBold ) },

	{ "SuperweaponCountdownReadyFont",				INI::parseAsciiString,	NULL,		offsetof( InGameUI, m_superweaponReadyFont ) },
	{ "SuperweaponCountdownReadyPointSize",		INI::parseInt,					NULL,		offsetof( InGameUI, m_superweaponReadyPointSize ) },
	{ "SuperweaponCountdownReadyBold",				INI::parseBool,					NULL,		offsetof( InGameUI, m_superweaponReadyBold ) },

	{ "NamedTimerCountdownPosition",					INI::parseCoord2D,			NULL,		offsetof( InGameUI, m_namedTimerPosition ) },
	{ "NamedTimerCountdownFlashDuration",			INI::parseDurationReal,	NULL,		offsetof( InGameUI, m_namedTimerFlashDuration ) },
	{ "NamedTimerCountdownFlashColor",				INI::parseColorInt,			NULL,		offsetof( InGameUI, m_namedTimerFlashColor ) },

	{ "NamedTimerCountdownNormalFont",				INI::parseAsciiString,	NULL,		offsetof( InGameUI, m_namedTimerNormalFont ) },
	{ "NamedTimerCountdownNormalPointSize",		INI::parseInt,					NULL,		offsetof( InGameUI, m_namedTimerNormalPointSize ) },
	{ "NamedTimerCountdownNormalBold",				INI::parseBool,					NULL,		offsetof( InGameUI, m_namedTimerNormalBold ) },
	{ "NamedTimerCountdownNormalColor",				INI::parseColorInt,			NULL,		offsetof( InGameUI, m_namedTimerNormalColor ) },

	{ "NamedTimerCountdownReadyFont",					INI::parseAsciiString,	NULL,		offsetof( InGameUI, m_namedTimerReadyFont ) },
	{ "NamedTimerCountdownReadyPointSize",		INI::parseInt,					NULL,		offsetof( InGameUI, m_namedTimerReadyPointSize ) },
	{ "NamedTimerCountdownReadyBold",					INI::parseBool,					NULL,		offsetof( InGameUI, m_namedTimerReadyBold ) },
	{ "NamedTimerCountdownReadyColor",				INI::parseColorInt,			NULL,		offsetof( InGameUI, m_namedTimerReadyColor ) },

	{ "FloatingTextTimeOut",									INI::parseDurationUnsignedInt,		NULL,		offsetof( InGameUI, m_floatingTextTimeOut ) },
	{ "FloatingTextMoveUpSpeed",							INI::parseVelocityReal,	NULL,		offsetof( InGameUI, m_floatingTextMoveUpSpeed ) },
	{ "FloatingTextVanishRate",								INI::parseVelocityReal,	NULL,		offsetof( InGameUI, m_floatingTextMoveVanishRate ) },

	{ "PopupMessageColor",								INI::parseColorInt,					NULL,		offsetof( InGameUI, m_popupMessageColor ) },
	
	{ "DrawableCaptionFont",									INI::parseAsciiString,	NULL,		offsetof( InGameUI, m_drawableCaptionFont ) },
	{ "DrawableCaptionPointSize",							INI::parseInt,					NULL,		offsetof( InGameUI, m_drawableCaptionPointSize ) },
	{ "DrawableCaptionBold",									INI::parseBool,					NULL,		offsetof( InGameUI, m_drawableCaptionBold ) },
	{ "DrawableCaptionColor",									INI::parseColorInt,			NULL,		offsetof( InGameUI, m_drawableCaptionColor ) },

	{ "DrawRMBScrollAnchor",									INI::parseBool,					NULL,		offsetof( InGameUI, m_drawRMBScrollAnchor ) },
	{ "MoveRMBScrollAnchor",									INI::parseBool,					NULL,		offsetof( InGameUI, m_moveRMBScrollAnchor ) },

	{ "AttackDamageAreaRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[RADIUSCURSOR_ATTACK_DAMAGE_AREA] ) },
	{ "AttackScatterAreaRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[RADIUSCURSOR_ATTACK_SCATTER_AREA] ) },
	{ "AttackContinueAreaRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[RADIUSCURSOR_ATTACK_CONTINUE_AREA] ) },
	{ "FriendlySpecialPowerRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[RADIUSCURSOR_FRIENDLY_SPECIALPOWER] ) },
	{ "OffensiveSpecialPowerRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[RADIUSCURSOR_OFFENSIVE_SPECIALPOWER] ) },
	{ "SuperweaponScatterAreaRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[RADIUSCURSOR_SUPERWEAPON_SCATTER_AREA] ) },

	{ "GuardAreaRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[RADIUSCURSOR_GUARD_AREA] ) },
	{ "EmergencyRepairRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[RADIUSCURSOR_EMERGENCY_REPAIR] ) },

	{ "ParticleCannonRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_PARTICLECANNON] ) },
	{ "A10StrikeRadiusCursor",			RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_A10STRIKE] ) },
	{ "CarpetBombRadiusCursor",			RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_CARPETBOMB] ) },
	{ "DaisyCutterRadiusCursor",		RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_DAISYCUTTER] ) },
	{ "ParadropRadiusCursor",				RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_PARADROP] ) },
	{ "SpySatelliteRadiusCursor",		RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_SPYSATELLITE] ) },
	
	{ "NuclearMissileRadiusCursor", RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_NUCLEARMISSILE] ) }, 
	{ "EMPPulseRadiusCursor",		  	RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_EMPPULSE] ) },
	{ "ArtilleryRadiusCursor",		  RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_ARTILLERYBARRAGE] ) },
	{ "NapalmStrikeRadiusCursor",		RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_NAPALMSTRIKE] ) },
	{ "ClusterMinesRadiusCursor",		RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_CLUSTERMINES] ) },
	
	{ "ScudStormRadiusCursor",			RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_SCUDSTORM] ) }, 
	{ "AnthraxBombRadiusCursor",		RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_ANTHRAXBOMB] ) },
	{ "AmbushRadiusCursor",					RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_AMBUSH] ) }, 
	{ "RadarRadiusCursor",					RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[	RADIUSCURSOR_RADAR] ) },
	{ "SpyDroneRadiusCursor",				RadiusDecalTemplate::parseRadiusDecalTemplate, NULL, offsetof( InGameUI, m_radiusCursors[ RADIUSCURSOR_SPYDRONE] ) },

	{ NULL,													NULL,										NULL,		0 }  // keep this last
};

//-------------------------------------------------------------------------------------------------
/** Parse MouseCursor entry */
//-------------------------------------------------------------------------------------------------
void INI::parseInGameUIDefinition( INI* ini )
{
	if( TheInGameUI )
	{
		// parse the ini weapon definition
		ini->initFromINI( TheInGameUI, TheInGameUI->getFieldParse() );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
InGameUI::InGameUI()
{
	Int i;

	
  m_inputEnabled = true;
	m_isDragSelecting = false;
	m_nextMoveHint = 0;
	m_selectCount = 0;
	m_frameSelectionChanged = 0;
	m_maxSelectCount = -1;
	m_isScrolling = FALSE;
	m_isSelecting = FALSE;
	m_mouseMode = MOUSEMODE_DEFAULT;
	m_mouseModeCursor = Mouse::ARROW;
	m_mousedOverDrawableID = INVALID_DRAWABLE_ID;
	
	//Added By Sadullah Nader
	//Initializations missing and needed
	m_currentlyPlayingMovie.clear();
	m_militarySubtitle = NULL;
	m_popupMessageData = NULL;
	m_waypointMode = FALSE;
	m_clientQuiet = FALSE;
	
	m_messageColor1 = GameMakeColor( 255, 255, 255, 255 );
	m_messageColor2 = GameMakeColor( 180, 180, 180, 255 );
	m_messagePosition.x = 10;
	m_messagePosition.y = 10;
	m_messageFont = "Arial";
	m_messagePointSize = 10;
	m_messageBold = FALSE;
	m_messageDelayMS = 5000;

	m_militaryCaptionColor.red   = 200;
	m_militaryCaptionColor.green = 200;
	m_militaryCaptionColor.blue  = 30;
	m_militaryCaptionColor.alpha = 255;
	m_militaryCaptionPosition.x = 10;
	m_militaryCaptionPosition.y = 380;

	m_militaryCaptionTitleFont = "Courier";
	m_militaryCaptionTitlePointSize = 12;
	m_militaryCaptionTitleBold = TRUE;

	m_militaryCaptionFont = "Courier";
	m_militaryCaptionPointSize = 12;
	m_militaryCaptionBold = FALSE;

	m_militaryCaptionRandomizeTyping = FALSE;
	m_militaryCaptionSpeed = 1;
	m_militaryCaptionDelayMS = 750;
	m_popupMessageColor = GameMakeColor(255,255,255,255);

	m_tooltipsDisabledUntil = 0;

	// init hint lists
	for( i = 0; i < MAX_MOVE_HINTS; i++ )
	{

		m_moveHint[ i ].pos.zero();
		m_moveHint[ i ].sourceID = 0;
		m_moveHint[ i ].frame = 0;

	}  //  end for i

	for( i = 0; i < MAX_BUILD_PROGRESS; i++ )
	{

		m_buildProgress[ i ].m_thingTemplate = NULL;
		m_buildProgress[ i ].m_percentComplete = 0.0f;
		m_buildProgress[ i ].m_control = NULL;

	}  // end for i

	m_pendingGUICommand = NULL;

	// allocate an array for the placement icons
	m_placeIcon = NEW Drawable* [ TheGlobalData->m_maxLineBuildObjects ];
	for( i = 0; i < TheGlobalData->m_maxLineBuildObjects; i++ )
		m_placeIcon[ i ] = NULL;
	m_pendingPlaceType = NULL;
	m_pendingPlaceSourceObjectID = INVALID_ID;
	m_placeAnchorStart.x = m_placeAnchorStart.y = 0;
	m_placeAnchorEnd.x = m_placeAnchorEnd.y = 0;
	m_placeAnchorInProgress = FALSE;

	m_videoStream = NULL;
	m_videoBuffer = NULL;
	m_cameoVideoStream = NULL;
	m_cameoVideoBuffer = NULL;

	// message info
	for( i = 0; i < MAX_UI_MESSAGES; i++ )
	{

		m_uiMessages[ i ].fullText.clear();
		m_uiMessages[ i ].displayString = NULL;
		m_uiMessages[ i ].timestamp = 0;
		m_uiMessages[ i ].color = 0;

	}  // end for i

	m_replayWindow = NULL;
	m_messagesOn = TRUE;

	m_superweaponPosition.x = 0.7f;
	m_superweaponPosition.y = 0.7f;
	m_superweaponFlashDuration = 1.0f;
	m_superweaponNormalFont = "Arial";
	m_superweaponNormalPointSize = 10;
	m_superweaponNormalBold = FALSE;
	m_superweaponReadyFont = "Arial";
	m_superweaponReadyPointSize = 10;
	m_superweaponReadyBold = FALSE;

	m_superweaponFlashColor = GameMakeColor(255, 255, 255, 255);
	m_superweaponLastFlashFrame = 0;
	m_superweaponUsedFlashColor = TRUE; // so next one is false
	m_superweaponHiddenByScript = FALSE;

	m_namedTimerPosition.x = 0.05f;
	m_namedTimerPosition.y = 0.7f;
	m_namedTimerFlashDuration = 1.0f;
	m_namedTimerNormalFont = "Arial";
	m_namedTimerNormalPointSize = 10;
	m_namedTimerNormalBold = FALSE;
	m_namedTimerReadyFont = "Arial";
	m_namedTimerReadyPointSize = 10;
	m_namedTimerReadyBold = FALSE;


	m_namedTimerNormalColor	= GameMakeColor(255, 255,   0, 255);
	m_namedTimerReadyColor	= GameMakeColor(255,   0, 255, 255);
	m_namedTimerFlashColor	= GameMakeColor(  0, 255, 255, 255);
	m_namedTimerLastFlashFrame = 0;
	m_namedTimerUsedFlashColor = TRUE; // so next one is false
	m_showNamedTimers = TRUE;

	m_floatingTextTimeOut = DEFAULT_FLOATING_TEXT_TIMEOUT;
	m_floatingTextMoveUpSpeed = 1.0f;
	m_floatingTextMoveVanishRate = 0.1f;

	m_drawableCaptionFont = "Arial";
	m_drawableCaptionPointSize = 10;
	m_drawableCaptionBold = FALSE;
	m_drawableCaptionColor = GameMakeColor(255, 255, 255, 255);

	m_drawRMBScrollAnchor = FALSE;
	m_moveRMBScrollAnchor = FALSE;
	m_displayedMaxWarning = FALSE; 

	m_idleWorkerWin = NULL;
	m_currentIdleWorkerDisplay = -1;

	m_waypointMode			= false;
	m_forceAttackMode		= false;
	m_forceMoveToMode		= false;
	m_attackMoveToMode	= false;
	m_preferSelection		= false;

	m_curRcType = RADIUSCURSOR_NONE;
	
	m_soloNexusSelectedDrawableID = INVALID_DRAWABLE_ID;

}  // end InGameUI

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
InGameUI::~InGameUI()
{
	delete TheControlBar;
	TheControlBar = NULL;

	// free all the display strings if we're
	removeMilitarySubtitle();

	stopMovie();
	stopCameoMovie();

	// remove any build available status
	placeBuildAvailable( NULL, NULL );
	setRadiusCursorNone();

	// delete the message resources
	freeMessageResources();

	// delete the array for the drawbles
	delete [] m_placeIcon;
	m_placeIcon = NULL;

	// clear floating text
	clearFloatingText();

	// clear world animations
	clearWorldAnimations();
	resetIdleWorker();
}

//-------------------------------------------------------------------------------------------------
/** Initialize the in game user interface */
//-------------------------------------------------------------------------------------------------
void InGameUI::init( void )
{
	INI ini;
	ini.load( AsciiString( "Data\\INI\\InGameUI.ini" ), INI_LOAD_OVERWRITE, NULL );

	//override INI values with language localized values:
	if (TheGlobalLanguageData)
	{
		if (TheGlobalLanguageData->m_drawableCaptionFont.name.isNotEmpty())
		{	m_drawableCaptionFont = TheGlobalLanguageData->m_drawableCaptionFont.name;
			m_drawableCaptionPointSize = TheGlobalLanguageData->m_drawableCaptionFont.size;
			m_drawableCaptionBold = TheGlobalLanguageData->m_drawableCaptionFont.bold;
		}

		if (TheGlobalLanguageData->m_messageFont.name.isNotEmpty())
		{	m_messageFont = TheGlobalLanguageData->m_messageFont.name;
			m_messagePointSize = TheGlobalLanguageData->m_messageFont.size;
			m_messageBold = TheGlobalLanguageData->m_messageFont.bold;
		}

		if (TheGlobalLanguageData->m_militaryCaptionTitleFont.name.isNotEmpty())
		{	m_militaryCaptionTitleFont = TheGlobalLanguageData->m_militaryCaptionTitleFont.name;
			m_militaryCaptionTitlePointSize = TheGlobalLanguageData->m_militaryCaptionTitleFont.size;
			m_militaryCaptionTitleBold = TheGlobalLanguageData->m_militaryCaptionTitleFont.bold;
		}

		if (TheGlobalLanguageData->m_militaryCaptionFont.name.isNotEmpty())
		{	m_militaryCaptionFont = TheGlobalLanguageData->m_militaryCaptionFont.name;
			m_militaryCaptionPointSize = TheGlobalLanguageData->m_militaryCaptionFont.size;
			m_militaryCaptionBold = TheGlobalLanguageData->m_militaryCaptionFont.bold;
		}

		if (TheGlobalLanguageData->m_superweaponCountdownNormalFont.name.isNotEmpty())
		{	m_superweaponNormalFont = TheGlobalLanguageData->m_superweaponCountdownNormalFont.name;
			m_superweaponNormalPointSize = TheGlobalLanguageData->m_superweaponCountdownNormalFont.size;
			m_superweaponNormalBold = TheGlobalLanguageData->m_superweaponCountdownNormalFont.bold;
		}

		if (TheGlobalLanguageData->m_superweaponCountdownReadyFont.name.isNotEmpty())
		{	m_superweaponReadyFont = TheGlobalLanguageData->m_superweaponCountdownReadyFont.name;
			m_superweaponReadyPointSize = TheGlobalLanguageData->m_superweaponCountdownReadyFont.size;
			m_superweaponReadyBold = TheGlobalLanguageData->m_superweaponCountdownReadyFont.bold;
		}

		if (TheGlobalLanguageData->m_namedTimerCountdownNormalFont.name.isNotEmpty())
		{	m_namedTimerNormalFont = TheGlobalLanguageData->m_namedTimerCountdownNormalFont.name;
			m_namedTimerNormalPointSize = TheGlobalLanguageData->m_namedTimerCountdownNormalFont.size;
			m_namedTimerNormalBold = TheGlobalLanguageData->m_namedTimerCountdownNormalFont.bold;
		}

		if (TheGlobalLanguageData->m_namedTimerCountdownReadyFont.name.isNotEmpty())
		{	m_namedTimerReadyFont = TheGlobalLanguageData->m_namedTimerCountdownReadyFont.name;
			m_namedTimerReadyPointSize = TheGlobalLanguageData->m_namedTimerCountdownReadyFont.size;
			m_namedTimerReadyBold = TheGlobalLanguageData->m_namedTimerCountdownReadyFont.bold;
		}
	}

	/**@ todo we used to put in the hint spy translator, but it's difficult
	to order the translators when the code is not centralized so it has
	been moved to where all the other translators are attached in game client */

	// create the tactical view
	if (TheDisplay)
	{
		TheTacticalView = createView();
		TheTacticalView->init();
		TheDisplay->attachView( TheTacticalView );

		// make the tactical display the full screen width for now
		TheTacticalView->setWidth( TheDisplay->getWidth());
		// make the tactical display 0.76 of full screen so no drawing under GUI.
		TheTacticalView->setHeight( TheDisplay->getHeight() * 0.77f);
	}
	TheTacticalView->setDefaultView(0.0f, 0.0f, 1.0f);

	/** @todo this may be the wrong place to create the sidebar, but for now
	this is where it lives */
	createControlBar();

	/** @todo This may be the wrong place to create the replay menu, but for now
	this is where it lives */
	createReplayControl();

	// create the command bar
	TheControlBar = NEW ControlBar;
	TheControlBar->init();

	m_windowLayouts.clear();

	m_soloNexusSelectedDrawableID = INVALID_DRAWABLE_ID;


}  // end init

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void InGameUI::setRadiusCursor(RadiusCursorType cursorType, const SpecialPowerTemplate* specPowTempl, WeaponSlotType weaponSlot)
{
	if (cursorType == m_curRcType)
		return;

	m_curRadiusCursor.clear();
	m_curRcType = RADIUSCURSOR_NONE;

	if (cursorType == RADIUSCURSOR_NONE)
		return;

	Object* obj = NULL;
	if (m_pendingGUICommand && m_pendingGUICommand->getCommandType() == GUI_COMMAND_SPECIAL_POWER_FROM_COMMAND_CENTER)
	{
		if (ThePlayerList && ThePlayerList->getLocalPlayer())
			obj = ThePlayerList->getLocalPlayer()->findNaturalCommandCenter();
	}
	else
	{
		if (getSelectCount() == 0)
			return;

		Drawable *draw = getFirstSelectedDrawable();
		if (draw == NULL)
			return;

		obj = draw->getObject();
	}

	if (obj == NULL)
		return;
	
	Player* controller = obj->getControllingPlayer();
	if (controller == NULL)
		return;

	Real radius = 0.0f;
	const Weapon* w = NULL;
	switch (cursorType)
	{
		// already handled
		//case RADIUSCURSOR_NONE:
		//	return;
		case RADIUSCURSOR_ATTACK_DAMAGE_AREA:
			w = obj->getWeaponInWeaponSlot(weaponSlot);
			radius = w ? w->getPrimaryDamageRadius(obj) : 0.0f;
			break;
		case RADIUSCURSOR_ATTACK_SCATTER_AREA:
			w = obj->getWeaponInWeaponSlot(weaponSlot);
			radius = w ? (w->getScatterRadius() + w->getScatterTargetScalar()) : 0.0f;
			break;
		case RADIUSCURSOR_ATTACK_CONTINUE_AREA:
			w = obj->getWeaponInWeaponSlot(weaponSlot);
			radius = w ? w->getContinueAttackRange() : 0.0f;
			break;
		case RADIUSCURSOR_GUARD_AREA:
			radius = AIGuardMachine::getStdGuardRange(obj);
			break;
		case RADIUSCURSOR_FRIENDLY_SPECIALPOWER:
		case RADIUSCURSOR_OFFENSIVE_SPECIALPOWER:
		case RADIUSCURSOR_SUPERWEAPON_SCATTER_AREA:
		case RADIUSCURSOR_EMERGENCY_REPAIR:

		case 	RADIUSCURSOR_PARTICLECANNON: 
		case 	RADIUSCURSOR_A10STRIKE:
		case 	RADIUSCURSOR_DAISYCUTTER:
		case 	RADIUSCURSOR_CARPETBOMB:
		case 	RADIUSCURSOR_PARADROP:
		case 	RADIUSCURSOR_SPYSATELLITE: 
		case 	RADIUSCURSOR_NUCLEARMISSILE: 
		case 	RADIUSCURSOR_EMPPULSE:
		case 	RADIUSCURSOR_ARTILLERYBARRAGE:
		case 	RADIUSCURSOR_NAPALMSTRIKE:
		case 	RADIUSCURSOR_CLUSTERMINES:
		case 	RADIUSCURSOR_SCUDSTORM: 
		case 	RADIUSCURSOR_ANTHRAXBOMB:
		case 	RADIUSCURSOR_AMBUSH: 
		case	RADIUSCURSOR_RADAR:
		case	RADIUSCURSOR_SPYDRONE:

			radius = specPowTempl ? specPowTempl->getRadiusCursorRadius() : 0.0f;
			break;

	}

	if (radius <= 0.0f)
		return;

	Coord3D pos = { 0, 0, 0 };	// will be updated right away
	m_radiusCursors[cursorType].createRadiusDecal(pos, radius, controller, m_curRadiusCursor);
	m_curRcType = cursorType;

	handleRadiusCursor();
}

//-------------------------------------------------------------------------------------------------
/** handle updating of "radius cursors" that follow the mouse pos */
//-------------------------------------------------------------------------------------------------
void InGameUI::handleRadiusCursor()
{
	if (!m_curRadiusCursor.isEmpty())
	{
		const MouseIO* mouseIO = TheMouse->getMouseStatus();
		Coord3D pos;

		//
		// if the mouse is in the radar window, the position in the world is that which is
		// represented by the radar, otherwise we use the mouse position itself transformed
		// from screen to world
		// But only if the radar is on.
		//
		Bool radarOn = TheRadar->isRadarForced() 
									|| ( !TheRadar->isRadarHidden() 
												&& ThePlayerList->getLocalPlayer() 
												&& ThePlayerList->getLocalPlayer()->hasRadar()
											);

		if( !radarOn  ||  (TheRadar->screenPixelToWorld( &mouseIO->pos, &pos ) == FALSE) )// if radar off, or point not on radar
			TheTacticalView->screenToTerrain( &mouseIO->pos, &pos );

		m_curRadiusCursor.setPosition(pos);	//world space position of center of decal
		m_curRadiusCursor.update();
	}
}

//-------------------------------------------------------------------------------------------------
/** Handle the placement "icons" that appear at the cursor when we're putting down a 
	* structure to build.  Note that this has additional logic to also show a line
	* of objects because when we build "walls" we want to draw a line of repeating
	* wall pieces on the map where we want to put all of them */
//-------------------------------------------------------------------------------------------------


void InGameUI::evaluateSoloNexus( Drawable *newlyAddedDrawable )
{

	m_soloNexusSelectedDrawableID = INVALID_DRAWABLE_ID;//failsafe...

	// short test: If the thing just added is a nonmobster, bail with NULL
	if ( newlyAddedDrawable )
	{
		const Object *newObj = newlyAddedDrawable->getObject();
		if ( newObj && ! ( newObj->isKindOf(KINDOF_MOB_NEXUS) || newObj->isKindOf(KINDOF_IGNORED_IN_GUI) ) )
			return;
	}

	//LoopAllSelectedDrawables
	UnsignedShort nexaeFound = 0;
	for( DrawableListCIt it = m_selectedDrawables.begin(); it != m_selectedDrawables.end(); ++it ) 
	{

		Drawable *draw = (*it);
		const Object *obj = draw->getObject();


		if ( ! obj )
			continue;
			
		if ( obj->isKindOf( KINDOF_MOB_NEXUS ) )
		{
			++nexaeFound;
			if ( nexaeFound == 1 )
			{
				m_soloNexusSelectedDrawableID = draw->getID();
			}
			else // darn! more than one!
			{
				m_soloNexusSelectedDrawableID = INVALID_DRAWABLE_ID;
				return;
			}
		}
		else if ( ! obj->isKindOf( KINDOF_IGNORED_IN_GUI ) )// darn! a non-angrymobster!
		{
			m_soloNexusSelectedDrawableID = INVALID_DRAWABLE_ID;
			return;
		}

	}  // end for


}


void InGameUI::handleBuildPlacements( void )
{

	//
	// if we're in the process of placing something we need up update one or more drawables
	// based on the position of the mouse
	//
	if( m_pendingPlaceType )
	{
		ICoord2D loc;
		Coord3D world;
		Real angle = m_placeIcon[ 0 ]->getOrientation();

		// update the angle of the icon to match any placement angle and pick the
		// location the icon will be at (anchored is the start, otherwise it's the mouse)
		if( isPlacementAnchored() )
		{
			ICoord2D start, end;
								
			// get the placement arrow points	
			getPlacementPoints( &start, &end );

			// set icon to anchor point
			loc = start;

			// only adjust angle if we've actually moved the mouse
			if( start.x != end.x || start.y != end.y )
			{
				Coord3D worldStart, worldEnd;

				// project the start and the end points of the line anchor into the 3D world
				TheTacticalView->screenToTerrain( &start, &worldStart );
				TheTacticalView->screenToTerrain( &end, &worldEnd );
				
				Coord2D v;
				v.x = worldEnd.x - worldStart.x;
				v.y = worldEnd.y - worldStart.y;
				angle = v.toAngle();

			}  // end if

		}  // end if
		else
		{
			const MouseIO *mouseIO = TheMouse->getMouseStatus();

			// location is the mouse position
			loc = mouseIO->pos;

		}  // end else

		// set the location and angle of the place icon
		/**@todo this whole orientation vector thing is LAME! Must replace, all I want to
		to do is set a simple angle and have it automatically change, ug! */
		TheTacticalView->screenToTerrain( &loc, &world );
		m_placeIcon[ 0 ]->setPosition( &world );
		m_placeIcon[ 0 ]->setOrientation( angle );


		//
		// check to see if this is a legal location to build something at and tint or "un-tint"
		// the cursor icons as appropriate.  This involves a pathfind which could be
		// expensive so we don't want to do it on every frame (althought that would be ideal)
		// If we discover there are cases that this is just too slow we should increase the
		// delay time between checks or we need to come up with a way of recording what is
		// valid and what isn't or "fudge" the results to feel "ok"
		//
		if( TheGameClient->getFrame() & 0x1 )
		{
			TheTerrainVisual->removeAllBibs();

			Object *builderObject = TheGameLogic->findObjectByID( getPendingPlaceSourceObjectID() );

			LegalBuildCode lbc;
			lbc = TheBuildAssistant->isLocationLegalToBuild( &world,
																											 m_pendingPlaceType,
																											 angle,
																											 BuildAssistant::USE_QUICK_PATHFIND |
																											 BuildAssistant::TERRAIN_RESTRICTIONS | 
																											 BuildAssistant::CLEAR_PATH |
																											 BuildAssistant::NO_OBJECT_OVERLAP |
																											 BuildAssistant::SHROUD_REVEALED, 
																											 builderObject,
																											 NULL );
			if( lbc != LBC_OK )
				m_placeIcon[ 0 ]->colorTint( &illegalBuildColor );
			else
				m_placeIcon[ 0 ]->colorTint( NULL );

			


			// Add the bibs around the structure.
			if (lbc != LBC_OK) 
			{
				TheTerrainVisual->addFactionBibDrawable(m_placeIcon[0], lbc != LBC_OK);
			} else {
				TheTerrainVisual->removeFactionBibDrawable(m_placeIcon[0]);
			}
		}  // end if



		//
		// we have additional place icons when we're placing down a line of walls or other
		// similarly placed object ... for those we will have them be oriented the same way
		// as the first one, but we'll set their positions so that they "tile" end to end
		//
		if( isPlacementAnchored() && TheBuildAssistant->isLineBuildTemplate( m_pendingPlaceType ) )
		{
			Int i;

			// get our line placement points
			ICoord2D screenStart, screenEnd;
			getPlacementPoints( &screenStart, &screenEnd );

			// project the start and the end points of the line anchor into the 3D world
			Coord3D worldStart, worldEnd;
			TheTacticalView->screenToTerrain( &screenStart, &worldStart );
			TheTacticalView->screenToTerrain( &screenEnd, &worldEnd );

			// how big are each of our objects
			Real objectSize = m_pendingPlaceType->getTemplateGeometryInfo().getMajorRadius() * 2.0f;
			
			// what is our max tiling length we can make
			Int maxObjects = TheGlobalData->m_maxLineBuildObjects;

			// get the builder object that will be constructing things
			Object *builderObject = TheGameLogic->findObjectByID( TheInGameUI->getPendingPlaceSourceObjectID() );

			//
			// given the start/end points in the world and the the angle of the wall, fill
			// out an array of positions that "tile" this wall across the landscape
			//
			BuildAssistant::TileBuildInfo *tileBuildInfo;
			tileBuildInfo = TheBuildAssistant->buildTiledLocations( m_pendingPlaceType, angle,
																															&worldStart, &worldEnd,
																															objectSize, maxObjects,
																															builderObject );	

			// create any necessary drawables we need to "fill out" the line
			for( i = 0; i < tileBuildInfo->tilesUsed; i++ )
			{
			
				if( m_placeIcon[ i ] == NULL )
					m_placeIcon[ i ] = TheThingFactory->newDrawable( m_pendingPlaceType,
																													 DRAWABLE_STATUS_NO_STATE_PARTICLES );

			}  // end for i

			//
			// destroy any drawables that we're not using anymore because a previous
			// line length was longer
			//
			for( i = tileBuildInfo->tilesUsed; i < maxObjects; i++ )
			{

				if( m_placeIcon[ i ] != NULL )
					TheGameClient->destroyDrawable( m_placeIcon[ i ] );
				m_placeIcon[ i ] = NULL;

			}  // end for i

			//
			// march down each drawable and set the position based on its position in the
			// line and set their angles all the same
			//
			for( i = 0; i < tileBuildInfo->tilesUsed; i++ )
			{

				// set the drawble position
				m_placeIcon[ i ]->setPosition( &tileBuildInfo->positions[ i ] );

				// set opacity for the drawble
				m_placeIcon[ i ]->setDrawableOpacity( placementOpacity );

				// set the drawable angle
				m_placeIcon[ i ]->setOrientation( angle );

			}  // end for i

		}  // end if

	}  // end if

}  // end handleBuildPlacements

//-------------------------------------------------------------------------------------------------
/** Pre-draw phase of the in game ui */
//-------------------------------------------------------------------------------------------------
void InGameUI::preDraw( void )
{

	// handle any "icons" for the act of building things and placing them in the world
	handleBuildPlacements();

	// handle radius-cursors, if any
	handleRadiusCursor();

	// draw the floating text first;
	drawFloatingText();

	// draw world animations
	updateAndDrawWorldAnimations();

}  // end preDraw

//-------------------------------------------------------------------------------------------------
/** Update the in game user interface */
//-------------------------------------------------------------------------------------------------
//DECLARE_PERF_TIMER(InGameUI_update)
void InGameUI::update( void )
{ 
	//USE_PERF_TIMER(InGameUI_update)
	Int i;

	/// @todo make sure this code gets called even when the UI is not being drawn
	if ( m_videoStream && m_videoBuffer )
	{
		if ( m_videoStream->isFrameReady())
		{
			m_videoStream->frameDecompress();
			m_videoStream->frameRender( m_videoBuffer );
			m_videoStream->frameNext();
			if ( m_videoStream->frameIndex() == 0 )
			{
				stopMovie();
			}
		}
	}

	if ( m_cameoVideoStream && m_cameoVideoBuffer )
	{
		if ( m_cameoVideoStream->isFrameReady())
		{
			m_cameoVideoStream->frameDecompress();
			m_cameoVideoStream->frameRender( m_cameoVideoBuffer );
			m_cameoVideoStream->frameNext();
//			if ( m_cameoVideoStream->frameIndex() == 0 )
//			{
//				stopMovie();
//			}
		}
	}

	//
	// remove any message strings that have expired, note that the oldest strings are
	// always at the end of the array (higher index numbers) so we can just remove things
	// from the rear and never have to worry about shifting entries cause we check every
	// frame
	//
	UnsignedInt currLogicFrame = TheGameLogic->getFrame();
	const int messageTimeout = m_messageDelayMS / LOGICFRAMES_PER_SECOND / 1000;
	UnsignedByte r, g, b, a;
	Int amount;
	for( i = MAX_UI_MESSAGES - 1; i >= 0; i-- )
	{

		if( currLogicFrame - m_uiMessages[ i ].timestamp > messageTimeout )
		{

			// get the current color of this text
			GameGetColorComponents( m_uiMessages[ i ].color, &r, &g, &b, &a );

			// start fading the alpha on this color down
			amount = REAL_TO_INT( ((currLogicFrame - m_uiMessages[ i ].timestamp) * 0.01f) );
			if( a - amount < 0 )
				a = 0;
			else
				a -= amount;
			
			// set the new color
			m_uiMessages[ i ].color = GameMakeColor( r, g, b, a );

			// when alpha is completely zero we remove this string
			if( a == 0 )
				removeMessageAtIndex( i );

		}  // end if

	}  // end for i

	//
	// Update the Military Subtitle display
	//
	if( m_militarySubtitle )		// if we have a subtitle, work on it
	{
		// if the timeis frozen by a script, then we still want the text to display
		if(TheScriptEngine->isTimeFrozenScript())
		{
			m_militarySubtitle->lifetime--;
			m_militarySubtitle->blockBeginFrame--;
			m_militarySubtitle->incrementOnFrame--;
		}
		// if it's time to remove the subtitle, Then remove it
		if((Int)m_militarySubtitle->lifetime < (Int)currLogicFrame)
		{
			//steal colins fade from above :)
			GameGetColorComponents( m_militarySubtitle->color, &r, &g, &b, &a );
			// start fading the alpha on this color down
			amount = REAL_TO_INT( ((currLogicFrame - m_militarySubtitle->lifetime ) * 0.1f) );
			if( a - amount < 0 )
			{
				removeMilitarySubtitle();
			}
			else
			{
				a -= amount;
				m_militarySubtitle->color = GameMakeColor(r, g, b, a);
			}
		}
		else
		{
			// trigger whether or not we should draw the block	
			if( m_militarySubtitle->blockBeginFrame + 9 < currLogicFrame )
			{
				m_militarySubtitle->blockBeginFrame = currLogicFrame;
				m_militarySubtitle->blockDrawn = !m_militarySubtitle->blockDrawn;
			}

			// If it's time to add another letter to the display string, lets do that.
			if( m_militarySubtitle->incrementOnFrame < currLogicFrame )
			{
				// first grab the letter we want to add
				WideChar tempWChar = m_militarySubtitle->subtitle.getCharAt(m_militarySubtitle->index);
				// if that letter is a return, add a new line
				if(tempWChar == L'\n')
				{
					// increment the Block position's Y value to draw it on the next line
					Int height;
					m_militarySubtitle->displayStrings[m_militarySubtitle->currentDisplayString]->getSize(NULL, &height);
					m_militarySubtitle->blockPos.y = m_militarySubtitle->blockPos.y + height;

					// Now add a new display string
					m_militarySubtitle->currentDisplayString++;
					if(!(m_militarySubtitle->currentDisplayString >= MAX_SUBTITLE_LINES) )
					{	
						m_militarySubtitle->blockPos.x = m_militarySubtitle->position.x;
						m_militarySubtitle->displayStrings[m_militarySubtitle->currentDisplayString] = TheDisplayStringManager->newDisplayString();
						m_militarySubtitle->displayStrings[m_militarySubtitle->currentDisplayString]->reset();
						m_militarySubtitle->displayStrings[m_militarySubtitle->currentDisplayString]->setFont(	TheFontLibrary->getFont( m_militaryCaptionFont, TheGlobalLanguageData->adjustFontSize(m_militaryCaptionPointSize), m_militaryCaptionBold ) )	;

						m_militarySubtitle->blockDrawn = TRUE;
						m_militarySubtitle->incrementOnFrame = currLogicFrame + (Int)(((Real)LOGICFRAMES_PER_SECOND * m_militaryCaptionDelayMS)/1000.0f);
					}
					else
					{
						// if we've exceeded the allocated number of display strings, this will force us to essentially truncate the remaining text
						m_militarySubtitle->index = m_militarySubtitle->subtitle.getLength();
						DEBUG_CRASH(("You're Only Allowed to use %d lines of subtitle text\n",MAX_SUBTITLE_LINES));
					}
				}
				else
				{
					// okay, we're not a \n, lets append this character to the display string
					m_militarySubtitle->displayStrings[m_militarySubtitle->currentDisplayString]->appendChar(tempWChar);
					// increment the draw position of the block
					Int width;
					m_militarySubtitle->displayStrings[m_militarySubtitle->currentDisplayString]->getSize(&width,NULL);
					m_militarySubtitle->blockPos.x = m_militarySubtitle->position.x + width;

					// lets make a sound
					static AudioEventRTS click("MilitarySubtitlesTyping");
					TheAudio->addAudioEvent(&click);
					if(TheGlobalLanguageData)
						m_militarySubtitle->incrementOnFrame = currLogicFrame + TheGlobalLanguageData->m_militaryCaptionSpeed;
					else
						m_militarySubtitle->incrementOnFrame = currLogicFrame + m_militaryCaptionSpeed;

				}
				// increment the index			
				m_militarySubtitle->index++;
				if(m_militarySubtitle->index >= m_militarySubtitle->subtitle.getLength())
				{
					// We're at the end of the subtitle, set everything to persist till the subtitle has expired
					m_militarySubtitle->incrementOnFrame = m_militarySubtitle->lifetime + 1;
				}
	/*
							else
								{
									// randomize the space between printing of characters
									if(GameClientRandomValueReal(0,1) < 0.95f)
									{
										m_militarySubtitle->incrementOnFrame = GameClientRandomValue(2, 5) + currLogicFrame;
									}
									else
									{
										m_militarySubtitle->incrementOnFrame = GameClientRandomValue(10, 13) + currLogicFrame;
									}
								}*/
				
			}
		}
	}

	// update the player money window if the money amount has changed
	// this seems like as good a place as any to do the power hide/show
	static Int lastMoney = -1;
	static NameKeyType moneyWindowKey = TheNameKeyGenerator->nameToKey( "ControlBar.wnd:MoneyDisplay" );	
	static NameKeyType powerWindowKey = TheNameKeyGenerator->nameToKey( "ControlBar.wnd:PowerWindow" );	

	GameWindow *moneyWin = TheWindowManager->winGetWindowFromId( NULL, moneyWindowKey );
	GameWindow *powerWin = TheWindowManager->winGetWindowFromId( NULL, powerWindowKey );
//	if( moneyWin == NULL )
//	{
//		NameKeyType moneyWindowKey = TheNameKeyGenerator->nameToKey( "ControlBar.wnd:MoneyDisplay" );	
//
//		moneyWin = TheWindowManager->winGetWindowFromId( NULL, moneyWindowKey );
//
//	}  // end if
	Player *moneyPlayer = NULL;
	if( TheControlBar->isObserverControlBarOn())
		moneyPlayer = TheControlBar->getObserverLookAtPlayer();
	else
		moneyPlayer = ThePlayerList->getLocalPlayer();
	if( moneyPlayer)
	{
		Int currentMoney = moneyPlayer->getMoney()->countMoney();
		if( lastMoney != currentMoney )
		{
			UnicodeString buffer;

			buffer.format( TheGameText->fetch( "GUI:ControlBarMoneyDisplay" ), currentMoney );
			GadgetStaticTextSetText( moneyWin, buffer );
			lastMoney = currentMoney;
			
		}  // end if
		moneyWin->winHide(FALSE);
		powerWin->winHide(FALSE);
	}
	else
	{
		moneyWin->winHide(TRUE);
		powerWin->winHide(TRUE);
	}
	
	// Update the floating Text;
	updateFloatingText();

	// update the control bar
	TheControlBar->update();

	updateIdleWorker();

	// update any random window layout that so requests
	for (std::list<WindowLayout *>::iterator it = m_windowLayouts.begin(); it != m_windowLayouts.end(); ++it)
	{
		WindowLayout *layout = *it;
		layout->runUpdate();
	}

	//Handle keyboard camera rotations
	if( m_cameraRotatingLeft && !m_cameraRotatingRight )
	{
		//Keyboard rotate left
		TheTacticalView->setAngle( TheTacticalView->getAngle() - TheGlobalData->m_keyboardCameraRotateSpeed );
	}
	if( m_cameraRotatingRight && !m_cameraRotatingLeft )
	{
		//Keyboard rotate right
		TheTacticalView->setAngle( TheTacticalView->getAngle() + TheGlobalData->m_keyboardCameraRotateSpeed );
	}
	if( m_cameraZoomingIn && !m_cameraZoomingOut )
	{
		//Keyboard zoom in
		TheTacticalView->zoomIn();
	}
	if( m_cameraZoomingOut && !m_cameraZoomingIn )
	{
		//Keyboard zoom out
		TheTacticalView->zoomOut();
	}


}  // end update

//-------------------------------------------------------------------------------------------------
void InGameUI::registerWindowLayout( WindowLayout *layout )
{
	unregisterWindowLayout(layout); // sanity
	m_windowLayouts.push_back(layout);
}

//-------------------------------------------------------------------------------------------------
void InGameUI::unregisterWindowLayout( WindowLayout *layout )
{
	for (std::list<WindowLayout *>::iterator it = m_windowLayouts.begin(); it != m_windowLayouts.end(); ++it)
	{
		if (*it == layout)
		{
			m_windowLayouts.erase(it);
			return;
		}
	}
}

//-------------------------------------------------------------------------------------------------
/** Reset the in game user interface */
//-------------------------------------------------------------------------------------------------
void InGameUI::reset( void )
{
	m_isQuitMenuVisible = FALSE;
	m_inputEnabled = true;
	// reset the command bar
	TheControlBar->reset();

	TheTacticalView->setDefaultView(0.0f, 0.0f, 1.0f);

	ResetInGameChat();

	// stop any movie currently playing
	stopMovie();

	// remove any pending GUI command
	setGUICommand( NULL );

	// remove any build available status
	placeBuildAvailable( NULL, NULL );

	// free any message resources allocated
	freeMessageResources();

	Int i;
	for (i=0; i<MAX_PLAYER_COUNT; ++i)
	{
		for (SuperweaponMap::iterator mapIt = m_superweapons[i].begin(); mapIt != m_superweapons[i].end(); ++mapIt)
		{
			for (SuperweaponList::iterator listIt = mapIt->second.begin(); listIt != mapIt->second.end(); ++listIt)
			{
				SuperweaponInfo *info = *listIt;
				info->deleteInstance();
			}
			mapIt->second.clear();
		}
		m_superweapons[i].clear();
	}

	for (NamedTimerMapIt timerIt = m_namedTimers.begin(); timerIt != m_namedTimers.end(); ++timerIt)
	{
		NamedTimerInfo *info = timerIt->second;
		TheDisplayStringManager->freeDisplayString(info->displayString);
		info->deleteInstance();
	}
	m_namedTimers.clear();
	m_namedTimerLastFlashFrame = 0;
	m_namedTimerUsedFlashColor = TRUE; // so next one is false
	m_showNamedTimers = TRUE;

	removeMilitarySubtitle();
	clearPopupMessageData();
	m_superweaponLastFlashFrame = 0;
	m_superweaponUsedFlashColor = TRUE; // so next one is false
	m_superweaponHiddenByScript = FALSE;

	clearFloatingText();
	clearWorldAnimations();
	resetIdleWorker();
	// clear hint lists
	for( i = 0; i < MAX_MOVE_HINTS; i++ )
	{

		m_moveHint[ i ].pos.zero();
		m_moveHint[ i ].sourceID = 0;
		m_moveHint[ i ].frame = 0;

	}  //  end for i

	m_waypointMode			= false;
	m_forceAttackMode		= false;
	m_forceMoveToMode		= false;
	m_attackMoveToMode	= false;
	m_preferSelection		= false;
	m_clientQuiet    = false;
	
	m_windowLayouts.clear();

	m_tooltipsDisabledUntil = 0;

	UpdateDiplomacyBriefingText(AsciiString::TheEmptyString, TRUE);
}  // end reset

//-------------------------------------------------------------------------------------------------
/** Free any resources we used for our messages */
//-------------------------------------------------------------------------------------------------
void InGameUI::freeMessageResources( void )
{
	Int i;

	// release display strings and set text to empty
	for( i = 0; i < MAX_UI_MESSAGES; i++ )
	{

		// emtpy text
		m_uiMessages[ i ].fullText.clear();

		// free display string
		if( m_uiMessages[ i ].displayString )
			TheDisplayStringManager->freeDisplayString( m_uiMessages[ i ].displayString );
		m_uiMessages[ i ].displayString = NULL;

		// set timestamp to zero
		m_uiMessages[ i ].timestamp = 0;

	}  // end for i

}  // end freeMessageResources

//-------------------------------------------------------------------------------------------------
/** Same as the unicode message method, but this takes an ascii string which is assumed
	* to me a string manager label */
//-------------------------------------------------------------------------------------------------
// srj sez: passing as const-ref screws up varargs for some reason. dunno why. just pass by value.
void InGameUI::message( AsciiString stringManagerLabel, ... )
{
	UnicodeString stringManagerString;
	UnicodeString formattedMessage;

	// fetch the string from the string manger
	stringManagerString = TheGameText->fetch( stringManagerLabel.str() );

	// construct the final text after formatting
	va_list args;
  va_start( args, stringManagerLabel );
	WideChar buf[ UnicodeString::MAX_FORMAT_BUF_LEN ];
  if( vswprintf(buf, sizeof( buf )/sizeof( WideChar ) - 1, stringManagerString.str(), args ) < 0 )
			throw ERROR_OUT_OF_MEMORY;
	formattedMessage.set( buf );
  va_end(args);

	// add the text to the ui
	addMessageText( formattedMessage );

}  // end 

//-------------------------------------------------------------------------------------------------
/** Interface for display text messages to the user */
//-------------------------------------------------------------------------------------------------
// srj sez: passing as const-ref screws up varargs for some reason. dunno why. just pass by value.
void InGameUI::message( UnicodeString format, ... )
{
	UnicodeString formattedMessage;

	// construct the final text after formatting
	va_list args;
  va_start( args, format );
	WideChar buf[ UnicodeString::MAX_FORMAT_BUF_LEN ];
  if( vswprintf(buf, sizeof( buf )/sizeof( WideChar ) - 1, format.str(), args ) < 0 )
			throw ERROR_OUT_OF_MEMORY;
	formattedMessage.set( buf );
  va_end(args);

	// add the text to the ui
	addMessageText( formattedMessage );

}  // end message

//-------------------------------------------------------------------------------------------------
/** Interface for display text messages to the user */
//-------------------------------------------------------------------------------------------------
// srj sez: passing as const-ref screws up varargs for some reason. dunno why. just pass by value.
void InGameUI::messageColor( const RGBColor *rgbColor, UnicodeString format, ... )
{
	UnicodeString formattedMessage;

	// construct the final text after formatting
	va_list args;
  va_start( args, format );
	WideChar buf[ UnicodeString::MAX_FORMAT_BUF_LEN ];
  if( vswprintf(buf, sizeof( buf )/sizeof( WideChar ) - 1, format.str(), args ) < 0 )
			throw ERROR_OUT_OF_MEMORY;
	formattedMessage.set( buf );
  va_end(args);

	// add the text to the ui
	addMessageText( formattedMessage, rgbColor );

}  // end message

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void InGameUI::addMessageText( const UnicodeString& formattedMessage, const RGBColor *rgbColor )
{
	Int i;
	Color color1 = m_messageColor1;
	Color color2 = m_messageColor2;

	if (rgbColor)
	{
		color1 = rgbColor->getAsInt() | GameMakeColor( 0, 0, 0, 255 );
		color2 = rgbColor->getAsInt() | GameMakeColor( 0, 0, 0, 255 );
	}

	// delete the message stuff at the last index
	m_uiMessages[ MAX_UI_MESSAGES - 1 ].fullText.clear();
	if( m_uiMessages[ MAX_UI_MESSAGES - 1 ].displayString )
		TheDisplayStringManager->freeDisplayString( m_uiMessages[ MAX_UI_MESSAGES - 1 ].displayString );
	m_uiMessages[ MAX_UI_MESSAGES - 1 ].displayString = NULL;
	m_uiMessages[ MAX_UI_MESSAGES - 1 ].timestamp = 0;

	// shift all the messages down one index and remove the last one
	for( i = MAX_UI_MESSAGES - 1; i >= 1; i-- )
		m_uiMessages[ i ] = m_uiMessages[ i - 1 ];

	//
	// set the new message in index 0, note that we need to allocate a display string, but
	// we do not need to free the one that is already there because it has been moved
	// "up" an index
	//
	m_uiMessages[ 0 ].fullText = formattedMessage;
	m_uiMessages[ 0 ].timestamp = TheGameLogic->getFrame();
	m_uiMessages[ 0 ].displayString = TheDisplayStringManager->newDisplayString();
	m_uiMessages[ 0 ].displayString->setFont( TheFontLibrary->getFont( m_messageFont, 
																						TheGlobalLanguageData->adjustFontSize(m_messagePointSize), m_messageBold ) );
	m_uiMessages[ 0 ].displayString->setText( m_uiMessages[ 0 ].fullText );
	
	//
	// assign a color for this string instance that will stay with it no matter what
	// line it is rendered on
	//
	if( m_uiMessages[ 1 ].displayString == NULL || m_uiMessages[ 1 ].color == color2 )
		m_uiMessages[ 0 ].color = color1;
	else
		m_uiMessages[ 0 ].color = color2;

}  // end addFormattedMessage

//-------------------------------------------------------------------------------------------------
/** Remove the message on screen at index i */
//-------------------------------------------------------------------------------------------------
void InGameUI::removeMessageAtIndex( Int i )
{

	m_uiMessages[ i ].fullText.clear();
	if( m_uiMessages[ i ].displayString )
		TheDisplayStringManager->freeDisplayString( m_uiMessages[ i ].displayString );
	m_uiMessages[ i ].displayString = NULL;
	m_uiMessages[ i ].timestamp = 0;

}  // end removeMessageAtIndex

//-------------------------------------------------------------------------------------------------
/** An area selection is occurring, start graphical "hint". */
//-------------------------------------------------------------------------------------------------
void InGameUI::beginAreaSelectHint( const GameMessage *msg )
{
	m_isDragSelecting = true;
	m_dragSelectRegion = msg->getArgument( 0 )->pixelRegion;
}

//-------------------------------------------------------------------------------------------------
/** An area selection has occurred, finish graphical "hint". */
//-------------------------------------------------------------------------------------------------
void InGameUI::endAreaSelectHint( const GameMessage *msg )
{
	m_isDragSelecting = false;
}

//-------------------------------------------------------------------------------------------------
/** A move command has occurred, start graphical "hint". */
//-------------------------------------------------------------------------------------------------
void InGameUI::createMoveHint( const GameMessage *msg )
{
	Int i;

	// first, remove any existing move hint for this source if present
	for( i = 0; i < MAX_MOVE_HINTS; i++ )
		if( m_moveHint[ i ].sourceID == msg->getArgument( 0 )->objectID &&
				m_moveHint[ i ].frame != 0 )
			expireHint( MOVE_HINT, i );

		
	if( getSelectCount() == 1 )
	{
		Drawable *draw = getFirstSelectedDrawable();
		Object *obj = draw ? draw->getObject() : NULL;
		if( obj && obj->isKindOf( KINDOF_IMMOBILE ) )
		{
			//Don't allow move hints to be created if our selected object can't move!
			return;
		}
	}

	m_moveHint[ m_nextMoveHint ].frame = TheGameClient->getFrame();
	m_moveHint[ m_nextMoveHint ].pos = msg->getArgument( 0 )->location;
		
	m_nextMoveHint++;

	// wrap around
	if (m_nextMoveHint == InGameUI::MAX_MOVE_HINTS)
		m_nextMoveHint = 0;
}

//-------------------------------------------------------------------------------------------------
/** An attack command has occurred, start graphical "hint". */
//-------------------------------------------------------------------------------------------------
void InGameUI::createAttackHint( const GameMessage *msg )
{

}

//-------------------------------------------------------------------------------------------------
/** A force attack command has occurred, start graphical "hint". */
//-------------------------------------------------------------------------------------------------
void InGameUI::createForceAttackHint( const GameMessage *msg )
{

}

//-------------------------------------------------------------------------------------------------
/** An garrison command has occurred, start graphical "hint". */
//-------------------------------------------------------------------------------------------------
void InGameUI::createGarrisonHint( const GameMessage *msg )
{
	Drawable *draw = TheGameClient->findDrawableByID( msg->getArgument(0)->drawableID );
	if( draw )
	{
		draw->onSelected();
	}
}

#if defined(_DEBUG) || defined(_INTERNAL)
#define AI_DEBUG_TOOLTIPS		1

#ifdef AI_DEBUG_TOOLTIPS
#include "Common/StateMachine.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/AIPathfind.h"
#endif // AI_DEBUG_TOOLTIPS

#endif // defined(_DEBUG) || defined(_INTERNAL)

//-------------------------------------------------------------------------------------------------
/** Details of what is mouse hovered over right now are in this message.  Terrain might result
	* in just a tooltip.  An object might get a tooltip and show its hit points.
 */
//-------------------------------------------------------------------------------------------------
void InGameUI::createMouseoverHint( const GameMessage *msg )
{
	if (m_isScrolling || m_isSelecting)
		return; // no mouseover for you

	GameWindow *window = NULL;
	const MouseIO *io = TheMouse->getMouseStatus();
	Bool underWindow = false;
	if (io && TheWindowManager)
		window = TheWindowManager->getWindowUnderCursor(io->pos.x, io->pos.y);

	while (window)
	{
		if (window->winGetInputFunc() == LeftHUDInput) {
			underWindow = false;
			break;
		}
		
		// check to see if it or any of its parents are opaque.  If so, we can't select anything.
		if (!BitTest( window->winGetStatus(), WIN_STATUS_SEE_THRU ))
		{
			underWindow = true;
			break;
		}

		window = window->winGetParent();
	}
	if (underWindow)
	{
		setMouseCursor(Mouse::ARROW); // regardless of m_mouseMode
		return;
	}

	DrawableID oldID = m_mousedOverDrawableID;

	if (msg->getType() == GameMessage::MSG_MOUSEOVER_DRAWABLE_HINT)
	{
		TheMouse->setCursorTooltip(UnicodeString::TheEmptyString );
		m_mousedOverDrawableID = INVALID_DRAWABLE_ID;
		const Drawable *draw = TheGameClient->findDrawableByID(msg->getArgument(0)->drawableID);
		const Object *obj = draw ? draw->getObject() : NULL;
		if( obj )
		{
			
 			//Ahh, here is a wierd exception: if the moused-over drawable is a mob-member
			//(e.g. AngryMob), Lets fool the UI into creating the hint for the NEXUS instead...
 			if (obj->isKindOf( KINDOF_IGNORED_IN_GUI ))
 			{
 				static NameKeyType key_MobMemberSlavedUpdate = NAMEKEY( "MobMemberSlavedUpdate" );
 				MobMemberSlavedUpdate *MMSUpdate = (MobMemberSlavedUpdate*)obj->findUpdateModule( key_MobMemberSlavedUpdate );
 				if( MMSUpdate )
 				{
 					Object *slaver = TheGameLogic->findObjectByID(MMSUpdate->getSlaverID());
 					if ( slaver )
 					{
 						Drawable *slaverDraw = slaver->getDrawable();
 						if ( slaverDraw )
 							m_mousedOverDrawableID = slaverDraw->getID();
 							// if this fails, not to worry... it has already defaulted to INVALID_DRAWABLE_ID, above
 					}
 				}
 			}
 			else
 				m_mousedOverDrawableID = draw->getID();

#if defined(_DEBUG) || defined(_INTERNAL) //Extra hacky, sorry, but I need to use this in constantdebug report
			if ( TheGlobalData->m_constantDebugUpdate == TRUE )
				m_mousedOverDrawableID = draw->getID();
#endif


			const Player* player = NULL;
			const ThingTemplate *thingTemplate = obj->getTemplate();

			ContainModuleInterface* contain = obj->getContain();
			if( contain )
				player = contain->getApparentControllingPlayer(ThePlayerList->getLocalPlayer());

			if (player == NULL)
				player = obj->getControllingPlayer();

			Bool disguised = false;
			if( obj->isKindOf( KINDOF_DISGUISER ) )
			{
				//Because we have support for disguised units pretending to be units from another
				//team, we need to intercept it here and make sure it's rendered appropriately
				//based on which client is rendering it.
				static NameKeyType key_StealthUpdate = NAMEKEY( "StealthUpdate" );
				StealthUpdate *update = (StealthUpdate*)obj->findUpdateModule( key_StealthUpdate );
				if( update )
				{
					if( update->isDisguised() )
					{
						Player *clientPlayer = ThePlayerList->getLocalPlayer();
						Player *disguisedPlayer = ThePlayerList->getNthPlayer( update->getDisguisedPlayerIndex() );
						if( player->getRelationship( clientPlayer->getDefaultTeam() ) != ALLIES && clientPlayer->isPlayerActive() )
						{
							//Neutrals and enemies will see this disguised unit as the team it's disguised as.
							player = disguisedPlayer;
							const ThingTemplate *disguisedTemplate = update->getDisguisedTemplate();
							if( disguisedTemplate )
							{
								thingTemplate = disguisedTemplate;
								disguised = true;
							}
						}
						//Otherwise, the color will show up as the team it really belongs to (already set above).
					}
				}
			}


			UnicodeString str = thingTemplate->getDisplayName();
			UnicodeString displayName = thingTemplate->getDisplayName();
			if( str.isEmpty() )
			{
				AsciiString txtTemp;
				txtTemp.format("ThingTemplate:%s", obj->getTemplate()->getName().str());
				str = TheGameText->fetch(txtTemp);
				//str.format(L"ThingTemplate:'%hs'", obj->getTemplate()->getName().str());
			}

#ifdef AI_DEBUG_TOOLTIPS
			if (TheGlobalData->m_debugAI) {
				const Team *team = obj->getTeam();
				AsciiString objName = obj->getName();
				AsciiString teamName;
				AsciiString stateName;
				
				AIUpdateInterface *ai = (AIUpdateInterface*)obj->getAI();
				if (ai) {
					if (ai->getPath()) {
						TheAI->pathfinder()->setDebugPath(ai->getPath());
					}
#ifdef STATE_MACHINE_DEBUG	
					stateName = ai->getCurrentStateName();
					if (ai->getAttackInfo()) {
						stateName.concat(" AttackPriority=");
						stateName.concat(ai->getAttackInfo()->getName());
					}
#endif
				}
				if( team )
				{
					teamName = team->getName();
				}
				if (!objName.isEmpty())
				{
					if (!teamName.isEmpty())
					{
						str.format(L"%hs(%hs): %s", teamName.str(), objName.str(), str.str());
					}
					else
					{
						str.format(L"%hs: %s", objName.str(), str.str());
					}
				}
				else
				{
					if (!teamName.isEmpty())
					{
						str.format(L"%hs: %s", teamName.str(), str.str());
					}
				}
				str.format(L"%s - %hs", str.str(), stateName.str());

			}
#endif
			UnicodeString warehouseFeedback;
			// Add on dollar amount of warehouse contents so people don't freak out until the art is hooked up
			static const NameKeyType warehouseModuleKey = TheNameKeyGenerator->nameToKey( "SupplyWarehouseDockUpdate" );
			SupplyWarehouseDockUpdate *warehouseModule = (SupplyWarehouseDockUpdate *)obj->findUpdateModule( warehouseModuleKey );
			if( warehouseModule != NULL )
			{
				Int boxes = warehouseModule->getBoxesStored();
				Int value = boxes * TheGlobalData->m_baseValuePerSupplyBox;
				warehouseFeedback.format(TheGameText->fetch("TOOLTIP:SupplyWarehouse"), value);
				str.concat(warehouseFeedback);
			}

      if (player)
			{
				UnicodeString tooltip;
				//if (TheRecorder->isMultiplayer() && player->getPlayerType() == PLAYER_HUMAN)
				if (TheRecorder->isMultiplayer() && player->isPlayableSide())
					tooltip.format(L"%s\n%s", str.str(), ((Player *)player)->getPlayerDisplayName().str());
				else
					tooltip = str;

				Int localPlayerIndex = ThePlayerList ? ThePlayerList->getLocalPlayer()->getPlayerIndex() : 0;

				Int x, y;
				ThePartitionManager->worldToCell(obj->getPosition()->x, obj->getPosition()->y, &x, &y);
				if( ThePartitionManager->getShroudStatusForPlayer(localPlayerIndex, x, y) == CELLSHROUD_CLEAR )
				{
					RGBColor rgb;
					if( disguised )
					{
						rgb.setFromInt( player->getPlayerColor() );
					}
					else
					{
						rgb.setFromInt(draw->getObject()->getIndicatorColor());

						// Unless this is a stealth garrisoned building, 
						// Let's not use the contained's housecolor
						const Object *obj = draw->getObject();
						if ( obj )
						{
							ContainModuleInterface *contain = obj->getContain();
							if ( contain && contain->isGarrisonable() )
							{
								const Player *play = contain->getApparentControllingPlayer( ThePlayerList->getLocalPlayer() );
								if ( play )
									rgb.setFromInt( play->getPlayerColor() );
							}
						}

					}

					//Object:Prop is a blank string... but we don't want to show
					//any popup box at all if that is the case!
					if( displayName.compare( TheGameText->fetch( "OBJECT:Prop" ) ) )
					{
	  				TheMouse->setCursorTooltip(tooltip, -1, &rgb );
					}
				}
			}
		}

	}
	else
	{
		m_mousedOverDrawableID = INVALID_DRAWABLE_ID;
	}

	if (oldID != m_mousedOverDrawableID)
	{
		//DEBUG_LOG(("Resetting tooltip delay\n"));
		TheMouse->resetTooltipDelay();
	}

	if (m_mouseMode == MOUSEMODE_DEFAULT && !m_isScrolling && !m_isSelecting && !TheInGameUI->getSelectCount() && (TheRecorder->getMode() != RECORDERMODETYPE_PLAYBACK || TheLookAtTranslator->hasMouseMovedRecently()))
	{
		if( m_mousedOverDrawableID != INVALID_DRAWABLE_ID )
		{
			Drawable *draw = TheGameClient->findDrawableByID(m_mousedOverDrawableID);
			
			//Add basic logic to determine if we can select a unit (or hint)
			const Object *obj = draw ? draw->getObject() : NULL;
			Bool drawSelectable = CanSelectDrawable(draw, FALSE);
			if( !obj )
			{
				drawSelectable = false;
			}

			if( drawSelectable && obj->isLocallyControlled() )
			{
				setMouseCursor(Mouse::SELECTING);
			}
			else
			{
				setMouseCursor(Mouse::ARROW);
			}
		}
		else
		{
			setMouseCursor(Mouse::ARROW);
		}
	}
	else if (m_mouseMode != MOUSEMODE_DEFAULT)
	{
		setMouseCursor((Mouse::MouseCursor)m_mouseModeCursor);
	}
}

//-------------------------------------------------------------------------------------------------
/** A command would be given if a click were to happen, so give a preview hint of what it would be.
	* Changing the mouse cursor is an example
	*/
void InGameUI::createCommandHint( const GameMessage *msg )
{
	if (m_isScrolling || m_isSelecting || TheRecorder->getMode() == RECORDERMODETYPE_PLAYBACK)
		return;

	const Drawable *draw = TheGameClient->findDrawableByID(m_mousedOverDrawableID);
	GameMessage::Type t = msg->getType();
//#ifdef DO_SHROUD_PROJECTION
	if( draw && (t == GameMessage::MSG_DO_ATTACK_OBJECT_HINT || t == GameMessage::MSG_DO_ATTACK_OBJECT_AFTER_MOVING_HINT) )
	{
		const Object* obj = draw->getObject();
		Int localPlayerIndex = ThePlayerList ? ThePlayerList->getLocalPlayer()->getPlayerIndex() : 0;
#if defined(_DEBUG) || defined(_INTERNAL)
		ObjectShroudStatus ss = (!obj || !TheGlobalData->m_shroudOn) ? OBJECTSHROUD_CLEAR : obj->getShroudedStatus(localPlayerIndex);
#else
		ObjectShroudStatus ss = (!obj) ? OBJECTSHROUD_CLEAR : obj->getShroudedStatus(localPlayerIndex);
#endif
		if (ss == OBJECTSHROUD_SHROUDED)
		{
			t = GameMessage::MSG_DO_MOVETO_HINT;	// if the object is hidden, switch to something innocuous
		}
	}
//#endif

	// set cursor to normal if there is a window under the cursor
	GameWindow *window = NULL;
	const MouseIO *io = TheMouse->getMouseStatus();
	Bool underWindow = false;
	if (io && TheWindowManager)
		window = TheWindowManager->getWindowUnderCursor(io->pos.x, io->pos.y);


	while (window)
	{
		if (window->winGetInputFunc() == LeftHUDInput) {
			underWindow = false;
			break;
		}
		
		// check to see if it or any of its parents are opaque.  If so, we can't select anything.
		if (!BitTest( window->winGetStatus(), WIN_STATUS_SEE_THRU ))
		{
			underWindow = true;
			break;
		}

		window = window->winGetParent();
	}

	//Add basic logic to determine if we can select a unit (or hint)
	const Object *obj = draw ? draw->getObject() : NULL;
	Bool drawSelectable = CanSelectDrawable(draw, FALSE);
	if( !obj )
	{
		drawSelectable = false;
	}

	// Note: These are only non-NULL if there is exactly one thing selected.
	const Drawable *srcDraw = NULL;
	const Object *srcObj = NULL;
	if (getSelectCount() == 1) {
		srcDraw = getAllSelectedDrawables()->front();
		srcObj = (srcDraw ? srcDraw->getObject() : NULL);
	}

	switch (m_mouseMode)
	{
		case MOUSEMODE_DEFAULT:
			{ 
				// This section of code only gets called when there is no specific cursor mode happening.
				if (underWindow || (srcObj && !srcObj->isLocallyControlled()))
				{
					setMouseCursor(Mouse::ARROW);
					return;
				}
				switch (t)
				{
					case GameMessage::MSG_DO_MOVETO_HINT:
					{
						if( !drawSelectable && srcObj && srcObj->isLocallyControlled() && srcObj->isKindOf(KINDOF_STRUCTURE))
							setMouseCursor( Mouse::GENERIC_INVALID );
						else if( drawSelectable && obj->isLocallyControlled() && !obj->isKindOf(KINDOF_MINE))
							setMouseCursor( Mouse::SELECTING );
						else if( TheRadar->isRadarWindow( window ) &&
										 TheRadar->isRadarForced() == FALSE &&
										 (TheRadar->isRadarHidden() || 
										 ThePlayerList->getLocalPlayer()->hasRadar() == FALSE) )
							setMouseCursor( Mouse::ARROW );
						else 
							setMouseCursor( Mouse::MOVETO );
						break;
					}
					case GameMessage::MSG_DO_ATTACKMOVETO_HINT:
						if( drawSelectable && obj->isLocallyControlled()  )
							setMouseCursor( Mouse::SELECTING );
						else
							setMouseCursor( Mouse::ATTACKMOVETO );
						break;
					case GameMessage::MSG_ADD_WAYPOINT_HINT:
						setMouseCursor( Mouse::WAYPOINT );
						break;
					case GameMessage::MSG_DO_ATTACK_OBJECT_HINT:
						setMouseCursor( Mouse::ATTACK_OBJECT );
						break;
					case GameMessage::MSG_DO_ATTACK_OBJECT_AFTER_MOVING_HINT:
						setMouseCursor( Mouse::OUTRANGE );
						break;
					case GameMessage::MSG_DO_FORCE_ATTACK_OBJECT_HINT:
						setMouseCursor( Mouse::FORCE_ATTACK_OBJECT );
						break;
					case GameMessage::MSG_DO_FORCE_ATTACK_GROUND_HINT:
						setMouseCursor( Mouse::FORCE_ATTACK_GROUND );
						break;
					case GameMessage::MSG_GET_REPAIRED_HINT:
						setMouseCursor( Mouse::GET_REPAIRED );
						break;
					case GameMessage::MSG_DOCK_HINT:
						setMouseCursor( Mouse::DOCK );
						break;
					case GameMessage::MSG_GET_HEALED_HINT:
						setMouseCursor( Mouse::GET_HEALED );
						break;
					case GameMessage::MSG_DO_REPAIR_HINT:
						setMouseCursor( Mouse::DO_REPAIR );
						break;
					case GameMessage::MSG_RESUME_CONSTRUCTION_HINT:
						setMouseCursor( Mouse::RESUME_CONSTRUCTION );
						break;				
					case GameMessage::MSG_ENTER_HINT:
						setMouseCursor( Mouse::ENTER_FRIENDLY );
						break;
					case GameMessage::MSG_CONVERT_TO_CARBOMB_HINT:
					case GameMessage::MSG_HIJACK_HINT:
						setMouseCursor( Mouse::ENTER_AGGRESSIVELY );
						break;
					case GameMessage::MSG_DEFECTOR_HINT:
						setMouseCursor( Mouse::DEFECTOR );
						break;
#ifdef ALLOW_SURRENDER
					case GameMessage::MSG_PICK_UP_PRISONER_HINT:
						setMouseCursor( Mouse::PICK_UP_PRISONER );
						break;
#endif
					case GameMessage::MSG_CAPTUREBUILDING_HINT:
						setMouseCursor( Mouse::CAPTUREBUILDING );
						break;
					case GameMessage::MSG_HACK_HINT:
						setMouseCursor( Mouse::HACK );
						break;
					case GameMessage::MSG_IMPOSSIBLE_ATTACK_HINT:
						setMouseCursor( Mouse::GENERIC_INVALID );
						break;
					case GameMessage::MSG_SET_RALLY_POINT_HINT:
						if ( !drawSelectable )
							setMouseCursor( Mouse::SET_RALLY_POINT );
						else
							setMouseCursor( Mouse::SELECTING );
						break;
					case GameMessage::MSG_DO_SPECIAL_POWER_OVERRIDE_DESTINATION_HINT:
						setMouseCursor( Mouse::PARTICLE_UPLINK_CANNON );
						break;
					case GameMessage::MSG_DO_SALVAGE_HINT:
						setMouseCursor( Mouse::MOVETO );
						break;
					case GameMessage::MSG_DO_INVALID_HINT:
						setMouseCursor( Mouse::GENERIC_INVALID );
						break;
				}
			}
			break;
		case MOUSEMODE_BUILD_PLACE:
			{
				if (underWindow)
				{
					setMouseCursor(Mouse::ARROW);
					return;
				}
				switch (t)
				{
					case GameMessage::MSG_DO_MOVETO_HINT:
					case GameMessage::MSG_DO_ATTACKMOVETO_HINT:
					case GameMessage::MSG_ADD_WAYPOINT:
						setMouseCursor(Mouse::BUILD_PLACEMENT);
						break;
					case GameMessage::MSG_DO_ATTACK_OBJECT_HINT:
					case GameMessage::MSG_DO_ATTACK_OBJECT_AFTER_MOVING_HINT:
						setMouseCursor(Mouse::INVALID_BUILD_PLACEMENT);
						break;
				}
			}
			break;
		case MOUSEMODE_GUI_COMMAND:
			{
				if (underWindow)
				{
					setMouseCursor(Mouse::ARROW);
					return;
				}
				// set the mouse cursor for commands that need a targeting or to normal with no command
				if( m_pendingGUICommand )
				{
					if( m_pendingGUICommand->isContextCommand() || 
							m_pendingGUICommand->getCommandType() == GUI_COMMAND_SPECIAL_POWER ||
							m_pendingGUICommand->getCommandType() == GUI_COMMAND_SPECIAL_POWER_FROM_COMMAND_CENTER )
					{
						//Here is the hook for when we are in a context sensitive command mode. We can
						//either do the specified command mode command or nothing! Whether or not the 
						//command is valid or not was determined in evaluateContextCommand which is
						//called first, and posts the appropriate message.
						AsciiString cursorName;	// empty by default
						switch( t )
						{
							case GameMessage::MSG_VALID_GUICOMMAND_HINT:
								cursorName = m_pendingGUICommand->getCursorName();
								break;
							case GameMessage::MSG_INVALID_GUICOMMAND_HINT:
							default:
								cursorName = m_pendingGUICommand->getInvalidCursorName();
								break;
						}

						Int index = TheMouse->getCursorIndex(cursorName);
						if( index != Mouse::INVALID_MOUSE_CURSOR )
						{
							setMouseCursor( (Mouse::MouseCursor)index );
						}
						else
						{
							setMouseCursor( Mouse::CROSS );
						}
						setRadiusCursor(m_pendingGUICommand->getRadiusCursorType(), //*****************************************************************
														m_pendingGUICommand->getSpecialPowerTemplate(),
														m_pendingGUICommand->getWeaponSlot());
					}
					else if( BitTest( m_pendingGUICommand->getOptions(), COMMAND_OPTION_NEED_TARGET ) )
					{
						Int index = TheMouse->getCursorIndex(m_pendingGUICommand->getCursorName());
						if (index != Mouse::INVALID_MOUSE_CURSOR)
							setMouseCursor( (Mouse::MouseCursor)index );
						else
							setMouseCursor( Mouse::CROSS );
						setRadiusCursor(m_pendingGUICommand->getRadiusCursorType(), //*****************************************************************
														m_pendingGUICommand->getSpecialPowerTemplate(),
														m_pendingGUICommand->getWeaponSlot());
					}
					else
					{
						setRadiusCursorNone();
					}
				}
			}
			break;
	}
}

//-------------------------------------------------------------------------------------------------
/// Get drawable ID under cursor
//-------------------------------------------------------------------------------------------------
DrawableID InGameUI::getMousedOverDrawableID( void ) const
{

	return m_mousedOverDrawableID;

}

//-------------------------------------------------------------------------------------------------
/// set right-click scroll mode
//-------------------------------------------------------------------------------------------------
void InGameUI::setScrolling( Bool isScrolling )
{
	if (m_isScrolling == isScrolling)
	{
		return;
	}

	if (isScrolling)
	{
		TheMouse->capture();
		setMouseCursor( Mouse::SCROLL );

		// break any camera locks
		TheTacticalView->setCameraLock( INVALID_ID );
		TheTacticalView->setCameraLockDrawable( NULL );
	}
	else
	{
		setMouseCursor( Mouse::ARROW );
		TheMouse->releaseCapture();
	}

	m_isScrolling = isScrolling;

}

//-------------------------------------------------------------------------------------------------
/// are we scrolling?
//-------------------------------------------------------------------------------------------------
Bool InGameUI::isScrolling( void )
{
	return m_isScrolling;
}

//-------------------------------------------------------------------------------------------------
/// set drag select mode
//-------------------------------------------------------------------------------------------------
void InGameUI::setSelecting( Bool isSelecting )
{
	if (m_isSelecting == isSelecting)
	{
		return;
	}

	//setMouseCursor( Mouse::SELECTING );
	m_isSelecting = isSelecting;
}

//-------------------------------------------------------------------------------------------------
/// are we selecting?
//-------------------------------------------------------------------------------------------------
Bool InGameUI::isSelecting( void )
{
	return m_isSelecting;
}

//-------------------------------------------------------------------------------------------------
/// get scroll amount
//-------------------------------------------------------------------------------------------------
void InGameUI::setScrollAmount( Coord2D amt )
{
	m_scrollAmt = amt;
}

//-------------------------------------------------------------------------------------------------
/// get scroll amount
//-------------------------------------------------------------------------------------------------
Coord2D InGameUI::getScrollAmount( void )
{
	return m_scrollAmt;
}

//-------------------------------------------------------------------------------------------------
/** Like the building "placement" mode, clicking on some buttons in the UI require us to
	* provide additional data by clicking on a target object/location in the world.  This
	* is where we enable that "mode" so that we can get the additional data needed for a
	* command from the user */
//-------------------------------------------------------------------------------------------------
void InGameUI::setGUICommand( const CommandButton *command )
{
	if (TheRecorder->getMode() == RECORDERMODETYPE_PLAYBACK)
		return;

	// sanity
	if( command )
	{

		if( BitTest( command->getOptions(), COMMAND_OPTION_NEED_TARGET ) == FALSE )
		{

			DEBUG_ASSERTCRASH( 0, ("setGUICommand: Command '%s' does not need additional user interaction\n",	
														command->getName().str()) );
			m_pendingGUICommand = NULL;
			m_mouseMode = MOUSEMODE_DEFAULT;
			return;

		}  // end if

		m_mouseMode = MOUSEMODE_GUI_COMMAND;

	}  // end if
	else
	{
		m_mouseMode = MOUSEMODE_DEFAULT;
	}

	// set the command
	m_pendingGUICommand = command;

	// set the mouse cursor for commands that need a targeting or to normal with no command
	if( command && BitTest( command->getOptions(), COMMAND_OPTION_NEED_TARGET ) && !command->isContextCommand() )
	{
		setMouseCursor( Mouse::ARROW );// This occurs on the mouse-up of a panel button, so make an arrow
		// the mouseoverhint code will take care of the cursor context, once the mouse leaves the panel
		// but we will set the radius cursor here, so you can see it bleeding out from beneath the panel

		setRadiusCursor(command->getRadiusCursorType(), //*****************************************************************
										command->getSpecialPowerTemplate(),
										command->getWeaponSlot());
	}
	else
	{
		if (TheMouse)
		{
			setMouseCursor( Mouse::ARROW );
		}
		setRadiusCursorNone();
	}

	m_mouseModeCursor = TheMouse->getMouseCursor();

}  // end setGUICommand

//-------------------------------------------------------------------------------------------------
/** Get the pending gui command */
//-------------------------------------------------------------------------------------------------
const CommandButton *InGameUI::getGUICommand( void ) const
{

	return m_pendingGUICommand;

} 

//-------------------------------------------------------------------------------------------------
/** Destroy any drawables we have in our placement icon array and set to NULL */
//-------------------------------------------------------------------------------------------------
void InGameUI::destroyPlacementIcons( void )
{
	Int i;

	for( i = 0; i < TheGlobalData->m_maxLineBuildObjects; ++i )
	{

		if( m_placeIcon[ i ] ) 
		{
			TheTerrainVisual->removeFactionBibDrawable(m_placeIcon[ i ]);
			TheGameClient->destroyDrawable( m_placeIcon[ i ] );
		}
		m_placeIcon[ i ] = NULL;

	}  // end for i
	TheTerrainVisual->removeAllBibs();

}  // end destroyPlacementIcons

//-------------------------------------------------------------------------------------------------
/** User has clicked on a built item that requires placement in the world.  We will 
	* record what that thing is so that the we can catch the next click in the world
	* and try to place the object there */
//-------------------------------------------------------------------------------------------------
void InGameUI::placeBuildAvailable( const ThingTemplate *build, Drawable *buildDrawable )
{

	if (build != NULL)
	{
		// if building something, no radius cursor, thankew
		setRadiusCursorNone();
	}

	//
	// if we're setting another place available, but we're somehow already in the placement
	// mode, get out of it before we start a new one
	//
	if( m_pendingPlaceType != NULL && build != NULL )
		placeBuildAvailable( NULL, NULL );

	//
	// keep a record of what we are trying to place, if we are already trying to
	// place something, it is overwritten
	//
	m_pendingPlaceType = build;
	m_pendingPlaceSourceObjectID = INVALID_ID;

	Object *sourceObject = NULL;
	if( buildDrawable )
		sourceObject = buildDrawable->getObject();
	if( sourceObject )
		m_pendingPlaceSourceObjectID = sourceObject->getID();

	//
	// hack, change our cursor to at least something different ... also note that it's
	// possible to not have the mouse yet, as some UI systems as part of initialization
	// make sure that there isn't anything valid for to "place build"
	//
	if( TheMouse )
	{

		if( build )
		{
			m_mouseMode = MOUSEMODE_BUILD_PLACE;
			m_mouseModeCursor = Mouse::CROSS;

			Drawable *draw;

			// capture the mouse for our window, windows is lame and changes it if we don't
			TheMouse->capture();

			// hack for changing cursor
			setMouseCursor( Mouse::CROSS );

			// deselect all drawables, otherwise they move to the place we click
			///@ todo when message stream order more formalized eliminate this
//			TheInGameUI->deselectAllDrawables();

			// create a drawble of what we are building to be "attached" at the cursor
			draw = TheThingFactory->newDrawable( build, DRAWABLE_STATUS_NO_STATE_PARTICLES );
			if (sourceObject)
			{
				if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
					draw->setIndicatorColor(sourceObject->getControllingPlayer()->getPlayerNightColor());
				else
					draw->setIndicatorColor(sourceObject->getControllingPlayer()->getPlayerColor());
			}
			DEBUG_ASSERTCRASH( draw, ("Unable to create icon at cursor for placement '%s'\n",
												 build->getName().str()) );

			//
			// set the initial angle of the free floating building to the property from INI
			// we have this so we can have the "cool" face the user until they click and
			// pick an actual direction for placement
			//
			Real angle = build->getPlacementViewAngle();

			// don't forget to take into account the current view angle
			// angle += TheTacticalView->getAngle();	Don't do this - makes odd angled building placements.  jba.

			// set the angle in the icon we just created
			draw->setOrientation( angle );

			// set the build icon attached to the cursor to be "see-thru"
			draw->setDrawableOpacity( placementOpacity );

			// set the "icon" in the icon array at the first index
			DEBUG_ASSERTCRASH( m_placeIcon[ 0 ] == NULL, ("placeBuildAvailable, build icon array is not empty!") );
			m_placeIcon[ 0 ] = draw;	

		}  // end if
		else
		{
			if (m_mouseMode == MOUSEMODE_BUILD_PLACE)
			{
				m_mouseMode = MOUSEMODE_DEFAULT;
				m_mouseModeCursor = Mouse::ARROW;
			}

			TheMouse->releaseCapture();
			setMouseCursor( Mouse::ARROW );
			setPlacementStart( NULL );

			// if we have a place icons destroy them
			destroyPlacementIcons();

		}  // end else

	}  // end if

}  // end placeBuildAvailable

//-------------------------------------------------------------------------------------------------
/** Return the thing we're attempting to place */
//-------------------------------------------------------------------------------------------------
const ThingTemplate *InGameUI::getPendingPlaceType( void )
{
	return m_pendingPlaceType;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const ObjectID InGameUI::getPendingPlaceSourceObjectID( void )
{

	return m_pendingPlaceSourceObjectID;

}  // end getPendingPlaceSourceObjectID

//-------------------------------------------------------------------------------------------------
/** Start the angle selection interface for selecting building angles when placing them */
//-------------------------------------------------------------------------------------------------
void InGameUI::setPlacementStart( const ICoord2D *start )
{

	// if we have a start point we turn "on" the interface, otherwise we turn it "off"
	if( start )
	{

		m_placeAnchorStart = *start;
		m_placeAnchorEnd = *start;
		m_placeAnchorInProgress = TRUE;

	}  // end if
	else
		m_placeAnchorInProgress = FALSE;

}  // end setPlacementStart

//-------------------------------------------------------------------------------------------------
/** Set the end anchor for the angle build interface */
//-------------------------------------------------------------------------------------------------
void InGameUI::setPlacementEnd( const ICoord2D *end )
{

	if( end )
		m_placeAnchorEnd = *end;

}  // end setPlacementEnd

//-------------------------------------------------------------------------------------------------
/** Is the angle selection interface for placing building at angles up? */
//-------------------------------------------------------------------------------------------------
Bool InGameUI::isPlacementAnchored( void )
{

	return m_placeAnchorInProgress;

}  // end isPlacementAnchored

//-------------------------------------------------------------------------------------------------
/** Get the start and end anchor points for the building angle selection interface */
//-------------------------------------------------------------------------------------------------
void InGameUI::getPlacementPoints( ICoord2D *start, ICoord2D *end )
{

	if( start )
		*start = m_placeAnchorStart;
	if( end )
		*end = m_placeAnchorEnd;

}  // end getPlacementPoints

//-------------------------------------------------------------------------------------------------
/** Return the angle of the drawable at the cursor if any */
//-------------------------------------------------------------------------------------------------
Real InGameUI::getPlacementAngle( void )
{

	if( m_placeIcon[ 0 ] )
		return m_placeIcon[ 0 ]->getOrientation();

	return 0.0f;

}  // end getPlacementAngle

//-------------------------------------------------------------------------------------------------
/** Mark given Drawable as "selected". */
//-------------------------------------------------------------------------------------------------
void InGameUI::selectDrawable( Drawable *draw )
{

	if( draw->isSelected() == FALSE )
	{

		m_frameSelectionChanged = TheGameLogic->getFrame();
		// set the selection in the drawable
		draw->friend_setSelected();

		// add to our selected list
		m_selectedDrawables.push_front( draw );

		// we now have one more selected drawable
		incrementSelectCount(); 


		// evaluate whether our selection consists of exactly one angry mob
		evaluateSoloNexus( draw );

		// the control needs to update its context sensitive display now
		TheControlBar->onDrawableSelected( draw );

	}  // end if

}  // end selectDrawable

//-------------------------------------------------------------------------------------------------
/** Clear "selected" status of Drawable. */
//-------------------------------------------------------------------------------------------------
void InGameUI::deselectDrawable( Drawable *draw )
{

	if( draw->isSelected() )
	{

		m_frameSelectionChanged = TheGameLogic->getFrame();
		// clear the selected bit out of the drawable
		draw->friend_clearSelected();

		// find the drawable entry in our list
		DrawableListIt findIt = std::find( m_selectedDrawables.begin(), 
																			 m_selectedDrawables.end(), 
																			 draw );

		// sanity
		DEBUG_ASSERTCRASH( findIt != m_selectedDrawables.end(),
											 ("deselectDrawable: Drawable not found in the selected drawable list '%s'\n",
											 draw->getTemplate()->getName().str()) );

		// remove it from the selected drawable list		
		m_selectedDrawables.erase( findIt );

		// keep out own internal count happy
		decrementSelectCount(); 

		// evaluate whether our selection consists of exactly one angry mob
		evaluateSoloNexus();

		// the control needs to update its context sensitive display now
		TheControlBar->onDrawableDeselected( draw );

	}  // end if

}  // end deselectDrawable

//-------------------------------------------------------------------------------------------------
/** Clear all drawables' "select" status */
//-------------------------------------------------------------------------------------------------
void InGameUI::deselectAllDrawables( Bool postMsg )
{
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();

	// loop through all the selected drawables
	for ( DrawableListCIt it = selected->begin(); it != selected->end(); )
	{

		// get drawable and increment iterator, we will invalidate it as we deselect
		Drawable* draw = *it++;

		// do the deselection
		TheInGameUI->deselectDrawable( draw );

	}  // end while

	// keep our list all tidy
	m_selectedDrawables.clear();


	// our selection can no longer consist of exactly one angry mob
	m_soloNexusSelectedDrawableID = INVALID_DRAWABLE_ID;


	///@todo don't we want to not emit this message if there wasn't a group at all? (CBD)
	/** @todo also, we probably are sending this message too much, we should come up with
	some kind of "selections are dirty" status that we can check once per frame and send
	the correct group info over the network ... could be tricky tho (or impossible) given
	the order of operations of things happening in the code (CBD) */
	if( postMsg )
	{
		GameMessage *groupMsg = TheMessageStream->appendMessage( GameMessage::MSG_DESTROY_SELECTED_GROUP );

		//True deletes entire group.
		groupMsg->appendBooleanArgument( true );
	}
}



//-------------------------------------------------------------------------------------------------
/** Return the list of all the currently selected Drawable pointers. */
//-------------------------------------------------------------------------------------------------
const DrawableList *InGameUI::getAllSelectedDrawables( void ) const
{
	return &m_selectedDrawables;
}

//-------------------------------------------------------------------------------------------------
/** Return the list of all the currently selected Drawable pointers. */
//-------------------------------------------------------------------------------------------------
const DrawableList *InGameUI::getAllSelectedLocalDrawables( void )
{
	m_selectedLocalDrawables.clear();
	for (DrawableList::const_iterator it = m_selectedDrawables.begin(); it != m_selectedDrawables.end(); ++it)
	{
		Drawable *draw = (*it);
		if (draw && draw->getObject() && draw->getObject()->isLocallyControlled())
			m_selectedLocalDrawables.push_back( draw );
	}
	return &m_selectedLocalDrawables;
}

//-------------------------------------------------------------------------------------------------
/** Return poiner to the first selected drawable, if any */
//-------------------------------------------------------------------------------------------------
Drawable *InGameUI::getFirstSelectedDrawable( void )
{

	// sanity
	if( m_selectedDrawables.empty() )
		return NULL;  // this is valid, nothing is selected

	return m_selectedDrawables.front();

}  // end getFirstSelectedDrawable

//-------------------------------------------------------------------------------------------------
/** Return true if the selected ID is in the drawable list */
//-------------------------------------------------------------------------------------------------
Bool InGameUI::isDrawableSelected( DrawableID idToCheck ) const
{

	for( DrawableListCIt it = m_selectedDrawables.begin(); it != m_selectedDrawables.end(); ++it ) 
	{

		if( (*it)->getID() == idToCheck )
			return TRUE;

	}  // end for

	return FALSE;

}  // end isDrawableSelected

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool InGameUI::isAnySelectedKindOf( KindOfType kindOf ) const
{
	Drawable *draw;

	for( DrawableListCIt it = m_selectedDrawables.begin();
			 it != m_selectedDrawables.end();
			 ++it )
	{

		/** @todo, it seems like we might want to keep a list of drawable pointers so we
		don't have to do this lookup ... it seems "tightly coupled" to me (CBD) */
		// get the drawable from the ID
		draw = *it;
		if( draw && draw->isKindOf( kindOf ) )
			return TRUE;

	}  // end for, it

	return FALSE;  // no selected objects are of the kind of type

}  // end isAnySelectedKindOf

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool InGameUI::isAllSelectedKindOf( KindOfType kindOf ) const
{
	Drawable *draw;

	for( DrawableListCIt it = m_selectedDrawables.begin();
			 it != m_selectedDrawables.end();
			 ++it )
	{

		/** @todo, it seems like we might want to keep a list of drawable pointers so we
		don't have to do this lookup ... it seems "tightly coupled" to me (CBD) */
		// get the drawable from the ID
		draw = *it;
		if( draw && draw->isKindOf( kindOf ) == FALSE )
			return FALSE;  // not all objects are of the kind of type

	}  // end for, it

	return TRUE;  // all objects have this kindof bit set in them

}  // end isAllSelectedKindOf

//-------------------------------------------------------------------------------------------------
/** Set the input enabled/disabled */
//-------------------------------------------------------------------------------------------------
void InGameUI::setInputEnabled( Bool enable )
{
	if(!enable)
		setSelecting( FALSE );
	
	Bool wasEnabled = m_inputEnabled;

	m_inputEnabled = enable;
	
	if (wasEnabled && !enable)
	{
		/*
			when input is disabled, clear out all the special "modes" we can be in, since we can miss
			the "exit mode" message during the cinematic. e.g., hold down the ctrl key when a cinematic
			begins, then release it during the cinematic... since input is disabled, we never see the keyup
			and thus think we're still in forceattack when its done, until you jiggle that key again.
			(admittedly, this code will actually do the wrong thing if you were to hold down the ctrl
			key thru the whole cinematic, but that's even more unlikely...)
		*/
		setForceAttackMode( false );			// CTRL
		setForceMoveMode( false );				// apparently unmapped in current CommandMap.ini
		setWaypointMode( false );					// ALT
		setPreferSelectionMode( false );	// SHIFT
		setCameraRotateLeft( false );			// KP4
		setCameraRotateRight( false );		// KP6
		setCameraZoomIn( false );					// KP8
		setCameraZoomOut( false );				// KP2
	}
}

//-------------------------------------------------------------------------------------------------
/** Drawable is being destroyed, clean up any UI elements associated with it. */
//-------------------------------------------------------------------------------------------------
void InGameUI::disregardDrawable( Drawable *draw )
{

	// make sure drawable is no longer selected
	deselectDrawable( draw );		

}

//-------------------------------------------------------------------------------------------------
/** This is called after the UI has been drawn. */
//-------------------------------------------------------------------------------------------------
void InGameUI::postDraw( void )
{

	// render our display strings for the messages if on
	if( m_messagesOn )
	{
		Int i, x, y;
		Color dropColor;
		UnsignedByte r, g, b, a;

		x = m_messagePosition.x;
		y = m_messagePosition.y;
		for( i = MAX_UI_MESSAGES - 1; i >= 0; i-- )
		{

			if( m_uiMessages[ i ].displayString )
			{

				// make drop color black, but use the alpha setting of the fill color specified (for fading)
				GameGetColorComponents( m_uiMessages[ i ].color, &r, &g, &b, &a );
				dropColor = GameMakeColor( 0, 0, 0, a );

				// draw the text
				m_uiMessages[ i ].displayString->draw( x, y, m_uiMessages[ i ].color, dropColor );

				// increment text spot to next location
				GameFont *font = m_uiMessages[ i ].displayString->getFont();
				y += font->height;

			}  //end if

		}  // end for i

	}  // end if

	if( m_militarySubtitle )
	{
		ICoord2D pos;
		pos.x = m_militarySubtitle->position.x;
		pos.y = m_militarySubtitle->position.y;
		Color dropColor;
		UnsignedByte r, g, b, a;
		GameGetColorComponents( m_militarySubtitle->color, &r, &g, &b, &a );
		dropColor = GameMakeColor( 0, 0, 0, a );
		for(Int i = 0; i <= m_militarySubtitle->currentDisplayString; i++)
		{
			m_militarySubtitle->displayStrings[i]->draw(pos.x,pos.y, m_militarySubtitle->color,dropColor );
			Int height;
			m_militarySubtitle->displayStrings[i]->getSize(NULL, &height);
			pos.y += height;
		}
		if( m_militarySubtitle->blockDrawn )
		{
			ICoord2D size;
			size.y = m_militarySubtitle->displayStrings[m_militarySubtitle->currentDisplayString]->getFont()->height;
			size.x = size.y * 0.8f;
			TheDisplay->drawFillRect(m_militarySubtitle->blockPos.x, m_militarySubtitle->blockPos.y, size.x, size.y, m_militarySubtitle->color);
		}

	}

	// draw superweapon timers
	if (TheGameLogic->getFrame() > 0 && !m_superweaponHiddenByScript)
	{
//	Int superweaponCount = 0;
		Int startX = (Int)(m_superweaponPosition.x * TheDisplay->getWidth());
		Int startY = (Int)(m_superweaponPosition.y * TheDisplay->getHeight());
		
		Int bottomMargin = (Int)( (Real)TheTacticalView->getHeight() * 0.82f ); 
			


		Bool marginExceeded = FALSE;

		for (Int i=0; i<MAX_PLAYER_COUNT; ++i)
		{
			if ( marginExceeded )
				break;

			Color bgColor = GameMakeColor( 0, 0, 0, 255 );
			for (SuperweaponMap::iterator mapIt = m_superweapons[i].begin(); mapIt != m_superweapons[i].end(); ++mapIt)
			{
				if ( marginExceeded )
					break;
				
				AsciiString templateName = mapIt->first;
				for (SuperweaponList::iterator listIt = mapIt->second.begin(); listIt != mapIt->second.end(); ++listIt)
				{
					if ( marginExceeded )
						break;

					SuperweaponInfo *info = *listIt;
					DEBUG_ASSERTCRASH(info, ("No superweapon info!"));
					if (info && !info->m_hiddenByScript && !info->m_hiddenByScience)
					{
						//enforce bottom margin of tactical view
						if ( startY >= bottomMargin)
						{
							UnicodeString ellipsis;
							ellipsis.format(L"...");
							info->setText( ellipsis, ellipsis );
							info->setFont( m_superweaponReadyFont, m_superweaponNormalPointSize, m_superweaponNormalBold );
							info->drawTime( startX,	startY, m_superweaponFlashColor, bgColor );
							
							marginExceeded = TRUE;
							break;
						}

						Object * owningObject = TheGameLogic->findObjectByID(info->m_id);
						if (owningObject)
						{
							
							// We don't draw our timers until we are finished with construction.
							// It is important that let the SpecialPowerUpdate is add its timer in its contructor,,
							// since the science for it could be added before construction is finished,
							// And thus the timer set to READY before the timer is first drawn, here
							if ( owningObject->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION ))
								continue;

							SpecialPowerModuleInterface *module = owningObject->getSpecialPowerModule(info->getSpecialPowerTemplate());
							if (module)
							{
								// found one - draw it
 								Bool isReady = module->isReady();
 								Int readySecs;
 
								// IsReady includes disabledness, so if you have a 0 timer disabled super, you don't want 
 								// the UnsignedInt to wrap around to hundreds of millions of seconds.
 								if( module->getReadyFrame() < TheGameLogic->getFrame() )
									readySecs = 0;
 								else 
 									readySecs = (module->getReadyFrame() - TheGameLogic->getFrame()) / LOGICFRAMES_PER_SECOND;
								// Yes, integer math.  We can't have float imprecision display 4:01 on a disabled superweapon.
 
 								// Similarly, only checking timers is not truly indicitive of readyness.
 								Bool changeBolding = (readySecs != info->m_timestamp) || (isReady != info->m_ready) || info->m_forceUpdateText;
 								if (changeBolding)
 								{
 									if (isReady)
									{
										// go bold - we're good to go
										info->setFont( m_superweaponReadyFont, m_superweaponReadyPointSize, m_superweaponReadyBold );
									}
									else
									{
										// if we were at 0, we've just fired - kill the bold
										if (info->m_timestamp == 0)
										{
											info->setFont( m_superweaponNormalFont, m_superweaponNormalPointSize, m_superweaponNormalBold );
										}
									}
									
									info->m_forceUpdateText = false;
 									info->m_ready = isReady;
									info->m_timestamp = readySecs;
									Int min = readySecs/60;
									Int sec = readySecs - min*60;
									AsciiString strIndex;
									strIndex.format("GUI:%s", templateName.str());
									UnicodeString name, time;
									name.format(L"%ls: ", TheGameText->fetch(strIndex.str()).str());
									time.format(L"%d:%2.2d", min, sec);
									info->setText(name, time);
								}

								// draw the text
 								if (isReady)
								{
									if ( m_superweaponFlashDuration != 0.0f )
									{
										if ( TheGameLogic->getFrame() >= m_superweaponLastFlashFrame + (Int)(m_superweaponFlashDuration) )
										{
											m_superweaponUsedFlashColor = !m_superweaponUsedFlashColor;
											m_superweaponLastFlashFrame = TheGameLogic->getFrame();
										}
										info->drawName( startX,
											startY, (m_superweaponUsedFlashColor)?0:m_superweaponFlashColor, bgColor );
										info->drawTime( startX,
											startY, (m_superweaponUsedFlashColor)?0:m_superweaponFlashColor, bgColor );
									}
									else
									{
										info->drawName( startX, startY, 0, bgColor );
										info->drawTime( startX, startY, 0, bgColor );
									}
								}
								else
								{
									info->drawName( startX,	startY, 0, bgColor );
									info->drawTime( startX, startY, 0, bgColor );
								}

								// increment text spot to next location
								startY += info->getHeight();

								if (info->getSpecialPowerTemplate()->isSharedNSync())
									break; // Wow, it is almost too easy!
									// This prevents redundant timers for shared powers/superweapons
									// No matter how many specialpowermodules register their timers with me,
									// I will only draw the timer of the first valid one in my list,
									// since they all have the same template, ans they all
									// use the Player::getReadyFrame() functions to stay in sync.
							}
						}
					}
				}




			}
		}
	}

	// draw named timers
	if (TheGameLogic->getFrame() > 0 && m_showNamedTimers)
	{
//		Int namedTimerCount = 0;
		Bool reverseXDir = (m_namedTimerPosition.x >= 0.5f);
		Int startX = (Int)(m_namedTimerPosition.x * TheDisplay->getWidth());
		Int startY = (Int)(m_namedTimerPosition.y * TheDisplay->getHeight());
		Color bgColor = GameMakeColor( 0, 0, 0, 255 );
		for (NamedTimerMapIt mapIt = m_namedTimers.begin(); mapIt != m_namedTimers.end(); ++mapIt)
		{
			AsciiString timerName = mapIt->first;
			NamedTimerInfo *info = mapIt->second;
			DEBUG_ASSERTCRASH(info, ("No namedTimer info!"));
			if (info)
			{
				// found one - draw it
				UnicodeString line;
				Int framesLeft = TheScriptEngine->getCounter(timerName)->value;
				UnsignedInt readyFrame = TheGameLogic->getFrame();
				if (framesLeft > 0)
					readyFrame += framesLeft;
				Int readySecs = (Int)(SECONDS_PER_LOGICFRAME_REAL * (readyFrame - TheGameLogic->getFrame()));
				if ( (info->isCountdown && readySecs != info->timestamp) || (!info->isCountdown && framesLeft != info->timestamp) )
				{
					if (!readySecs && info->isCountdown)
					{
						// go bold - we're good to go
						info->displayString->setFont( TheFontLibrary->getFont( m_namedTimerReadyFont, 
							TheGlobalLanguageData->adjustFontSize(m_namedTimerReadyPointSize), m_namedTimerReadyBold ) );
					}
					else
					{
						// if we were at 0, we've just fired - kill the bold
						if (info->timestamp == 0 || info->isCountdown)
						{
							info->displayString->setFont( TheFontLibrary->getFont( m_namedTimerNormalFont, 
								TheGlobalLanguageData->adjustFontSize(m_namedTimerNormalPointSize), m_namedTimerNormalBold ) );
						}
					}

					info->timestamp = readySecs;
					Int min = readySecs/60;
					Int sec = readySecs - min*60;
					
					if (!info->isCountdown)
						line.format(L"%s %d", info->timerText.str(), framesLeft);
					else
					{
						if (sec >= 10)
							line.format(L"%s %d:%d", info->timerText.str(), min, sec);
						else
							line.format(L"%s %d:0%d", info->timerText.str(), min, sec);
					}
					info->displayString->setText(line);
				}

				// draw the text
				Int drawX = startX;
				if (reverseXDir)
					drawX -= info->displayString->getWidth();
				if (!readySecs && info->isCountdown)
				{
					if ( m_namedTimerFlashDuration != 0.0f )
					{
						if ( TheGameLogic->getFrame() >= m_namedTimerLastFlashFrame + (Int)(m_namedTimerFlashDuration) )
						{
							m_namedTimerUsedFlashColor = !m_namedTimerUsedFlashColor;
							m_namedTimerLastFlashFrame = TheGameLogic->getFrame();
						}
						info->displayString->draw( drawX, startY, (m_namedTimerUsedFlashColor)?info->color:m_namedTimerFlashColor, bgColor );
					}
					else
					{
						info->displayString->draw( drawX, startY, info->color, bgColor );
					}
				}
				else
				{
					info->displayString->draw( drawX, startY, info->color, bgColor );
				}

				// increment text spot to next location
				startY -= info->displayString->getFont()->height;
			}
		}
	}
	
	// draw RMB scroll anchor
	if (TheLookAtTranslator && m_drawRMBScrollAnchor)
	{
		const ICoord2D* anchor = TheLookAtTranslator->getRMBScrollAnchor();
		if (anchor)
		{
			static const Int w = 2;
			static const Int h = 2;
			static const Int r = 4; // ratio
			static const Color mainColor = GameMakeColor(0, 255, 0, 255);
			static const Color dropColor = GameMakeColor(0, 0, 0, 255);
			TheDisplay->drawFillRect( anchor->x-w*r-1, anchor->y-h-1, w*2*r+3, h*2+3, dropColor );
			TheDisplay->drawFillRect( anchor->x-w-1, anchor->y-h*r-1, w*2+3, h*2*r+3, dropColor );
			TheDisplay->drawFillRect( anchor->x-w*r, anchor->y-h, w*2*r+1, h*2+1, mainColor );
			TheDisplay->drawFillRect( anchor->x-w, anchor->y-h*r, w*2+1, h*2*r+1, mainColor );
		}
	}
}  // end postDraw

//-------------------------------------------------------------------------------------------------
/** Expire a hint of the specified type with the corresponding hint index */
//-------------------------------------------------------------------------------------------------
void InGameUI::expireHint( HintType type, UnsignedInt hintIndex )
{

	if( type == MOVE_HINT )
	{

		// sanity
		if( hintIndex < 0 || hintIndex >= MAX_MOVE_HINTS )
			return;

		m_moveHint[ hintIndex ].sourceID = 0;
		m_moveHint[ hintIndex ].frame = 0;

	}  // end if
	else
	{

		// undefined hint type
		DEBUG_CRASH(("undefined hint type"));
		return;

	}  // end else

}  // end expireHint

//-------------------------------------------------------------------------------------------------
/** Create the control user interface GUI */
//-------------------------------------------------------------------------------------------------
void InGameUI::createControlBar( void )
{

	TheWindowManager->winCreateFromScript( AsciiString("ControlBar.wnd") );
	HideControlBar();
/*	
	// hide all windows created from this layout
	GameWindow *window = TheWindowManager->winGetWindowList();
	for( ; window; window = window->winGetPrev() )
		window->winHide( TRUE );
*/

}  // end createControlBar

//-------------------------------------------------------------------------------------------------
/** Create the replay control GUI */
//-------------------------------------------------------------------------------------------------
void InGameUI::createReplayControl( void )
{

	m_replayWindow = TheWindowManager->winCreateFromScript( AsciiString("ReplayControl.wnd") );

/*	
	// hide all windows created from this layout
	GameWindow *window = TheWindowManager->winGetWindowList();
	for( ; window; window = window->winGetPrev() )
		window->winHide( TRUE );
*/

}  // end createReplayControl

// ------------------------------------------------------------------------------------------------
// InGameUI::playMovie
// ------------------------------------------------------------------------------------------------
void InGameUI::playMovie( const AsciiString& movieName )
{

	stopMovie();

	m_videoStream = TheVideoPlayer->open( movieName );

	if ( m_videoStream == NULL )
	{
		return;
	}

	m_currentlyPlayingMovie = movieName;
	m_videoBuffer = TheDisplay->createVideoBuffer();

	if (	m_videoBuffer == NULL || 
				!m_videoBuffer->allocate(	m_videoStream->width(), 
													m_videoStream->height())
		)
	{
		stopMovie();
		return;
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::stopMovie( void )
{
	delete m_videoBuffer;
	m_videoBuffer = NULL;

	if ( m_videoStream )
	{
		m_videoStream->close();
		m_videoStream = NULL;
	}

	if (!m_currentlyPlayingMovie.isEmpty()) {
		//TheScriptEngine->notifyOfCompletedVideo(m_currentlyPlayingMovie); // removing sync error source -MDC
		m_currentlyPlayingMovie = AsciiString::TheEmptyString;
	}
}

// ------------------------------------------------------------------------------------------------
// InGameUI::videoBuffer
// ------------------------------------------------------------------------------------------------
VideoBuffer* InGameUI::videoBuffer( void )
{
	return m_videoBuffer;
}

// ------------------------------------------------------------------------------------------------
// InGameUI::playMovie
// ------------------------------------------------------------------------------------------------
void InGameUI::playCameoMovie( const AsciiString& movieName )
{

	stopCameoMovie();

	m_cameoVideoStream = TheVideoPlayer->open( movieName );

	if ( m_cameoVideoStream == NULL )
	{
		return;
	}

	m_cameoVideoBuffer = TheDisplay->createVideoBuffer();

	if (	m_cameoVideoBuffer == NULL || 
				!m_cameoVideoBuffer->allocate(	m_cameoVideoStream->width(), 
													m_cameoVideoStream->height())
		)
	{
		stopCameoMovie();
		return;
	}
	GameWindow *window = TheWindowManager->winGetWindowFromId(NULL,TheNameKeyGenerator->nameToKey( AsciiString("ControlBar.wnd:RightHUD") ));
	WinInstanceData *winData = window->winGetInstanceData();
	winData->setVideoBuffer(m_cameoVideoBuffer);
//	window->winHide(FALSE);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void InGameUI::stopCameoMovie( void )
{
//RightHUD
	//GameWindow *window = TheWindowManager->winGetWindowFromId(NULL,TheNameKeyGenerator->nameToKey( AsciiString("ControlBar.wnd:CameoMovieWindow") ));
	GameWindow *window = TheWindowManager->winGetWindowFromId(NULL,TheNameKeyGenerator->nameToKey( AsciiString("ControlBar.wnd:RightHUD") ));
//	window->winHide(FALSE);
	WinInstanceData *winData = window->winGetInstanceData();
	winData->setVideoBuffer(NULL);
	
	delete m_cameoVideoBuffer;
	m_cameoVideoBuffer = NULL;

	if ( m_cameoVideoStream )
	{
		m_cameoVideoStream->close();
		m_cameoVideoStream = NULL;
	}
	
}

// ------------------------------------------------------------------------------------------------
// InGameUI::videoBuffer
// ------------------------------------------------------------------------------------------------
VideoBuffer* InGameUI::cameoVideoBuffer( void )
{
	return m_cameoVideoBuffer;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void InGameUI::displayCantBuildMessage( LegalBuildCode lbc )
{

	switch( lbc )
	{

		//---------------------------------------------------------------------------------------------
		case LBC_RESTRICTED_TERRAIN:
			TheInGameUI->message( "GUI:CantBuildRestrictedTerrain" );
			break;

		//---------------------------------------------------------------------------------------------
		case LBC_NOT_FLAT_ENOUGH:
			TheInGameUI->message( "GUI:CantBuildNotFlatEnough" );
			break;

		//---------------------------------------------------------------------------------------------
		case LBC_OBJECTS_IN_THE_WAY:
			TheInGameUI->message( "GUI:CantBuildObjectsInTheWay" );
			break;

		//---------------------------------------------------------------------------------------------
		case LBC_TOO_CLOSE_TO_SUPPLIES:
			TheInGameUI->message( "GUI:CantBuildTooCloseToSupplies" );
			break;

		//---------------------------------------------------------------------------------------------
		case LBC_NO_CLEAR_PATH:
		  TheInGameUI->message( "GUI:CantBuildNoClearPath" );
			break;

		//---------------------------------------------------------------------------------------------
		case LBC_SHROUD:
			TheInGameUI->message( "GUI:CantBuildShroud" );
			break;

		//---------------------------------------------------------------------------------------------
		default:

			TheInGameUI->message( "GUI:CantBuildThere" );
			break;

	}  // end switch

}  // end displayCantBuildMessage

// ------------------------------------------------------------------------------------------------
// InGameUI::militarySubtitle
// ------------------------------------------------------------------------------------------------
void InGameUI::militarySubtitle( const AsciiString& label, Int duration )
{
	// make sure we don't already have a subtitle up there
	removeMilitarySubtitle();

	// update our history
	UpdateDiplomacyBriefingText(label, FALSE);

	UnicodeString title = TheGameText->fetch(label);

	// make sure we actually will be displaying something
	if( title.isEmpty() || duration <= 0)
	{
		DEBUG_CRASH(("Trying to create a military subtitle but either title is empty (%ls) or duration is <= 0 (%d)",title.str(), duration));
		return;
	}

	// we need some frame info to set our timings
	UnsignedInt currLogicFrame = TheGameLogic->getFrame();
	const int messageTimeout = currLogicFrame + (Int)(((Real)LOGICFRAMES_PER_SECOND * duration)/1000.0f);

	// disable tooltips until this frame, cause we don't want to collide with the military subtitles.
	TheInGameUI->disableTooltipsUntil(messageTimeout);
	
	// calculate where this screen position should be since the position being passed in is based off 8x6
	Coord2D multiplyer;
	multiplyer.x = TheDisplay->getWidth() / 800;
	multiplyer.y = TheDisplay->getHeight() / 600;

	// lets bring out the data structure!
	m_militarySubtitle = NEW MilitarySubtitleData;

	m_militarySubtitle->subtitle.set(title);
	m_militarySubtitle->blockDrawn = TRUE;
	m_militarySubtitle->blockBeginFrame = currLogicFrame;
	m_militarySubtitle->lifetime = messageTimeout;
	m_militarySubtitle->blockPos.x =  m_militarySubtitle->position.x = m_militaryCaptionPosition.x * multiplyer.x;
	m_militarySubtitle->blockPos.y =  m_militarySubtitle->position.y = m_militaryCaptionPosition.y * multiplyer.y;
	m_militarySubtitle->incrementOnFrame = currLogicFrame + (Int)(((Real)LOGICFRAMES_PER_SECOND * m_militaryCaptionDelayMS)/1000.0f);
	m_militarySubtitle->index = 0;
	for (int i = 1; i < MAX_SUBTITLE_LINES; i ++)
		m_militarySubtitle->displayStrings[i] = NULL;

	m_militarySubtitle->currentDisplayString = 0;
	m_militarySubtitle->displayStrings[0] = TheDisplayStringManager->newDisplayString();
	m_militarySubtitle->displayStrings[0]->reset();
	m_militarySubtitle->displayStrings[0]->setFont(	TheFontLibrary->getFont( m_militaryCaptionTitleFont, 
		TheGlobalLanguageData->adjustFontSize(m_militaryCaptionTitlePointSize), m_militaryCaptionTitleBold ) );
	m_militarySubtitle->color = GameMakeColor(m_militaryCaptionColor.red, m_militaryCaptionColor.green, m_militaryCaptionColor.blue, m_militaryCaptionColor.alpha);
}

// ------------------------------------------------------------------------------------------------
// InGameUI::removeMilitarySubtitle
// ------------------------------------------------------------------------------------------------
void InGameUI::removeMilitarySubtitle( void )
{
	// sanity (is there really such a thing in this world?)
	if(!m_militarySubtitle)
		return;

	TheInGameUI->clearTooltipsDisabled();

	// loop through and free up the display strings
	for(Int i = 0; i <= m_militarySubtitle->currentDisplayString; i ++)
	{
		TheDisplayStringManager->freeDisplayString(m_militarySubtitle->displayStrings[i]);
		m_militarySubtitle->displayStrings[i] = NULL;
	}

	//delete it man!
	delete m_militarySubtitle;
	m_militarySubtitle= NULL;

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool InGameUI::areSelectedObjectsControllable() const
{
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();

	// loop through all the selected drawables
	const Drawable *draw;
	for( DrawableListCIt it = selected->begin(); it != selected->end(); ++it )
	{
		// get this drawable
		draw = *it;

		// All selected objects will have the same local controller, so 
		// simply return the first one.
		return draw->getObject()->isLocallyControlled();
	}

	// Nothing selected...
	return FALSE;
}

//------------------------------------------------------------------------------
//Resets the camera to default zoom and orientation.
//------------------------------------------------------------------------------
void InGameUI::resetCamera()
{
	ViewLocation currentView;
	TheTacticalView->getLocation( &currentView ); 
	TheTacticalView->resetCamera( &currentView.getPosition(), 1 );
}

//------------------------------------------------------------------------------
//Checks to see if an object can interact with an object in a non-hostile manner. This is currently used by the selection 
//translator to determine whether to do something to an object or select it instead based on the context of what is currently
//selected.
//------------------------------------------------------------------------------
Bool InGameUI::canSelectedObjectsNonAttackInteractWithObject( const Object *objectToInteractWith, SelectionRules rule ) const
{
	for( int i = 1; i < NUM_ACTIONTYPES; i++ )
	{
		if( i != ACTIONTYPE_ATTACK_OBJECT )
		{
			if( canSelectedObjectsDoAction( (ActionType)i, objectToInteractWith, rule ) )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

CanAttackResult InGameUI::getCanSelectedObjectsAttack( ActionType action, const Object *objectToInteractWith, SelectionRules rule, Bool additionalChecking ) const
{
	// Setting a rally point doesn't require an object to interact with.
	if( (objectToInteractWith == NULL) != (action == ACTIONTYPE_SET_RALLY_POINT))
	{
		return ATTACKRESULT_NOT_POSSIBLE;
	}

	// get selected list of drawables
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();

	// set up counters for rule checking
	Int count = 0;
	CanAttackResult bestResult = ATTACKRESULT_NOT_POSSIBLE;
	CanAttackResult worstResult = ATTACKRESULT_POSSIBLE;

	// loop through all the selected drawables
	Drawable *other;
	for( DrawableListCIt it = selected->begin(); it != selected->end(); ++it )
	{
	
		// get this drawable
		other = *it;
		count++;

		switch( action )
		{
			case ACTIONTYPE_ATTACK_OBJECT:
			{
				//additionalChecking is TRUE only if force attack mode is on.
				CanAttackResult result = 	TheActionManager->getCanAttackObject( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER, 
									additionalChecking ? ATTACK_NEW_TARGET_FORCED : ATTACK_NEW_TARGET );

				if( result > bestResult )
				{
					//Best result is used for the rule: SELECTION_ANY
					bestResult = result;
				}
				if( result < worstResult )
				{
					//Worst result is used for the rule: SELECTION_ALL
					worstResult = result;
				}
				break;
			}

			case ACTIONTYPE_NONE:
			case ACTIONTYPE_GET_REPAIRED_AT:
			case ACTIONTYPE_DOCK_AT:
			case ACTIONTYPE_GET_HEALED_AT:
			case ACTIONTYPE_REPAIR_OBJECT:
			case ACTIONTYPE_RESUME_CONSTRUCTION:
			case ACTIONTYPE_COMBATDROP_INTO:
			case ACTIONTYPE_ENTER_OBJECT:
			case ACTIONTYPE_HIJACK_VEHICLE:
			case ACTIONTYPE_CONVERT_OBJECT_TO_CARBOMB:
			case ACTIONTYPE_CAPTURE_BUILDING:
			case ACTIONTYPE_DISABLE_VEHICLE_VIA_HACKING:
#ifdef ALLOW_SURRENDER
			case ACTIONTYPE_PICK_UP_PRISONER:
#endif
			case ACTIONTYPE_STEAL_CASH_VIA_HACKING:
			case ACTIONTYPE_DISABLE_BUILDING_VIA_HACKING:
			case ACTIONTYPE_MAKE_DEFECTOR:
			case ACTIONTYPE_SET_RALLY_POINT:
			default:
				DEBUG_CRASH( ("Called InGameUI::getCanSelectedObjectsAttack() with actiontype %d. Only accepts attack types! Should you be calling InGameUI::canSelectedObjectsDoAction() instead?") );
				return ATTACKRESULT_INVALID_SHOT;

		}

	}  // end for

	if( count > 0 )
	{
		if( rule == SELECTION_ANY )
		{
			return bestResult;
		}
		return worstResult;
	}

	// no can do!
	return ATTACKRESULT_NOT_POSSIBLE;
}

//------------------------------------------------------------------------------
//Wrapper function that checks a specific action.
//------------------------------------------------------------------------------
Bool InGameUI::canSelectedObjectsDoAction( ActionType action, const Object *objectToInteractWith, SelectionRules rule, Bool additionalChecking ) const
{
	// Setting a rally point doesn't require an object to interact with.
	if( (objectToInteractWith == NULL) != (action == ACTIONTYPE_SET_RALLY_POINT))
	{
		return FALSE;
	}

	// get selected list of drawables
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();

	// set up counters for rule checking
	Int count = 0;
	Int qualify = 0;

	// loop through all the selected drawables
	Drawable *other;
	for( DrawableListCIt it = selected->begin(); it != selected->end(); ++it )
	{
	
		// get this drawable
		other = *it;
		count++;
		Bool success = FALSE;

		switch( action )
		{
			case ACTIONTYPE_NONE:
				//However strange this might be, it is always possible to do "nothing"
				//although I can't think of why this would be needed...
				return TRUE;
			case ACTIONTYPE_GET_REPAIRED_AT:
				success = TheActionManager->canGetRepairedAt( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_DOCK_AT:
				success = TheActionManager->canDockAt( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_GET_HEALED_AT:
				success = TheActionManager->canGetHealedAt( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				if( success )
				{
					ContainModuleInterface *contain = objectToInteractWith->getContain();
					if( contain && contain->isHealContain() )
					{
						//This container is only used for the purposes of healing and we cannot 
						//enter it normally -- this is NOT a transport!
						success = false;
					}
				}
				break;
			case ACTIONTYPE_REPAIR_OBJECT:
			{
				ObjectID currentRepairer = objectToInteractWith->getSoleHealingBenefactor(); 
				success = ( TheActionManager->canRepairObject( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER ) 
										&& ( currentRepairer == INVALID_ID || currentRepairer == other->getObject()->getID() ) );
											// unless someone else is already healing it...
											// please note that this add'l test is left out of canRepairObject() since canRepairObject 
											// gets called from within the Dozer/WorkerAIUpdates' stateMachines as they continue the repair process.
											// This remains true.
				break;
			}
			case ACTIONTYPE_RESUME_CONSTRUCTION:
				success = TheActionManager->canResumeConstructionOf( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_COMBATDROP_INTO:
				success = TheActionManager->canEnterObject( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER, COMBATDROP_INTO );
				break;
			case ACTIONTYPE_ENTER_OBJECT:
				//additionalChecking is TRUE only if we want to check if transport is full first.
				success = TheActionManager->canEnterObject( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER, additionalChecking ? CHECK_CAPACITY : DONT_CHECK_CAPACITY );
				break;
			case ACTIONTYPE_ATTACK_OBJECT:
				DEBUG_CRASH( ("Called InGameUI::canSelectedObjectsDoAction() with ACTIONTYPE_ATTACK_OBJECT. You must use InGameUI::getCanSelectedObjectsAttack() instead.") );
				return FALSE;
			case ACTIONTYPE_HIJACK_VEHICLE:
				success = TheActionManager->canHijackVehicle( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_CONVERT_OBJECT_TO_CARBOMB:
				success = TheActionManager->canConvertObjectToCarBomb( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_CAPTURE_BUILDING:
				success = TheActionManager->canCaptureBuilding( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_DISABLE_VEHICLE_VIA_HACKING:
				success = TheActionManager->canDisableVehicleViaHacking( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
#ifdef ALLOW_SURRENDER
			case ACTIONTYPE_PICK_UP_PRISONER:
				success = TheActionManager->canPickUpPrisoner( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
#endif
			case ACTIONTYPE_STEAL_CASH_VIA_HACKING:
				success = TheActionManager->canStealCashViaHacking( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_DISABLE_BUILDING_VIA_HACKING:
				success = TheActionManager->canDisableBuildingViaHacking( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_MAKE_DEFECTOR:
				success = TheActionManager->canMakeObjectDefector( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER );
				break;
			case ACTIONTYPE_SET_RALLY_POINT:
			{
				Object *obj = other->getObject();
				if (!obj) {
					success = false;
					break;
				}
				success = (obj->isKindOf(KINDOF_AUTO_RALLYPOINT) && obj->isLocallyControlled());
				break;
			}
		}

		if( success )
		{
			if( rule == SELECTION_ANY )
			{
				return TRUE;
			}

			++qualify;
		}
	}  // end for

	//If the rule is all must qualify, do the check now and return success
	//only if all the selected units qualified.
	if( rule == SELECTION_ALL && count > 0 && qualify == count )
	{
		return TRUE;
	}

	// no can do!
	return FALSE;
}

//------------------------------------------------------------------------------
Bool InGameUI::canSelectedObjectsDoSpecialPower( const CommandButton *command, const Object *objectToInteractWith, const Coord3D *position, SelectionRules rule, UnsignedInt commandOptions, Object* ignoreSelObj ) const
{
	//Get the special power template.
	const SpecialPowerTemplate *spTemplate = command->getSpecialPowerTemplate();

	//Order of precendence:
	//1) NO TARGET OR POS
	//2) COMMAND_OPTION_NEED_OBJECT_TARGET
	//3) NEED_TARGET_POS
	Bool doAtPosition = BitTest( command->getOptions(), NEED_TARGET_POS );
	Bool doAtObject = BitTest( command->getOptions(), COMMAND_OPTION_NEED_OBJECT_TARGET );

	//Sanity checks
	if( doAtObject && !objectToInteractWith )
	{
		return false;
	}
	if( doAtPosition && !position )
	{
		return false;		
	}

	// get selected list of drawables
	Drawable* ignoreSelDraw = ignoreSelObj ? ignoreSelObj->getDrawable() : NULL;

	DrawableList tmpList;
	if (ignoreSelDraw)
		tmpList.push_back(ignoreSelDraw);

	const DrawableList* selected = (tmpList.size() > 0) ? &tmpList : TheInGameUI->getAllSelectedDrawables();

	// set up counters for rule checking
	Int count = 0;
	Int qualify = 0;

	// loop through all the selected drawables
	for( DrawableListCIt it = selected->begin(); it != selected->end(); ++it )
	{
	
		// get this drawable
		Drawable* other = *it;
		count++;

		if( !doAtObject && !doAtPosition )
		{
			if( TheActionManager->canDoSpecialPower( other->getObject(), spTemplate, CMD_FROM_PLAYER, commandOptions ) )
			{
				//This is the no target version
				if( rule == SELECTION_ANY )
				{
					return true;
				}
				qualify++;
			}
		}
		else if( doAtObject )
		{
			if( TheActionManager->canDoSpecialPowerAtObject( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER, spTemplate, commandOptions ) )
			{
				//This requires a object target
				if( rule == SELECTION_ANY )
				{
					return true;
				}
				qualify++;
			}
		}
		else if( doAtPosition )
		{
			if( TheActionManager->canDoSpecialPowerAtLocation( other->getObject(), position, CMD_FROM_PLAYER, spTemplate, objectToInteractWith, commandOptions ) )
			{
				//This requires a valid location.
				if( rule == SELECTION_ANY )
				{
					return true;
				}
				qualify++;
			}
		}
	}
	if( rule == SELECTION_ALL && count > 0 && qualify == count )
	{
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
Bool InGameUI::canSelectedObjectsOverrideSpecialPowerDestination( const Coord3D *loc, SelectionRules rule, SpecialPowerType spType ) const
{
	// set up counters for rule checking
	Int count = 0;
	Int qualify = 0;

	// get selected list of drawables
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();

	// loop through all the selected drawables
	Drawable *other;
	for( DrawableListCIt it = selected->begin(); it != selected->end(); ++it )
	{
	
		// get this drawable
		other = *it;
		count++;

		if( TheActionManager->canOverrideSpecialPowerDestination( other->getObject(), loc, spType, CMD_FROM_PLAYER ) )
		{
			if( rule == SELECTION_ANY )
			{
				return true;
			}
			qualify++;
		}
	}
	if( rule == SELECTION_ALL && count > 0 && qualify == count )
	{
		return true;
	}
	return false;
}


//------------------------------------------------------------------------------
Bool InGameUI::canSelectedObjectsEffectivelyUseWeapon( const CommandButton *command, const Object *objectToInteractWith, const Coord3D *position, SelectionRules rule ) const
{
	//Get the special power template.
	WeaponSlotType slot = command->getWeaponSlot();

	//Order of precendence:
	//1) NO TARGET OR POS
	//2) COMMAND_OPTION_NEED_OBJECT_TARGET
	//3) NEED_TARGET_POS
	Bool doAtPosition = BitTest( command->getOptions(), NEED_TARGET_POS );
	Bool doAtObject = BitTest( command->getOptions(), COMMAND_OPTION_NEED_OBJECT_TARGET );

	//Sanity checks
	if( doAtObject && !objectToInteractWith )
	{
		return false;
	}
	if( doAtPosition && !position )
	{
		return false;		
	}

	// get selected list of drawables
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();

	// set up counters for rule checking
	Int count = 0;
	Int qualify = 0;

	// loop through all the selected drawables
	Drawable *other;
	for( DrawableListCIt it = selected->begin(); it != selected->end(); ++it )
	{
	
		// get this drawable
		other = *it;
		count++;

		if( !doAtObject && !doAtPosition )
		{
			if( TheActionManager->canFireWeapon( other->getObject(), slot, CMD_FROM_PLAYER ) )
			{
				//This is the no target version
				if( rule == SELECTION_ANY )
				{
					return true;
				}
				qualify++;
			}
		}
		else if( doAtObject )
		{
			if( TheActionManager->canFireWeaponAtObject( other->getObject(), objectToInteractWith, CMD_FROM_PLAYER, slot ) )
			{
				//This requires a object target
				if( rule == SELECTION_ANY )
				{
					return true;
				}
				qualify++;
			}
		}
		else if( doAtPosition )
		{
			if( TheActionManager->canFireWeaponAtLocation( other->getObject(), position, CMD_FROM_PLAYER, slot, objectToInteractWith ) )
			{
				//This requires a valid location.
				if( rule == SELECTION_ANY )
				{
					return true;
				}
				qualify++;
			}
		}
	}
	if( rule == SELECTION_ALL && count > 0 && qualify == count )
	{
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
/** Selects maching units on the screen */
// ------------------------------------------------------------------------------------------------
Int InGameUI::selectAcrossRegion( IRegion2D *region )
{
	const DrawableList *selected = getAllSelectedDrawables();

	/* loop through all the selected drawables and create a set of all the objects,
	   so that you only iterate once through each type of object
	*/

	const Drawable *draw;

	//std::set<AsciiString> drawableList;
	std::set<const ThingTemplate*> drawableList;
	Bool carBomb = FALSE;
	
	for( DrawableListCIt it = selected->begin(); it != selected->end(); ++it )
	{
		// get this drawable
		draw = *it;
		if( draw && draw->getObject() && draw->getObject()->isLocallyControlled() )
		{
			// Use the Object's thing template, doing so will prevent wierdness for disguised vehicles.
			drawableList.insert( draw->getObject()->getTemplate() );
			if( draw->getObject()->testStatus( OBJECT_STATUS_IS_CARBOMB ) )
			{
				carBomb = TRUE;
			}
		}
	}

	if (drawableList.size() == 0)
		return -1; // nothing useful selected to begin with - don't bother iterating

	std::set<const ThingTemplate*>::iterator iter;
	const ThingTemplate *templateName;

	// now use the list to select across screen
	SelectionData data;
	Int newSelectionCount = 0;

	for( iter = drawableList.begin(); iter != drawableList.end(); ++iter )
	{
		// get this drawable
		templateName = *iter;

		data.templateToSelect = templateName;
		data.isCarBomb        = carBomb;
		if (region)
			newSelectionCount +=TheTacticalView->iterateDrawablesInRegion(region, similarUnitSelection, (void *)&data);
		else
		{
			// loop over the map
			Drawable *temp = TheGameClient->firstDrawable();
			while( temp )
			{
				newSelectionCount += similarUnitSelection( temp, (void *)&data);
				temp = temp->getNextDrawable();
			}
		}
		setDisplayedMaxWarning( FALSE );
	}

	if (newSelectionCount > 0)
	{
		// create selected message
		GameMessage *teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP_NO_SOUND );
		// not creating a new team so pass in false
		teamMsg->appendBooleanArgument( FALSE );

		//Loop through each drawable add append it's objectID to the event.
		for( DrawableListCIt it = data.newlySelectedDrawables.begin(); it != data.newlySelectedDrawables.end(); ++it )
		{
			draw = *it;
			if( draw && draw->getObject() )
			{
				teamMsg->appendObjectIDArgument( draw->getObject()->getID() );
			}
		}
	}

	return newSelectionCount;

}

// ------------------------------------------------------------------------------------------------
/** Selects maching units on the screen */
// ------------------------------------------------------------------------------------------------
Int InGameUI::selectAcrossScreen( void )
{
	/// When implementing this, obey TheInGameUI->getMaxSelectCount() if it is > 0
			
	IRegion2D region;
	ICoord2D origin;
	ICoord2D size;
 
	TheTacticalView->getOrigin( &origin.x, &origin.y );
	size.x = TheTacticalView->getWidth();
	size.y = TheTacticalView->getHeight();
 
	buildRegion( &origin, &size, &region );

	Int numSelected = selectAcrossRegion(&region);
	if (numSelected == -1)
	{
		UnicodeString message = TheGameText->fetch( "GUI:NothingSelected" );
		TheInGameUI->message( message );
	}
	else if (numSelected == 0)
	{
	}
	else
	{
		UnicodeString message = TheGameText->fetch( "GUI:SelectedAcrossScreen" );
		TheInGameUI->message( message );
	}
	return numSelected;
}

//-------------------------------------------------------------------------------------------------
/** Selects matching units across map */
//-------------------------------------------------------------------------------------------------
Int InGameUI::selectAcrossMap()
{
	/// When implementing this, obey TheInGameUI->getMaxSelectCount() if it is > 0
	Int numSelected = selectAcrossRegion(NULL);
	if (numSelected == -1)
	{
		UnicodeString message = TheGameText->fetch( "GUI:NothingSelected" );
		TheInGameUI->message( message );
	}
	else if (numSelected == 0)
	{
		Drawable *draw = TheInGameUI->getFirstSelectedDrawable();
		if( !draw || !draw->getObject() || !draw->getObject()->isKindOf( KINDOF_STRUCTURE ) )
		{
			UnicodeString message = TheGameText->fetch( "GUI:SelectedAcrossMap" );
			TheInGameUI->message( message );
		}
	}
	else
	{
		UnicodeString message = TheGameText->fetch( "GUI:SelectedAcrossMap" );
		TheInGameUI->message( message );
	}
	return numSelected;
}

//-------------------------------------------------------------------------------------------------
/** Selects matching units, either on screen or across map.  When called by pressing 'T',
    their is not a way to tell if the game is supposed to select across the screen, or
    across the map.  For mouse clicks, i.e. Alt + click or double click, we can directly call
    selectAcrossScreen or selectAcrossMap */
//-------------------------------------------------------------------------------------------------
Int InGameUI::selectMatchingUnits()
{
	/// When implementing this, obey TheInGameUI->getMaxSelectCount() if it is > 0
	Int numSelected = selectAcrossScreen();
	if (numSelected == -1)
		return numSelected;
	if (numSelected == 0)
	{
		Int numSelectedAcrossMap = selectAcrossMap();
		//if (numSelectedAcrossMap < 1)
		//{
			//UnicodeString message = TheGameText->fetch( "GUI:NothingSelected" );
			//TheInGameUI->message( message );
		//}
		return numSelectedAcrossMap;
	}
	return numSelected;

	/*
	/// When implementing this, obey TheInGameUI->getMaxSelectCount() if it is > 0
			
	// check to see if you select units across screen or across map
			
	IRegion2D region;
	ICoord2D origin;
	ICoord2D size;
 
	TheTacticalView->getOrigin( &origin.x, &origin.y );
	size.x = TheTacticalView->getWidth();
	size.y = TheTacticalView->getHeight();
 
	// setup the region and to the iterate function

	buildRegion( &origin, &size, &region );

	const DrawableList *selected = getAllSelectedDrawables();

	Drawable *draw;

	std::set<const ThingTemplate*> drawableList;
	
	// get a set of the selected types of object
	for( DrawableListCIt it = selected->begin(); it != selected->end(); ++it )
	{
		// get this drawable
		draw = *it;
		if( draw && draw->getObject() && draw->getObject()->isLocallyControlled() )
		{
			

			//drawableList.insert( draw->getObject()->getTemplate()->getName() );
			drawableList.insert( draw->getTemplate() );
		}
	}

	std::set<const ThingTemplate*>::const_iterator iter;
	
	const ThingTemplate *templateName;
	TypeSelectionData data;
	data.m_message = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP );

	// go though the drawableList and get the units of that type
	for( iter = drawableList.begin(); iter != drawableList.end(); ++iter )
	{
		// get this drawable
		data.m_template = *iter;
		//iterate through the drawable region
		// all drawables in the region will call the typeSelection method
		m_selectedAcrossScreen = TheTacticalView->iterateDrawablesInRegion( &region, InGameUI::typeSelection, (void *) &data);
		setDisplayedMaxWarning( FALSE );
	}

	if( m_selectedAcrossScreen )
	{
			UnicodeString message = TheGameText->fetch( "GUI:SelectedAcrossScreen" );
			TheInGameUI->message( message );
			setSelectedAcrossScreen( false );
	}
	else
	{
		// add to existing group
		GameMessage *teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP );

		// adding to previous group so pass false
		teamMsg->appendBooleanArgument( FALSE );

		selected = getAllSelectedDrawables();
		// loop through all the selected drawables
		Drawable *draw;

		//see if player has any units selected, if not, give message
		Bool check = FALSE;
		for( DrawableListCIt it1 = selected->begin(); it1 != selected->end(); ++it1 )
		{
			draw = *it1;
			if( draw && draw->getObject()->isLocallyControlled() )
			{
				check = TRUE;
			}
		}
		if( check == FALSE )
		{
			UnicodeString message = TheGameText->fetch( "GUI:NothingSelected" );
			TheInGameUI->message( message );
			setSelectedAcrossScreen( false );
			deselectAllDrawables();
			return;
		}

		//else select across the map

		for( iter = drawableList.begin(); iter != drawableList.end(); ++iter )
		{
			// get this drawable
			templateName = *iter;
			Drawable *temp = TheGameClient->firstDrawable();
			while( temp )
			{
				const Object *object = temp->getObject();

				if( object && object->isLocallyControlled()
						&& !object->isContained() && temp->getTemplate()->isEquivalentTo( templateName ) )
				{

					// enforce optional unit cap
					if ( getMaxSelectCount() > 0 && TheInGameUI->getSelectCount() >= getMaxSelectCount())
					{
						if ( !getDisplayedMaxWarning() )
						{
							setDisplayedMaxWarning( TRUE );
							UnicodeString msg;
							msg.format(TheGameText->fetch("GUI:MaxSelectionSize").str(), TheInGameUI->getMaxSelectCount());
							message(msg);
						}
					}
					else
					{
						selectDrawable( temp );
						teamMsg->appendObjectIDArgument( temp->getObject()->getID() );
						setDisplayedMaxWarning( FALSE );
					}
				}
				temp = temp->getNextDrawable();
			}
		}

		UnicodeString message = TheGameText->fetch( "GUI:SelectedAcrossMap" );
		TheInGameUI->message( message );
		setSelectedAcrossScreen( FALSE );

	}
	*/
}

//-----------------------------------------------------------------------------
/**
 * Given an "anchor" point and the current mouse position (dest),
 * construct a valid 2D bounding region.
 */
//-----------------------------------------------------------------------------------
void InGameUI::buildRegion( const ICoord2D *anchor, const ICoord2D *dest, IRegion2D *region )
{
	// build rectangular region defined by the drag selection
	if (anchor->x < dest->x)
	{
		region->lo.x = anchor->x;
		region->hi.x = dest->x;
	}
	else
	{
		region->lo.x = dest->x;
		region->hi.x = anchor->x;
	}

	if (anchor->y < dest->y)
	{
		region->lo.y = anchor->y;
		region->hi.y = dest->y;
	}
	else
	{
		region->lo.y = dest->y;
		region->hi.y = anchor->y;
	}
}

//-------------------------------------------------------------------------------------------------
/** Add a new floating text to our list */
//-------------------------------------------------------------------------------------------------
void InGameUI::addFloatingText(const UnicodeString& text,const Coord3D *pos, Color color)
{
	if( TheGameLogic->getDrawIconUI() )
	{
		FloatingTextData *newFTD = newInstance( FloatingTextData );
		newFTD->m_frameCount = 0;
		newFTD->m_color = color;
		newFTD->m_pos3D.x = pos->x;
		newFTD->m_pos3D.z = pos->z;
		newFTD->m_pos3D.y = pos->y;
		newFTD->m_text = text;
		newFTD->m_dString->setText(text);
		
			
		if(m_floatingTextTimeOut <= 0)
			newFTD->m_frameTimeOut = TheGameLogic->getFrame() +  DEFAULT_FLOATING_TEXT_TIMEOUT;
		else
			newFTD->m_frameTimeOut = TheGameLogic->getFrame() +  m_floatingTextTimeOut; 
		
		m_floatingTextList.push_front( newFTD ); // add to the list
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) || defined(_INTERNAL)
inline Bool isClose(Real a, Real b) { return fabs(a-b) <= 1.0f; }
inline Bool isClose(const Coord3D& a, const Coord3D& b) 
{
		return	isClose(a.x, b.x) && 
			isClose(a.y, b.y) && 
			isClose(a.z, b.z);
}
void InGameUI::DEBUG_addFloatingText(const AsciiString& text, const Coord3D * pos, Color color)
{
	const Int POINTSIZE = 8;
	const Int LEADING = 0;

	Coord3D posToUse = *pos;

try_again:
	for (FloatingTextListIt it = m_floatingTextList.begin(); it != m_floatingTextList.end(); ++it)
	{
		if (isClose((*it)->m_pos3D, posToUse))
		{
			posToUse.z -= (POINTSIZE + LEADING);
			goto try_again;
		}
	}

	FloatingTextData *newFTD = newInstance( FloatingTextData );
	newFTD->m_color = color;
	newFTD->m_pos3D.x = posToUse.x;
	newFTD->m_pos3D.y = posToUse.y;
	newFTD->m_pos3D.z = posToUse.z;
	UnicodeString translate;
	translate.translate(text);
	newFTD->m_text = translate;
	newFTD->m_dString->setText(translate);
	newFTD->m_dString->setFont(TheWindowManager->winFindFont( AsciiString("Arial"), POINTSIZE, FALSE ));
				
	if(m_floatingTextTimeOut <= 0)
		newFTD->m_frameTimeOut = TheGameLogic->getFrame() +  DEFAULT_FLOATING_TEXT_TIMEOUT;
	else
		newFTD->m_frameTimeOut = TheGameLogic->getFrame() +  m_floatingTextTimeOut; 
	
	m_floatingTextList.push_front( newFTD ); // add to the list

	//DEBUG_LOG(("%s\n",text.str()));
}
#endif

//-------------------------------------------------------------------------------------------------
/** modify the position of our floating text */
//-------------------------------------------------------------------------------------------------
void InGameUI::updateFloatingText( void )
{
	FloatingTextData *ftd;		// pointer to our floating point data
	UnsignedInt currLogicFrame = TheGameLogic->getFrame();			// the current logic frame
	UnsignedByte r, g, b, a;	// we'll need to break apart our color so we can modify the alpha
	Int amount;								// The amout we'll change the alpha
	static UnsignedInt lastLogicFrameUpdate = currLogicFrame;		// We need to make sure our current frame is different then our last frame we updated.

	// only update the position if we're incrementing frames
	if(lastLogicFrameUpdate == currLogicFrame)
		return;
	
	lastLogicFrameUpdate = currLogicFrame;

	// Loop through our floating text list
	for(FloatingTextListIt it = m_floatingTextList.begin(); it != m_floatingTextList.end();)
	{
		ftd = *it;
		
		// move it up
		++ftd->m_frameCount;
		
		// fade the text
		if( currLogicFrame > ftd->m_frameTimeOut)
		{
			// modify the color
			GameGetColorComponents(ftd->m_color, &r, &g, &b, &a);		
			amount = REAL_TO_INT( (currLogicFrame - ftd->m_frameTimeOut) * m_floatingTextMoveVanishRate);
			if(a - amount < 0)
				a = 0;
			else
				a -= amount;
			ftd->m_color = GameMakeColor(r, g, b, a);
			// if we have 0 alpha delete it
			if( a <= 0)
			{
				it = m_floatingTextList.erase(it);
				ftd->deleteInstance();
				continue; // don't do the ++it below
			}

		}
		// increase our itterator
		++it;
	
	}

}

//-------------------------------------------------------------------------------------------------
/** Itterates through and draws each floating text */
//-------------------------------------------------------------------------------------------------
void InGameUI::drawFloatingText( void )
{
	FloatingTextData *ftd;
	// loop through and draw all the texts
	for(FloatingTextListIt it = m_floatingTextList.begin(); it != m_floatingTextList.end(); ++it)
	{
		ftd = *it;
		ICoord2D pos;
		// get the local player's index
		Int playerNdx = ThePlayerList->getLocalPlayer()->getPlayerIndex();

		// which PartitionManager cells are we looking at?
		Int pCX, pCY;
		ThePartitionManager->worldToCell(ftd->m_pos3D.x, ftd->m_pos3D.y, &pCX, &pCY);

		// translate it's 3d pos into a 2d screen pos
		if( TheTacticalView->worldToScreen(&ftd->m_pos3D, &pos) 
			&& ftd->m_dString 
			&& ThePartitionManager->getShroudStatusForPlayer(playerNdx, pCX, pCY) == CELLSHROUD_CLEAR )
		{
			pos.y -= ftd->m_frameCount * m_floatingTextMoveUpSpeed;
			Color dropColor;
			UnsignedByte r, g, b, a;
			Int width;

			// make drop color black, but use the alpha setting of the fill color specified (for fading)
			GameGetColorComponents( ftd->m_color, &r, &g, &b, &a );
			dropColor = GameMakeColor( 0, 0, 0, a );
			ftd->m_dString->getSize(&width, NULL);
			// draw it!
			ftd->m_dString->draw(pos.x - (width / 2), pos.y, ftd->m_color,dropColor);
		}

	}
}

//-------------------------------------------------------------------------------------------------
/** ittereate through and clear out the list of floating text */
//-------------------------------------------------------------------------------------------------
void InGameUI::clearFloatingText( void )
{
	FloatingTextData *ftd;
	// loop through and draw all the texts
	for(FloatingTextListIt it = m_floatingTextList.begin(); it != m_floatingTextList.end();)
	{
		ftd = *it;
		it = m_floatingTextList.erase(it);
		ftd->deleteInstance();
	}
	
}

//-------------------------------------------------------------------------------------------------
/** If we want to use the default text color, then we call this function */
//-------------------------------------------------------------------------------------------------
void InGameUI::popupMessage( const AsciiString& message, Int x, Int y, Int width, Bool pause, Bool pauseMusic)
{
	popupMessage( message, x, y, width, m_popupMessageColor, pause, pauseMusic);
}

//-------------------------------------------------------------------------------------------------
/** initialize, and popup a message box to the user */
//-------------------------------------------------------------------------------------------------
void InGameUI::popupMessage( const AsciiString& identifier, Int x, Int y, Int width, Color textColor, Bool pause, Bool pauseMusic)
{
	if(m_popupMessageData)
		clearPopupMessageData();

	UpdateDiplomacyBriefingText(identifier, FALSE);

	UnicodeString message = TheGameText->fetch(identifier);

	m_popupMessageData = newInstance( PopupMessageData );	
	m_popupMessageData->message = message;
	// x and why are passed in as a percentage of the screen, convert to screen coords
	if( x > 100 )
		x = 100;
	if( x < 0 )
		x = 0;
	
	if( y > 100 )
		y = 100;
	if( y < 0 )
		y = 0;

	m_popupMessageData->x = TheDisplay->getWidth() * (INT_TO_REAL(x) / 100);
	m_popupMessageData->y = TheDisplay->getHeight() * (INT_TO_REAL(y) / 100);
	// cap the lower limit of the width
	if(width < 50)
		width = 50;
	m_popupMessageData->width = width;
	m_popupMessageData->textColor = textColor;
	m_popupMessageData->pause = pause;
	m_popupMessageData->pauseMusic = pauseMusic;

	if( pause )
		TheGameLogic->setGamePaused(TRUE, pauseMusic);

	m_popupMessageData->layout = TheWindowManager->winCreateLayout(AsciiString("InGamePopupMessage.wnd"));
	m_popupMessageData->layout->runInit();
}

//-------------------------------------------------------------------------------------------------
/** take care of the logic of clearing the popupMessageData */
//-------------------------------------------------------------------------------------------------
void InGameUI::clearPopupMessageData( void )
{
	if(!m_popupMessageData)
		return;
	if(m_popupMessageData->layout)
	{
		m_popupMessageData->layout->destroyWindows();
		m_popupMessageData->layout->deleteInstance();
		m_popupMessageData->layout = NULL;
	}
	if( m_popupMessageData->pause )
		TheGameLogic->setGamePaused(FALSE, m_popupMessageData->pauseMusic);
	m_popupMessageData->deleteInstance();
	m_popupMessageData = NULL;
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
/** Floating Text Constructor */
//-------------------------------------------------------------------------------------------------
FloatingTextData::FloatingTextData(void)
{
	// Added By Sadullah Nader
	// Initializations missing and needed
	m_color = 0;
	m_frameCount = 0;
	m_frameTimeOut = 0;
	m_pos3D.zero();
	m_text.clear();
	//
	m_dString = TheDisplayStringManager->newDisplayString();
}

//-------------------------------------------------------------------------------------------------
/** Floating Text Destructor */
//-------------------------------------------------------------------------------------------------
FloatingTextData::~FloatingTextData(void)
{
	if(m_dString)
		TheDisplayStringManager->freeDisplayString( m_dString );
	m_dString = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// WORLD ANIMATION DATA ///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
WorldAnimationData::WorldAnimationData( void )
{

	m_anim = NULL;
	m_worldPos.zero();
	m_expireFrame = 0;
	m_options = WORLD_ANIM_NO_OPTIONS;
	m_zRisePerSecond = 0.0f;

}  // end WorldAnimationData

// ------------------------------------------------------------------------------------------------
/** Add a 2D animation at a spot in the world */
// ------------------------------------------------------------------------------------------------
void InGameUI::addWorldAnimation( Anim2DTemplate *animTemplate,
																	const Coord3D *pos,
																	WorldAnimationOptions options,
																	Real durationInSeconds,
																	Real zRisePerSecond )
{

	// sanity
	if( animTemplate == NULL || pos == NULL || durationInSeconds <= 0.0f )
		return;

	// allocate a new world animation data struct
	// (huh huh, he said "wad")
	WorldAnimationData *wad = NEW WorldAnimationData;
	if( wad == NULL )
		return;		

	// allocate a new animation instance
	Anim2D *anim = newInstance(Anim2D)( animTemplate, TheAnim2DCollection );

	// assign all data
	wad->m_anim = anim;
	wad->m_expireFrame = TheGameLogic->getFrame() + (durationInSeconds * LOGICFRAMES_PER_SECOND);
	wad->m_options = options;
	wad->m_worldPos = *pos;
	wad->m_zRisePerSecond = zRisePerSecond;

	// add to list
	m_worldAnimationList.push_front( wad );

}  // end addWorldAnimation

// ------------------------------------------------------------------------------------------------
/** Delete all world animations */
// ------------------------------------------------------------------------------------------------
void InGameUI::clearWorldAnimations( void )
{
	WorldAnimationData *wad;

	// iterate through all entries and delete the animation data
	for( WorldAnimationListIterator it = m_worldAnimationList.begin();	
			 it != m_worldAnimationList.end(); /*empty*/ )
	{

		wad = *it;
		if( wad )
		{

			// delete the animation instance
			wad->m_anim->deleteInstance();

			// delete the world animation data
			delete wad;

		}  // end if

		it = m_worldAnimationList.erase( it );

	}  // end for
	
}  // end clearWorldAnimations

static const UnsignedInt FRAMES_BEFORE_EXPIRE_TO_FADE = LOGICFRAMES_PER_SECOND * 1;
// ------------------------------------------------------------------------------------------------
/** Update all world animations and draw the visible ones */
// ------------------------------------------------------------------------------------------------
void InGameUI::updateAndDrawWorldAnimations( void )
{
	WorldAnimationData *wad;

	// go through all animations
	for( WorldAnimationListIterator it = m_worldAnimationList.begin();
			 it != m_worldAnimationList.end(); /*empty*/ )
	{

		// get data
		wad = *it;

		// update portion ... only when the game is in motion
		if( TheGameLogic->isGamePaused() == FALSE )
		{

			//
			// see if it's time to expire this animation based on animation type and options or
			// the expire frame
			//
			if( TheGameLogic->getFrame() >= wad->m_expireFrame ||
					(BitTest( wad->m_options, WORLD_ANIM_PLAY_ONCE_AND_DESTROY ) &&
					 BitTest( wad->m_anim->getStatus(), ANIM_2D_STATUS_COMPLETE )) )
			{

				// delete this element and continue
				wad->m_anim->deleteInstance();
				delete wad;
				it = m_worldAnimationList.erase( it );
				continue;

			}  // end if

			// update the Z value
			if( wad->m_zRisePerSecond )
				wad->m_worldPos.z += wad->m_zRisePerSecond / LOGICFRAMES_PER_SECOND;

		}  // end if

		//
		// don't bother going forward with the draw process if this location is shrouded for
		// the local player
		//
		Int playerIndex = ThePlayerList->getLocalPlayer()->getPlayerIndex();
		if( ThePartitionManager->getShroudStatusForPlayer( playerIndex, &wad->m_worldPos ) != CELLSHROUD_CLEAR )
		{

			++it;
			continue;

		}  // end if

		// update translucency value
		if( BitTest( wad->m_options, WORLD_ANIM_FADE_ON_EXPIRE ) )
		{

			// see if we should be setting the translucency value
			UnsignedInt framesTillExpire = wad->m_expireFrame - TheGameLogic->getFrame();
			if( framesTillExpire < FRAMES_BEFORE_EXPIRE_TO_FADE )
			{
				
				// compute alpha level so that we're totally gone by the expire frame
				Real alpha = INT_TO_REAL( framesTillExpire ) / INT_TO_REAL( FRAMES_BEFORE_EXPIRE_TO_FADE );
				wad->m_anim->setAlpha( alpha );
				
			}  // end if

		}  // end if

		// project the point to screen space
		ICoord2D screen;
		if( TheTacticalView->worldToScreen( &wad->m_worldPos, &screen ) == TRUE )
		{
			UnsignedInt width = wad->m_anim->getCurrentFrameWidth();
			UnsignedInt height = wad->m_anim->getCurrentFrameHeight();

			// scale the width and height given the camera zoom level
			Real zoomScale = TheTacticalView->getMaxZoom() / TheTacticalView->getZoom();
			width *= zoomScale;
			height *= zoomScale;

			// adjust the screen position to draw so the image is centered at the location
			screen.x -= width / 2;
			screen.y -= height / 2;

			// draw the animation
			wad->m_anim->draw( screen.x, screen.y, width, height );

		}  // end if

		// go to the next element in the list
		++it;

	}  // end for

}  // end updateAndDrawWorldAnimations


Object *InGameUI::findIdleWorker( Object *obj)
{
	if(!obj)
		return NULL;
	
	Int index = obj->getControllingPlayer()->getPlayerIndex();	
	if(m_idleWorkers[index].empty())
		return NULL;

	ObjectListIt it = m_idleWorkers[index].begin();
	while(it != m_idleWorkers[index].end())
	{
		Object *itObj = *it;
		if(itObj == obj)
		{
			return itObj;
			break;
		}
		++it;
	}
	return NULL;
}

void InGameUI::addIdleWorker( Object *obj )
{
	if(!obj)
		return;

	if(findIdleWorker(obj))
		return;

	Int index = obj->getControllingPlayer()->getPlayerIndex();
	m_idleWorkers[index].push_back(obj);
}

void InGameUI::removeIdleWorker( Object *obj, Int playerNumber )
{
	if(!obj)
		return;
	if(playerNumber < 0 || playerNumber >= MAX_PLAYER_COUNT)  // we're leaving the game, so this is all screwed
		return;
	
	if(m_idleWorkers[playerNumber].empty())
		return;

	
	ObjectListIt it = m_idleWorkers[playerNumber].begin();
	while(it != m_idleWorkers[playerNumber].end())
	{
		Object *itObj = *it;
		if(itObj == obj)
		{
			m_idleWorkers[playerNumber].erase(it);
			return;
		}
		++it;
	}
	return;
}

void InGameUI::selectNextIdleWorker( void )
{
	Int index = ThePlayerList->getLocalPlayer()->getPlayerIndex();
	if(m_idleWorkers[index].empty())
	{
		DEBUG_ASSERTCRASH(FALSE, ("InGameUI::selectNextIdleWorker We're trying to select a worker when our list is empty for player %ls", ThePlayerList->getLocalPlayer()->getPlayerDisplayName().str()));
		return;
	}
	Object *selectThisObject = NULL;
	
	if(getSelectCount() == 0 || getSelectCount() > 1)
	{
		selectThisObject = *m_idleWorkers[index].begin();
	}
	else
	{
		Drawable *selectedDrawable = TheInGameUI->getFirstSelectedDrawable();	
		
		ObjectListIt it = m_idleWorkers[index].begin();
		while(it != m_idleWorkers[index].end())
		{
			Object *itObj = *it;
			if(itObj == selectedDrawable->getObject())
			{
				++it;
				if(it != m_idleWorkers[index].end())
					selectThisObject = *it;
				else
					selectThisObject = *m_idleWorkers[index].begin();
				break;
			}
			++it;
		}
		// if we had something selected that wasn't a worker, we'll get here
		if(!selectThisObject)
			selectThisObject = *m_idleWorkers[index].begin();

	}
	DEBUG_ASSERTCRASH(selectThisObject, ("InGameUI::selectNextIdleWorker Could not select the next IDLE worker"));
	if(selectThisObject)
	{	
		
		//If our idle worker is contained by anything, we need to select the container instead.
		Object *containedBy = selectThisObject->getContainedBy();
		if( containedBy )
		{
			selectThisObject = containedBy;
		}

		deselectAllDrawables();
		GameMessage *teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP );


		//New group or add to group? Passed in value is true if we are creating a new group.
		teamMsg->appendBooleanArgument( TRUE );

		teamMsg->appendObjectIDArgument( selectThisObject->getID() );
		
		selectDrawable( selectThisObject->getDrawable() );

		/*// removed becuase we're already playing a select sound... left in, just in case i"m wrong.
		// play the units sound
				const AudioEventRTS *soundEvent = selectThisObject->getTemplate()->getVoiceSelect();
				if (soundEvent)
				{
					TheAudio->addAudioEvent( soundEvent );
				}*/
		
		// center on the unit
		TheTacticalView->lookAt(selectThisObject->getPosition());
	}
}

Int InGameUI::getIdleWorkerCount( void )
{
	Int index = ThePlayerList->getLocalPlayer()->getPlayerIndex();
	return m_idleWorkers[index].size();
}

void InGameUI::showIdleWorkerLayout( void )
{
	if (!m_idleWorkerWin)
	{
		m_idleWorkerWin = TheWindowManager->winGetWindowFromId(NULL, TheNameKeyGenerator->nameToKey("ControlBar.wnd:ButtonIdleWorker"));
		DEBUG_ASSERTCRASH(m_idleWorkerWin, ("InGameUI::showIdleWorkerLayout could not find IdleWorker.wnd to load "));
		return;
	}

	m_idleWorkerWin->winEnable(TRUE);

	m_currentIdleWorkerDisplay = getIdleWorkerCount();
	
//	if(m_currentIdleWorkerDisplay < 1)
//		GadgetButtonSetText(m_idleWorkerWin, UnicodeString::TheEmptyString);
//	else
//	{
//		UnicodeString number;
//		number.format(L"%d",m_currentIdleWorkerDisplay);
//		GadgetButtonSetText(m_idleWorkerWin, number);
//	}
}
void InGameUI::hideIdleWorkerLayout( void )
{
	if(!m_idleWorkerWin)
		return;
	GadgetButtonSetText(m_idleWorkerWin, UnicodeString::TheEmptyString);
	m_idleWorkerWin->winEnable(FALSE);
	m_currentIdleWorkerDisplay = -1;
}

void InGameUI::updateIdleWorker( void )
{
	Int idleCount = getIdleWorkerCount();

	if(idleCount > 0 && m_currentIdleWorkerDisplay != idleCount && getInputEnabled())
		showIdleWorkerLayout();

	if((idleCount <= 0 && m_idleWorkerWin) || !getInputEnabled())
		hideIdleWorkerLayout();
}

void InGameUI::resetIdleWorker( void )
{
	if(m_idleWorkerWin)
	{
		GadgetButtonSetText(m_idleWorkerWin, UnicodeString::TheEmptyString);
	}
	m_currentIdleWorkerDisplay = -1;
	for(Int i = 0; i < MAX_PLAYER_COUNT; ++i)
	{
		m_idleWorkers[i].clear();
	}

}

void InGameUI::recreateControlBar( void )
{
	GameWindow *win = TheWindowManager->winGetWindowFromId(NULL, TheNameKeyGenerator->nameToKey(AsciiString("ControlBar.wnd")));
	if(win)
		win->deleteInstance();
	
	m_idleWorkerWin = NULL;	
	
	createControlBar();
		
	if(TheControlBar)
	{
		delete TheControlBar;
		TheControlBar = NEW ControlBar;
		TheControlBar->init();
	}


}

void InGameUI::disableTooltipsUntil(UnsignedInt frameNum)
{
	if (frameNum > m_tooltipsDisabledUntil) 
		m_tooltipsDisabledUntil = frameNum;
}

void InGameUI::clearTooltipsDisabled()
{
	m_tooltipsDisabledUntil = 0;
}

Bool InGameUI::areTooltipsDisabled() const
{
	return (TheGameLogic->getFrame() < m_tooltipsDisabledUntil);
}


WindowMsgHandledType IdleWorkerSystem( GameWindow *window, UnsignedInt msg, 
																				WindowMsgData mData1, WindowMsgData mData2 )
{
	switch( msg ) 
	{
		//---------------------------------------------------------------------------------------------
		case GWM_INPUT_FOCUS:
		{	
			// if we're givin the opportunity to take the keyboard focus we must say we don't want it
			if( mData1 == TRUE )
				*(Bool *)mData2 = FALSE;
		}
		//---------------------------------------------------------------------------------------------
		case GBM_SELECTED:
		{
			GameWindow *control = (GameWindow *)mData1;
			static NameKeyType buttonSelectID = NAMEKEY( "IdleWorker.wnd:ButtonSelectNextIdleWorker" );
			if (control && control->winGetWindowId() == buttonSelectID)
			{
				TheInGameUI->selectNextIdleWorker( );
			}
			break;

		}  // end button selected

		//---------------------------------------------------------------------------------------------
		default:
			return MSG_IGNORED;

	}  // end switch( msg )

	return MSG_HANDLED;

}


