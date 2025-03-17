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

// FILE: ControlBarPopupDescription.cpp /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//                                                                          
//                       Electronic Arts Pacific.                          
//                                                                          
//                       Confidential Information                           
//                Copyright (C) 2002 - All Rights Reserved                  
//                                                                          
//-----------------------------------------------------------------------------
//
//	created:	Sep 2002
//
//	Filename: 	ControlBarPopupDescription.cpp
//
//	author:		Chris Huybregts
//	
//	purpose:	
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// DEFINES ////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------


// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/GlobalData.h"
#include "Common/BuildAssistant.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/ProductionPrerequisite.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "GameClient/AnimateWindowManager.h"
#include "GameClient/DisconnectMenu.h"
#include "GameClient/GameWindow.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetTextEntry.h"
#include "GameClient/GadgetPushButton.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/GameClient.h"
#include "GameClient/GameText.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/InGameUI.h"
#include "GameClient/ControlBar.h"
#include "GameClient/DisplayStringManager.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/OverchargeBehavior.h"
#include "GameLogic/Module/ProductionUpdate.h"
#include "GameLogic/ScriptEngine.h"

#include "GameNetwork/NetworkInterface.h"
#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

static WindowLayout *theLayout = NULL;
static GameWindow *theWindow = NULL;
static AnimateWindowManager *theAnimateWindowManager = NULL;
static GameWindow *prevWindow = NULL;
static Bool useAnimation = FALSE;
void ControlBarPopupDescriptionUpdateFunc( WindowLayout *layout, void *param )
{
	if(TheScriptEngine->isGameEnding())
		TheControlBar->hideBuildTooltipLayout();
	
	if(theAnimateWindowManager && !TheControlBar->getShowBuildTooltipLayout() && !theAnimateWindowManager->isReversed())
		theAnimateWindowManager->reverseAnimateWindow();
	else if(!TheControlBar->getShowBuildTooltipLayout() && (!TheGlobalData->m_animateWindows || !useAnimation))
		TheControlBar->deleteBuildTooltipLayout();
		

	if ( useAnimation && theAnimateWindowManager && TheGlobalData->m_animateWindows)
	{
		Bool wasFinished = theAnimateWindowManager->isFinished();
		theAnimateWindowManager->update();
		if (theAnimateWindowManager && theAnimateWindowManager->isFinished() && !wasFinished && theAnimateWindowManager->isReversed())
		{
			delete theAnimateWindowManager;
			theAnimateWindowManager = NULL;
			TheControlBar->deleteBuildTooltipLayout();
		}
	}
	
}

// ---------------------------------------------------------------------------------------
void ControlBar::showBuildTooltipLayout( GameWindow *cmdButton )
{
	if (TheInGameUI->areTooltipsDisabled() 	|| TheScriptEngine->isGameEnding())
	{
		return;
	}

	Bool passedWaitTime = FALSE;
	static Bool isInitialized = FALSE;
	static UnsignedInt beginWaitTime;
	if(prevWindow == cmdButton)	
	{
		m_showBuildToolTipLayout = TRUE;
		if(!isInitialized &&  beginWaitTime + cmdButton->getTooltipDelay() < timeGetTime())
		{
			//DEBUG_LOG(("%d beginwaittime, %d tooltipdelay, %dtimegettime\n", beginWaitTime, cmdButton->getTooltipDelay(), timeGetTime()));
			passedWaitTime = TRUE;
		}
		
		if(!passedWaitTime)
			return;
	}
	else if( !m_buildToolTipLayout->isHidden() )
	{
		if(useAnimation && TheGlobalData->m_animateWindows && !theAnimateWindowManager->isReversed())
			theAnimateWindowManager->reverseAnimateWindow();
		else if( useAnimation && TheGlobalData->m_animateWindows && theAnimateWindowManager->isReversed())
		{
			return;
		}
		else
		{
//			m_buildToolTipLayout->destroyWindows();
//			m_buildToolTipLayout->deleteInstance();
//			m_buildToolTipLayout = NULL;
			m_buildToolTipLayout->hide(TRUE);
			prevWindow = NULL;
		}	
		return;
	}
	
	
	// will only get here the firsttime through the function through this window
	if(!passedWaitTime)
	{
		prevWindow = cmdButton;
		beginWaitTime = timeGetTime();
		isInitialized = FALSE;
		return;
	}
	isInitialized = TRUE;

	if(!cmdButton)
		return;
	if(BitTest(cmdButton->winGetStyle(), GWS_PUSH_BUTTON))
	{
		const CommandButton *commandButton = (const CommandButton *)GadgetButtonGetData(cmdButton);
		
		if(!commandButton)
			return;

	// note that, in this branch, ENABLE_SOLO_PLAY is ***NEVER*** defined...
	// this is so that we have a multiplayer build that cannot possibly be hacked
	// to work as a solo game!
	#if !defined(_PLAYTEST)
		if (TheGameLogic->isInReplayGame())
			return;
	#endif

		if (TheInGameUI->isQuitMenuVisible())
			return;

		if (TheDisconnectMenu && TheDisconnectMenu->isScreenVisible())
			return;

	//	if (m_buildToolTipLayout)
	//	{
	//		m_buildToolTipLayout->destroyWindows();
	//		m_buildToolTipLayout->deleteInstance();
	//
	//	}

		m_showBuildToolTipLayout = TRUE;
	//	m_buildToolTipLayout = TheWindowManager->winCreateLayout( "ControlBarPopupDescription.wnd" );
	//	m_buildToolTipLayout->setUpdate(ControlBarPopupDescriptionUpdateFunc);
		
		populateBuildTooltipLayout(commandButton);
	}
	else
	{
		// we're a generic window
		if(!BitTest(cmdButton->winGetStyle(), GWS_USER_WINDOW) && !BitTest(cmdButton->winGetStyle(), GWS_STATIC_TEXT))
			return;
		populateBuildTooltipLayout(NULL, cmdButton);
	}
	m_buildToolTipLayout->hide(FALSE);

	if (useAnimation && TheGlobalData->m_animateWindows)
	{
		theAnimateWindowManager = NEW AnimateWindowManager;	
		theAnimateWindowManager->reset();
		theAnimateWindowManager->registerGameWindow( m_buildToolTipLayout->getFirstWindow(), WIN_ANIMATION_SLIDE_RIGHT_FAST, TRUE, 200 );
	}
	
	
}


void ControlBar::repopulateBuildTooltipLayout( void )
{
	if(!prevWindow || !m_buildToolTipLayout)
		return;
	if(!BitTest(prevWindow->winGetStyle(), GWS_PUSH_BUTTON))
		return;
	const CommandButton *commandButton = (const CommandButton *)GadgetButtonGetData(prevWindow);
	populateBuildTooltipLayout(commandButton);
}

void ControlBar::populateBuildTooltipLayout( const CommandButton *commandButton, GameWindow *tooltipWin)
{
	if(!m_buildToolTipLayout)
		return;

	Player *player = ThePlayerList->getLocalPlayer();
	UnicodeString name, cost, descrip;
	UnicodeString requires = UnicodeString::TheEmptyString, requiresList;
	Bool firstRequirement = true;
	const ProductionPrerequisite *prereq;
	Bool fireScienceButton = false;

	if(commandButton)
	{
		const ThingTemplate *thingTemplate = commandButton->getThingTemplate();
		const UpgradeTemplate *upgradeTemplate = commandButton->getUpgradeTemplate();

		ScienceType	st = SCIENCE_INVALID; 
		if(commandButton->getScienceVec().size() > 1)
		{
			for(Int j = 0; j < commandButton->getScienceVec().size(); ++j)
			{
				st = commandButton->getScienceVec()[ j ];
				
				if( commandButton->getCommandType() != GUI_COMMAND_PURCHASE_SCIENCE )
				{
					if( !player->hasScience( st ) && j > 0 )
					{
						//If we're not looking at a command button that purchases a science, then
						//it means we are looking at a command button that can USE the science. This
						//means we want to get the description for the previous science -- the one
						//we can use, not purchase!
						st = commandButton->getScienceVec()[ j - 1 ];
					}

					//Now that we got the science for the button that executes the science, we need
					//to generate a simpler help text!
					fireScienceButton = true;

					break;
				}
				else if( !player->hasScience( st ) )
				{
					//Purchase science case. The first science we run into that we don't have, that's the
					//one we'll want to show!
					break;
				}
			}
		}
		else if(commandButton->getScienceVec().size()  == 1 )
		{
			st = commandButton->getScienceVec()[ 0 ];
		}

		if( commandButton->getDescriptionLabel().isNotEmpty() )
		{
			descrip = TheGameText->fetch(commandButton->getDescriptionLabel());

			Drawable *draw = TheInGameUI->getFirstSelectedDrawable();
			Object *selectedObject = draw ? draw->getObject() : NULL;
			if( selectedObject )
			{
				//Special case: Append status of overcharge on China power plant.
				if( commandButton->getCommandType() == GUI_COMMAND_TOGGLE_OVERCHARGE )
				{
					{
						OverchargeBehaviorInterface *obi;
						for( BehaviorModule **bmi = selectedObject->getBehaviorModules(); *bmi; ++bmi )
						{
							obi = (*bmi)->getOverchargeBehaviorInterface();
							if( obi )
							{
								descrip.concat( L"\n" );
								if( obi->isOverchargeActive() )
									descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipNukeReactorOverChargeIsOn" ) );
								else
									descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipNukeReactorOverChargeIsOff" ) );
							}
						}  
					}
				} //End overcharge special case
				
				//Special case: When building units, the CanMakeType determines reasons for not being able to buy stuff.
				else if( thingTemplate )
				{
					CanMakeType makeType = TheBuildAssistant->canMakeUnit( selectedObject, commandButton->getThingTemplate() );
					switch( makeType )
					{
						case CANMAKE_NO_MONEY:
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipNotEnoughMoneyToBuild" ) );
							break;
						case CANMAKE_QUEUE_FULL:
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotPurchaseBecauseQueueFull" ) );
							break;
						case CANMAKE_PARKING_PLACES_FULL:
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotBuildUnitBecauseParkingFull" ) );
							break;
						case CANMAKE_MAXED_OUT_FOR_PLAYER:
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotBuildUnitBecauseMaximumNumber" ) );
							break;
						//case CANMAKE_NO_PREREQ:
						//	descrip.concat( L"\n\n" );
						//	descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotBuildDueToPrerequisites" ) );
						//	break;
					}
				}

				//Special case: When building upgrades
				else if( upgradeTemplate && !player->hasUpgradeInProduction( upgradeTemplate ) )
				{
					if( commandButton->getCommandType() == GUI_COMMAND_PLAYER_UPGRADE ||
						  commandButton->getCommandType() == GUI_COMMAND_OBJECT_UPGRADE )
					{
						ProductionUpdateInterface *pui = selectedObject->getProductionUpdateInterface();
						if( pui && pui->getProductionCount() == MAX_BUILD_QUEUE_BUTTONS )
						{
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipCannotPurchaseBecauseQueueFull" ) );
						}
						else if( !TheUpgradeCenter->canAffordUpgrade( ThePlayerList->getLocalPlayer(), upgradeTemplate, FALSE ) )
						{
							descrip.concat( L"\n\n" );
							descrip.concat( TheGameText->fetch( "TOOLTIP:TooltipNotEnoughMoneyToBuild" ) );
						}
					}
				}
				
			}

		}

		name = TheGameText->fetch(commandButton->getTextLabel().str());

		if( thingTemplate && commandButton->getCommandType() != GUI_COMMAND_PURCHASE_SCIENCE )
		{
			//We are either looking at building a unit or a structure that may or may not have any 
			//prerequisites.

			//Format the cost only when we have to pay for it.
			cost.format(TheGameText->fetch("TOOLTIP:Cost"), thingTemplate->calcCostToBuild(player));

			// ask each prerequisite to give us a list of the non satisfied prerequisites
			for( Int i=0; i<thingTemplate->getPrereqCount(); i++ ) 
			{
				prereq = thingTemplate->getNthPrereq(i);
				requiresList = prereq->getRequiresList(player);

				if( requiresList != UnicodeString::TheEmptyString ) 
				{
					// make sure to put in 'returns' to space things correctly
					if (firstRequirement)
						firstRequirement = false;
					else
						requires.concat(L", ");
				}
				requires.concat(requiresList);
			}
			if( !requires.isEmpty() )
			{
				UnicodeString requireFormat = TheGameText->fetch("CONTROLBAR:Requirements");
				requires.format(requireFormat.str(), requires.str());
				if(!descrip.isEmpty())
					descrip.concat(L"\n");
				descrip.concat(requires);

			}
		}
		else if( upgradeTemplate )
		{
			//We are looking at an upgrade purchase icon. Maybe we already purchased it?

			Bool hasUpgradeAlready = false;
			Bool hasConflictingUpgrade = false;
			Bool playerUpgradeButton = commandButton->getCommandType() == GUI_COMMAND_PLAYER_UPGRADE;
			Bool objectUpgradeButton = commandButton->getCommandType() == GUI_COMMAND_OBJECT_UPGRADE;

			//Check if the local player has the specified upgrade
			hasUpgradeAlready = player->hasUpgradeComplete( upgradeTemplate );

			if( !hasUpgradeAlready )
			{
				//Check if the first selected object has the specified upgrade. 
				Drawable *draw = TheInGameUI->getFirstSelectedDrawable();
				if( draw )
				{
					Object *object = draw->getObject();
					if( object )
					{
						hasUpgradeAlready = object->hasUpgrade( upgradeTemplate );
						if( objectUpgradeButton )
						{
							hasConflictingUpgrade = !object->affectedByUpgrade( upgradeTemplate );
						}
					}
				}
			}
			if( hasConflictingUpgrade && !hasUpgradeAlready )
			{
				if( commandButton->getConflictingLabel().isNotEmpty() )
				{
					descrip = TheGameText->fetch( commandButton->getConflictingLabel() );
				}
				else
				{
					descrip = TheGameText->fetch( "TOOLTIP:HasConflictingUpgradeDefault" );
				}
			}
			else if( hasUpgradeAlready && ( playerUpgradeButton || objectUpgradeButton ) )
			{
				//See if we can fetch the "already upgraded" text for this upgrade. If not.... use the default "fill me in".
				if( commandButton->getPurchasedLabel().isNotEmpty() )
				{
					descrip = TheGameText->fetch( commandButton->getPurchasedLabel() );
				}
				else
				{
					descrip = TheGameText->fetch( "TOOLTIP:AlreadyUpgradedDefault" );
				}
			}
			else if( !hasUpgradeAlready )
			{
				//Determine the cost of the upgrade.
				cost.format(TheGameText->fetch("TOOLTIP:Cost"),upgradeTemplate->calcCostToBuild(player));
			}
		}	
		else if( st != SCIENCE_INVALID && !fireScienceButton )
		{
			TheScienceStore->getNameAndDescription(st, name, descrip);
			cost.format(TheGameText->fetch("TOOLTIP:ScienceCost"),TheScienceStore->getSciencePurchaseCost(st));
						// ask each prerequisite to give us a list of the non satisfied prerequisites

			if( thingTemplate )
			{
				for( Int i=0; i<thingTemplate->getPrereqCount(); i++ ) 
				{
					prereq = thingTemplate->getNthPrereq(i);
					requiresList = prereq->getRequiresList(player);

					if( requiresList != UnicodeString::TheEmptyString ) 
					{
						// make sure to put in 'returns' to space things correctly
						if (firstRequirement)
							firstRequirement = false;
						else
							requires.concat(L", ");
					}
					requires.concat(requiresList);
				}
				if( !requires.isEmpty() )
				{
					UnicodeString requireFormat = TheGameText->fetch("CONTROLBAR:Requirements");
					requires.format(requireFormat.str(), requires.str());
					if(!descrip.isEmpty())
						descrip.concat(L"\n");
					descrip.concat(requires);
				}
			}

		}
	}
	else if(tooltipWin)
	{
		
		if( tooltipWin == TheWindowManager->winGetWindowFromId(m_buildToolTipLayout->getFirstWindow(), TheNameKeyGenerator->nameToKey("ControlBar.wnd:MoneyDisplay")))
		{
			name = TheGameText->fetch("CONTROLBAR:Money");
			descrip = TheGameText->fetch("CONTROLBAR:MoneyDescription");
		}
		else if(tooltipWin == TheWindowManager->winGetWindowFromId(m_buildToolTipLayout->getFirstWindow(), TheNameKeyGenerator->nameToKey("ControlBar.wnd:PowerWindow")) )
		{
			name = TheGameText->fetch("CONTROLBAR:Power");
			descrip = TheGameText->fetch("CONTROLBAR:PowerDescription");

			Player *playerToDisplay = NULL;
			if(TheControlBar->isObserverControlBarOn())
				playerToDisplay = TheControlBar->getObserverLookAtPlayer();
			else
				playerToDisplay = ThePlayerList->getLocalPlayer();

			if( playerToDisplay && playerToDisplay->getEnergy() )
			{
				Energy *energy = playerToDisplay->getEnergy();
				descrip.format(descrip, energy->getProduction(), energy->getConsumption());
			}
			else
			{
				descrip.format(descrip, 0, 0);
			}
		}
		else if(tooltipWin == TheWindowManager->winGetWindowFromId(m_buildToolTipLayout->getFirstWindow(), TheNameKeyGenerator->nameToKey("ControlBar.wnd:GeneralsExp")) )
		{
			name = TheGameText->fetch("CONTROLBAR:GeneralsExp");
			descrip = TheGameText->fetch("CONTROLBAR:GeneralsExpDescription");
		}
		else
		{
			DEBUG_ASSERTCRASH(FALSE, ("ControlBar::populateBuildTooltipLayout We attempted to call the popup tooltip on a game window that has yet to be hand coded in as this fuction was/is designed for only buttons but has been hacked to work with GameWindows."));
			return;
		}
		
	}
	GameWindow *win = TheWindowManager->winGetWindowFromId(m_buildToolTipLayout->getFirstWindow(), TheNameKeyGenerator->nameToKey("ControlBarPopupDescription.wnd:StaticTextName"));
	if(win)
	{
		GadgetStaticTextSetText(win, name);
	}
	win = TheWindowManager->winGetWindowFromId(m_buildToolTipLayout->getFirstWindow(), TheNameKeyGenerator->nameToKey("ControlBarPopupDescription.wnd:StaticTextCost"));
	if(win)
	{
		GadgetStaticTextSetText(win, cost);
	}
	win = TheWindowManager->winGetWindowFromId(m_buildToolTipLayout->getFirstWindow(), TheNameKeyGenerator->nameToKey("ControlBarPopupDescription.wnd:StaticTextDescription"));
	if(win)
	{

		static NameKeyType winNamekey	= TheNameKeyGenerator->nameToKey( AsciiString( "ControlBar.wnd:BackgroundMarker" ) );
		static ICoord2D lastOffset = { 0, 0 };

		ICoord2D size, newSize, pos;
		Int diffSize;
		
		DisplayString *tempDString = TheDisplayStringManager->newDisplayString();
		win->winGetSize(&size.x, &size.y);
		tempDString->setFont(win->winGetFont());
		tempDString->setWordWrap(size.x - 10);
		tempDString->setText(descrip);
		tempDString->getSize(&newSize.x, &newSize.y);
		TheDisplayStringManager->freeDisplayString(tempDString);
		tempDString = NULL;
		diffSize = newSize.y - size.y;
 		GameWindow *parent = m_buildToolTipLayout->getFirstWindow();
 		if(!parent)
 			return;
		
 		parent->winGetSize(&size.x, &size.y);
 		if(size.y + diffSize < 102) {
			diffSize = 102 - size.y;
		}

		parent->winSetSize(size.x, size.y + diffSize);
 		parent->winGetPosition(&pos.x, &pos.y);
//		if(size.y + diffSize < 102)
//		{
//			
//			parent->winSetPosition(pos.x, pos.y -  (102 - (newSize.y + size.y + diffSize) ));
//		}
//		else

//		heightChange = controlBarPos.y - m_defaultControlBarPosition.y;

		GameWindow *marker =  TheWindowManager->winGetWindowFromId(NULL,winNamekey);
		static ICoord2D basePos;
		if(!marker)
		{
			return;
		}
		TheControlBar->getBackgroundMarkerPos(&basePos.x, &basePos.y);
		ICoord2D curPos, offset;
		marker->winGetScreenPosition(&curPos.x,&curPos.y);

		offset.x = curPos.x - basePos.x;
		offset.y = curPos.y - basePos.y;

		parent->winSetPosition(pos.x, (pos.y - diffSize) + (offset.y - lastOffset.y));

		lastOffset.x = offset.x;
		lastOffset.y = offset.y;

		win->winGetSize(&size.x, &size.y);
 		win->winSetSize(size.x, size.y + diffSize);

		GadgetStaticTextSetText(win, descrip);		
	}
	m_buildToolTipLayout->hide(FALSE);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ControlBar::hideBuildTooltipLayout()
{
	if(theAnimateWindowManager && theAnimateWindowManager->isReversed())
		return;
	if(useAnimation && theAnimateWindowManager && TheGlobalData->m_animateWindows)
		theAnimateWindowManager->reverseAnimateWindow();
	else
		deleteBuildTooltipLayout();

}

void ControlBar::deleteBuildTooltipLayout( void )
{
	m_showBuildToolTipLayout = FALSE;
	prevWindow= NULL;
	m_buildToolTipLayout->hide(TRUE);
//	if(!m_buildToolTipLayout)
//		return;
//	
//	m_buildToolTipLayout->destroyWindows();
//	m_buildToolTipLayout->deleteInstance();
//	m_buildToolTipLayout = NULL;
	if(theAnimateWindowManager)
		delete theAnimateWindowManager;
	theAnimateWindowManager = NULL;

}
