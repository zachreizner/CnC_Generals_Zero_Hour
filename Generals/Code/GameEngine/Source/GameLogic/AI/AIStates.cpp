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

// AIStates.cpp
// Implementation of AI behavior states
// Author: Michael S. Booth, January 2002
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine


#include "Common/ActionManager.h"
#include "Common/AudioHandleSpecialValues.h"
#include "Common/CRCDebug.h"
#include "Common/GameAudio.h"
#include "Common/GlobalData.h"
#include "Common/Money.h"
#include "Common/PerfTimer.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/RandomValue.h"
#include "Common/Team.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/Xfer.h"
#include "Common/XferCRC.h"

#include "GameClient/ControlBar.h"
#include "GameClient/FXList.h"
#include "GameClient/InGameUI.h"

#include "GameLogic/AIDock.h"
#include "GameLogic/AIGuard.h"
#include "GameLogic/AITNGuard.h"
#include "GameLogic/AIStateMachine.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/Locomotor.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/StealthUpdate.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/PolygonTrigger.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/Squad.h"
#include "GameLogic/TurretAI.h"
#include "GameLogic/Weapon.h"

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

static Bool cannotPossiblyAttackObject( State *thisState, void* userData );

//----------------------------------------------------------------------------------------------------------
AICommandParms::AICommandParms(AICommandType cmd, CommandSourceType cmdSource) : 
	m_cmd(cmd),
	m_cmdSource(cmdSource), 
	m_obj(NULL),
	m_otherObj(NULL),
	m_team(NULL),
	m_waypoint(NULL),
	m_polygon(NULL),
	m_intValue(0),
	m_commandButton(NULL),
	m_path(NULL)
{ 
		m_pos.zero();
		m_coords.clear();
}

//----------------------------------------------------------------------------------------------------------
void AICommandParmsStorage::store(const AICommandParms& parms)
{
	m_cmd = parms.m_cmd;
  m_cmdSource = parms.m_cmdSource;
  m_pos = parms.m_pos;
  m_obj = parms.m_obj ? parms.m_obj->getID() : INVALID_ID;
  m_otherObj = parms.m_otherObj ? parms.m_otherObj->getID() : INVALID_ID;
  m_teamName = parms.m_team ? parms.m_team->getName() : AsciiString::TheEmptyString;
	m_coords = parms.m_coords;
  m_waypoint = parms.m_waypoint; 
  m_polygon = parms.m_polygon;     
  m_intValue = parms.m_intValue;       /// misc usage
  m_damage = parms.m_damage;
	m_commandButton = parms.m_commandButton;
	m_path = parms.m_path;	/// @todo srj -- probably need a better way to safely save/restore this
}

//----------------------------------------------------------------------------------------------------------
void AICommandParmsStorage::reconstitute(AICommandParms& parms) const
{
	parms.m_cmd = m_cmd;
  parms.m_cmdSource = m_cmdSource;
  parms.m_pos = m_pos;
  parms.m_obj = TheGameLogic->findObjectByID(m_obj);
  parms.m_otherObj = TheGameLogic->findObjectByID(m_otherObj);
  parms.m_team = TheTeamFactory->findTeam(m_teamName);
	parms.m_coords = m_coords;
  parms.m_waypoint = m_waypoint;
  parms.m_polygon = m_polygon;
  parms.m_intValue = m_intValue;
  parms.m_damage = m_damage;
	parms.m_commandButton = m_commandButton;
	parms.m_path = m_path;	/// @todo srj -- probably need a better way to safely save/restore this
}

//----------------------------------------------------------------------------------------------------------
void AICommandParmsStorage::doXfer(Xfer *xfer) 
{
	xfer->xferUser(&m_cmd, sizeof(m_cmd));
	xfer->xferUser(&m_cmd, sizeof(m_cmdSource));
	xfer->xferCoord3D(&m_pos);
	xfer->xferObjectID(&m_obj);
	xfer->xferObjectID(&m_otherObj);
	xfer->xferAsciiString(&m_teamName);
	Int numCoords = m_coords.size();
	xfer->xferInt(&numCoords);
	Int i;
	if (xfer->getXferMode() == XFER_LOAD)
	{
		for (i=0; i<numCoords; i++) {
			Coord3D pos;
			xfer->xferCoord3D(&pos);
			m_coords.push_back(pos);
		}
	} else {
		for (i=0; i<numCoords; i++) {
			Coord3D pos = m_coords[i];
			xfer->xferCoord3D(&pos);
		}
	}

 	UnsignedInt id = INVALID_WAYPOINT_ID;
	if (m_waypoint) {
		id = m_waypoint->getID();
	}
	xfer->xferUnsignedInt(&id);
	if (xfer->getXferMode() == XFER_LOAD && id!= INVALID_WAYPOINT_ID)
	{
		m_waypoint = TheTerrainLogic->getWaypointByID(id);
	}

	AsciiString triggerName;
	if (m_polygon) triggerName = m_polygon->getTriggerName();
	xfer->xferAsciiString(&triggerName);
	if (xfer->getXferMode() == XFER_LOAD)
	{
		if (triggerName.isNotEmpty()) {
			m_polygon = TheTerrainLogic->getTriggerAreaByName(triggerName);
		}
	} 

	xfer->xferInt(&m_intValue);

	xfer->xferSnapshot(&m_damage);

	AsciiString cmdName;
	if (m_commandButton) {
		cmdName = m_commandButton->getName();
	}
	xfer->xferAsciiString(&cmdName);
	if (cmdName.isNotEmpty() && m_commandButton==NULL) {
		m_commandButton = TheControlBar->findCommandButton(cmdName);
	}

	Bool hasPath = m_path!=NULL;
	xfer->xferBool(&hasPath);
	if (hasPath && m_path==NULL) {
		m_path = newInstance(Path);
	}
	if (hasPath) {
		xfer->xferSnapshot(m_path);
	}

}

//----------------------------------------------------------------------------------------------------------
/**
 * Compare two positions to see if they are logically equal
 * @todo Move this somewhere more useful (MSB)
 */
static Bool isSamePosition( const Coord3D *ourPos, const Coord3D *prevTargetPos, const Coord3D *curTargetPos )
{
	Coord3D diff;

	// for pathfinding purposes, only care about 2d pos. (srj)
	diff.x = curTargetPos->x - prevTargetPos->x;
	diff.y = curTargetPos->y - prevTargetPos->y;

	Coord3D toTarget;
 	// for pathfinding purposes, only care about 2d pos. (srj)
	toTarget.x = curTargetPos->x - ourPos->x;
	toTarget.y = curTargetPos->y - ourPos->y;

	// Tolerance is (dist/10)squared.
	const Real TOLERANCE_FACTOR = 1.0f / (10.0f * 10.0f);
	Real toleranceSqr = (toTarget.x*toTarget.x+toTarget.y*toTarget.y) * TOLERANCE_FACTOR;

	if (diff.x * diff.x + diff.y * diff.y > toleranceSqr)
		return false;

	return true;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AttackStateMachine::crc( Xfer *xfer )
{
	StateMachine::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AttackStateMachine::xfer( Xfer *xfer )
{
	XferVersion cv = 1;	
	XferVersion v = cv; 
	xfer->xferVersion( &v, cv );

	StateMachine::xfer(xfer);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AttackStateMachine::loadPostProcess( void )
{
	StateMachine::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
static Bool inWeaponRangeObject(State *thisState, void* userData);

//----------------------------------------------------------------------------------------------------------
/**
 * Create an AI state machine. Define all of the states the machine 
 * can possibly be in, and set the initial (default) state.
 */
AttackStateMachine::AttackStateMachine( Object *obj, AIAttackState* att, AsciiString name, Bool follow, Bool attackingObject, Bool forceAttacking ) 
	: StateMachine( obj, name )
{
	// we want to use the CONTINUE mode (not NEW) since we already have acquired the target.
	static const StateConditionInfo objectConditionsNormal[] =
	{
		StateConditionInfo(outOfWeaponRangeObject, AttackStateMachine::CHASE_TARGET, NULL),
		StateConditionInfo(wantToSquishTarget, AttackStateMachine::CHASE_TARGET, NULL),
		StateConditionInfo(cannotPossiblyAttackObject, EXIT_MACHINE_WITH_FAILURE, (void*)ATTACK_CONTINUED_TARGET),
		StateConditionInfo(NULL, NULL, NULL)	// keep last
	};

	// we want to use the CONTINUE mode (not NEW) since we already have acquired the target.
	static const StateConditionInfo objectConditionsForced[] =
	{
		StateConditionInfo(outOfWeaponRangeObject, AttackStateMachine::CHASE_TARGET, NULL),
		StateConditionInfo(cannotPossiblyAttackObject, EXIT_MACHINE_WITH_FAILURE, (void*)ATTACK_CONTINUED_TARGET_FORCED),
		StateConditionInfo(wantToSquishTarget, AttackStateMachine::CHASE_TARGET, NULL),
		StateConditionInfo(NULL, NULL, NULL)	// keep last
	};

	const StateConditionInfo* objectConditions = forceAttacking ? objectConditionsForced : objectConditionsNormal;

	static const StateConditionInfo positionConditions[] = 
	{
		StateConditionInfo(outOfWeaponRangePosition, AttackStateMachine::CHASE_TARGET, NULL),
		StateConditionInfo(NULL, NULL, NULL)	// keep last
	};

#ifdef STATE_MACHINE_DEBUG
		AsciiString fullName = name;
		if (follow) fullName.concat(" follow");
		if (attackingObject) fullName.concat(" object");
		setName(fullName);
		//setDebugOutput(true);
#endif 

	// order matters: first state is the default state.
	// The default is Aim rather than Approach so things that cannot move will be able to shoot
	// things that are in range.  Things that cannot move will automatically FAILURE on approach state.
	/*
		This state will succeed when we are aiming a useful weapon at the victim, and fail 
		if the victim is dead. (Exception: if the weapon is on a turret, we don't leave this
		state unless we get out of range.)
	*/
	defineState(	AttackStateMachine::AIM_AT_TARGET, 
								newInstance(AIAttackAimAtTargetState)( this, attackingObject, forceAttacking ), 
								AttackStateMachine::FIRE_WEAPON, 
								EXIT_MACHINE_WITH_FAILURE,
								attackingObject ? objectConditions : positionConditions );

	/*
		Note that the fire state succeeds iff it is able to fire... it will "fail"
		if unable to fire. However, it may be unable to fire because the target object
		is already dead.
	*/
	defineState( AttackStateMachine::FIRE_WEAPON, 
								newInstance(AIAttackFireWeaponState)( this, att ),
								AttackStateMachine::AIM_AT_TARGET,
								AttackStateMachine::AIM_AT_TARGET,
								attackingObject ? objectConditions : positionConditions );


	if (obj->isKindOf(KINDOF_IMMOBILE) == FALSE)
	{
		if (obj->isKindOf(KINDOF_PORTABLE_STRUCTURE) && obj->isKindOf(KINDOF_CAN_ATTACK))
		{
			static const StateConditionInfo portableStructureChaseConditions[] =
			{
				StateConditionInfo(inWeaponRangeObject, AttackStateMachine::AIM_AT_TARGET, NULL),
				StateConditionInfo(NULL, NULL, NULL)	// keep last
			};

			/* we're a rider on a mobile object, so we can't control our motion. 
				just make bogus states that always fall back into "aim".
			*/
			defineState(	AttackStateMachine::CHASE_TARGET, 
										newInstance(ContinueState)(this), 
										EXIT_MACHINE_WITH_FAILURE, 
										EXIT_MACHINE_WITH_FAILURE,
										portableStructureChaseConditions );
		}
		else if (attackingObject) 
		{
			/*
				This state will pursue a target that is moving away from it.  If it is not moving away, 
				it will drop into the AIAttackApproachTarget state.
			*/
			defineState(	AttackStateMachine::CHASE_TARGET, 
										newInstance(AIAttackPursueTargetState)( this, follow, attackingObject, forceAttacking ), 
										AttackStateMachine::APPROACH_TARGET, 
										AttackStateMachine::APPROACH_TARGET );

			/*
				This state will succeed when we have a useful weapon within range of victim, and fail 
				if the victim is dead
			*/
			defineState(	AttackStateMachine::APPROACH_TARGET, 
										newInstance(AIAttackApproachTargetState)( this, follow, attackingObject, forceAttacking ), 
										AttackStateMachine::AIM_AT_TARGET, 
										EXIT_MACHINE_WITH_FAILURE );
		}	
		else 
		{
			/*
				This state will succeed when we have a useful weapon within range of victim, and fail 
				if the victim is dead
			*/
			defineState(	AttackStateMachine::CHASE_TARGET, 
										newInstance(AIAttackApproachTargetState)( this, follow, attackingObject, forceAttacking ), 
										AttackStateMachine::AIM_AT_TARGET, 
										EXIT_MACHINE_WITH_FAILURE );

			/*
				This state will succeed when we have a useful weapon within range of victim, and fail 
				if the victim is dead
			*/
			defineState(	AttackStateMachine::APPROACH_TARGET, 
										newInstance(AIAttackApproachTargetState)( this, follow, attackingObject, forceAttacking ), 
										AttackStateMachine::AIM_AT_TARGET, 
										EXIT_MACHINE_WITH_FAILURE );
		}

	}
	else
	{
		/*
			This state always instantly fails, so when immobile things transition here, we bail.
		*/
		defineState(	AttackStateMachine::CHASE_TARGET, 
									newInstance(FailureState)(this), 
									EXIT_MACHINE_WITH_FAILURE, 
									EXIT_MACHINE_WITH_FAILURE );
	}

};

//----------------------------------------------------------------------------------------------------------
AttackStateMachine::~AttackStateMachine()
{
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
static Object* findEnemyInContainer(Object* killer, Object* bldg)
{
	const ContainedItemsList* items = bldg->getContain() ? bldg->getContain()->getContainedItemsList() : NULL;
	if (items)
	{
		for (ContainedItemsList::const_iterator it = items->begin(); it != items->end(); ++it )
		{
			if ((*it)->isEffectivelyDead())
			{
				DEBUG_CRASH(("why is there a dead thing in this container?"));
				continue;
			}

			// order matters: we want to know if I consider it to be an enemy, not vice versa
			if (killer->getRelationship(*it) == ENEMIES)
			{
				return *it;
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------
static Int killEnemiesInContainer(Object* killer, Object* bldg, Int maxToKill)
{
	Int numKilled = 0;
	while (numKilled < maxToKill)
	{
		Object* enemy = findEnemyInContainer(killer, bldg);
		if (enemy)
		{
			Object *containedByObject = enemy->getContainedBy();
			if( containedByObject )
			{
				ContainModuleInterface *contain = containedByObject->getContain();
				if( contain )
				{
					contain->removeFromContain(enemy);
				}
			}
			if (killer)
				killer->scoreTheKill( enemy );
			enemy->kill();
			++numKilled;
		}
		else
		{
			break;
		}
	}
	return numKilled;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
AIRappelState::AIRappelState( StateMachine *machine ) : State( machine, "AIRappelState" ) 
{ 
}

//-----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIRappelState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIRappelState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );
 
	xfer->xferReal(&m_rappelRate);
	xfer->xferReal(&m_destZ);
	xfer->xferBool(&m_targetIsBldg);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIRappelState::loadPostProcess( void )
{
}  // end loadPostProcess
//-----------------------------------------------------------------------------------------------------------
StateReturnType AIRappelState::onEnter()
{
	Object* obj = getMachineOwner();
	if (!obj->isKindOf(KINDOF_CAN_RAPPEL))
		return STATE_FAILURE;

	//AIUpdateInterface* ai = obj->getAI();

	obj->setModelConditionState(MODELCONDITION_RAPPELLING);
// don't do this, or we'll be unable to be forced out of this state.
// instead, just manipulate physics directly.
//obj->setHeld();
	obj->getPhysics()->resetDynamicPhysics();

	m_targetIsBldg = true;
	Object* bldg = getMachineGoalObject();
	if (bldg == NULL || bldg->isEffectivelyDead() || !bldg->isKindOf(KINDOF_STRUCTURE))
		m_targetIsBldg = false;

	const Coord3D* pos = obj->getPosition();

	const Bool onlyHealthyBridges = true;	// ignore dead bridges.
	PathfindLayerEnum layerAtDest = TheTerrainLogic->getHighestLayerForDestination(pos, onlyHealthyBridges);
	m_destZ = TheTerrainLogic->getLayerHeight(pos->x, pos->y, layerAtDest);
	if (m_targetIsBldg)
		m_destZ += bldg->getGeometryInfo().getMaxHeightAbovePosition();
	else
		obj->setLayer(layerAtDest);

	AIUpdateInterface *ai = obj->getAI();
	Real MAX_RAPPEL_RATE = fabs(TheGlobalData->m_gravity) * LOGICFRAMES_PER_SECOND * 2.5f;
	m_rappelRate = -min(ai->getDesiredSpeed(), MAX_RAPPEL_RATE);

	return STATE_CONTINUE;
}

//-----------------------------------------------------------------------------------------------------------

StateReturnType AIRappelState::update()
{

	StateReturnType result = STATE_CONTINUE;
	
	Object* obj = getMachineOwner();
	const Coord3D* pos = obj->getPosition();

	Object* bldg = getMachineGoalObject();
	if (m_targetIsBldg && (bldg == NULL || bldg->isEffectivelyDead()))
	{
		// if bldg is destroyed, just head for the ground
		// BGC - bldg could be destroyed as they are heading down the rope.
		m_targetIsBldg = false;
		m_destZ = TheTerrainLogic->getGroundHeight(pos->x, pos->y);
	}
	
	// nuke 2d speed...
	obj->getPhysics()->scrubVelocity2D(0);
	// and clamp z speed to rappel rate (gravity will have accelerated us)
	obj->getPhysics()->scrubVelocityZ(m_rappelRate);

	if (!m_targetIsBldg)
	{
		// if heading for ground, do this every frame... since jitter at the very start
		// can move us slightly, and on uneven ground it might matter.
		m_destZ = TheTerrainLogic->getLayerHeight(pos->x, pos->y, obj->getLayer());
	}
	if (pos->z <= m_destZ)
	{
		Coord3D tmp = *pos;
		tmp.z = m_destZ;
		obj->setPosition(&tmp);
		
		if (m_targetIsBldg)
		{
			DEBUG_ASSERTCRASH(TheActionManager->canEnterObject(obj, bldg, obj->getAI()->getLastCommandSource(), COMBATDROP_INTO), ("Hmm, this seems unlikely"));
			// if there are enemies... kill up to two. if we kill two, then we die ourselves,
			// otherwise we enter the bldg.
			const Int MAX_TO_KILL = 2;
			Int numKilled = killEnemiesInContainer(obj, bldg, MAX_TO_KILL);
			
			if (numKilled > 0)
			{
				const FXList* fx = obj->getTemplate()->getPerUnitFX("CombatDropKillFX");
				FXList::doFXObj(fx, bldg, NULL);
				DEBUG_LOG(("Killing %d enemies in combat drop!\n",numKilled));
			}

			if (numKilled == MAX_TO_KILL)
			{
				obj->kill();
				DEBUG_LOG(("Killing SELF in combat drop!\n"));
			}
			else
			{
				if (bldg->getContain() && bldg->getContain()->isValidContainerFor(obj, TRUE ))
				{
					bldg->getContain()->addToContain(obj);
				}
				else
				{
					// this can legitimately happen if you drop into a full (or nearly full) building.
					// let's just place the guy on the ground nearby, since it sucks to fall from the top of a building.
					Real exitAngle = bldg->getOrientation();
					// Garrison doesn't have reserveDoor or exitDelay, so if we do nothing, everyone will appear on top 
					// of each other and get stuck inside each others' extent (except for the first guy).  So we'll
					// scatter the start point around a little to make it better.
					Real offset = min(obj->getGeometryInfo().getBoundingCircleRadius(), 
														bldg->getGeometryInfo().getBoundingCircleRadius());
					Real angle = GameLogicRandomValueReal( PI, 2*PI );//Downish.
					Coord3D startPosition = *bldg->getPosition();
					startPosition.x += offset * Cos( angle );
					startPosition.y += offset * Sin( angle );
					startPosition.z = TheTerrainLogic->getGroundHeight( startPosition.x, startPosition.y );

					obj->setPosition( &startPosition );
					obj->setOrientation( exitAngle );
					
					FindPositionOptions options;
					options.startAngle = (Real)(1.5 * PI);//Down.
					options.maxRadius = 200;
					Coord3D endPosition;
					Bool foundPosition = ThePartitionManager->findPositionAround( &startPosition, &options, &endPosition );

					if( foundPosition )
					{
						std::vector<Coord3D> exitPath;
						exitPath.push_back(endPosition);
						AIUpdateInterface* ai = obj->getAI();
						if( ai )
						{
							ai->aiFollowPath( &exitPath, bldg, CMD_FROM_AI );
						}
					}

				}
			}
		}

		result = STATE_SUCCESS;
	}

	return result;
}

//-----------------------------------------------------------------------------------------------------------
void AIRappelState::onExit( StateExitType status )
{
	Object* obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();
	obj->clearModelConditionState(MODELCONDITION_RAPPELLING);
// don't do this, or we'll be unable to be forced out of this state.
// instead, just manipulate physics directly.
//obj->clearHeld();
	ai->setDesiredSpeed( FAST_AS_POSSIBLE );
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
/*

	NOTE NOTE NOTE NOTE NOTE

	Do NOT subclass this unless you want ALL of the states this machine possesses.
	If you only want SOME of the states, please make a new StateMachine, descended
	from StateMachine, NOT AIStateMachine. Thank you. (srj)

	NOTE NOTE NOTE NOTE NOTE

*/
AIStateMachine::AIStateMachine( Object *obj, AsciiString name ) : StateMachine( obj, name )
{
	DEBUG_ASSERTCRASH(getOwner(), ("An AI State Machine '%s' was constructed without an owner, please tell JKMCD", name));
	DEBUG_ASSERTCRASH(getOwner()->getAI(), ("An AI State Machine '%s' was constructed without an AIUpdateInterface, please tell JKMCD", name));

	m_goalPath.clear();
	m_goalWaypoint = NULL;
	m_goalSquad = NULL;

	m_temporaryState = NULL;
	m_temporaryStateFramEnd = 0;

	// order matters: first state is the default state.
	defineState( AI_IDLE,																	newInstance(AIIdleState)( this, AIIdleState::LOOK_FOR_TARGETS), AI_IDLE, AI_IDLE );
	defineState( AI_MOVE_TO,															newInstance(AIMoveToState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_MOVE_OUT_OF_THE_WAY,									newInstance(AIMoveOutOfTheWayState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_MOVE_AND_TIGHTEN,											newInstance(AIMoveAndTightenState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_MOVE_AWAY_FROM_REPULSORS,							newInstance(AIMoveAwayFromRepulsorsState)( this ), AI_WANDER_IN_PLACE, AI_WANDER_IN_PLACE );
	defineState( AI_WANDER_IN_PLACE,											newInstance(AIWanderInPlaceState)( this ), AI_MOVE_AWAY_FROM_REPULSORS, AI_MOVE_AWAY_FROM_REPULSORS );
	defineState( AI_ATTACK_MOVE_TO,												newInstance(AIAttackMoveToState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_ATTACKFOLLOW_WAYPOINT_PATH_AS_TEAM,					newInstance(AIAttackFollowWaypointPathState)( this, true ), AI_IDLE, AI_IDLE );
	defineState( AI_ATTACKFOLLOW_WAYPOINT_PATH_AS_INDIVIDUALS,	newInstance(AIAttackFollowWaypointPathState)( this, false ), AI_IDLE, AI_IDLE );
	
	defineState( AI_FOLLOW_WAYPOINT_PATH_AS_TEAM,					newInstance(AIFollowWaypointPathState)( this, true ), AI_IDLE, AI_IDLE );
	defineState( AI_FOLLOW_WAYPOINT_PATH_AS_INDIVIDUALS,	newInstance(AIFollowWaypointPathState)( this, false ), AI_IDLE, AI_IDLE );
	defineState( AI_FOLLOW_WAYPOINT_PATH_AS_TEAM_EXACT,		newInstance(AIFollowWaypointPathExactState)( this, true ), AI_IDLE, AI_IDLE );
	defineState( AI_FOLLOW_WAYPOINT_PATH_AS_INDIVIDUALS_EXACT,newInstance(AIFollowWaypointPathExactState)( this, false ), AI_IDLE, AI_IDLE );
	defineState( AI_FOLLOW_PATH,													newInstance(AIFollowPathState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_FOLLOW_EXITPRODUCTION_PATH,						newInstance(AIFollowPathState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_MOVE_AND_EVACUATE,					newInstance(AIMoveAndEvacuateState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_MOVE_AND_EVACUATE_AND_EXIT,	newInstance(AIMoveAndEvacuateState)( this ), AI_MOVE_AND_DELETE, AI_MOVE_AND_DELETE );
	defineState( AI_MOVE_AND_DELETE,						newInstance(AIMoveAndDeleteState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_WAIT,												newInstance(AIWaitState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_ATTACK_POSITION,						newInstance(AIAttackState)( this, false, false, false,  NULL ), AI_IDLE, AI_IDLE );
	defineState( AI_ATTACK_OBJECT,							newInstance(AIAttackState)( this, false, true, false, NULL ), AI_IDLE, AI_IDLE );
	defineState( AI_FORCE_ATTACK_OBJECT,				newInstance(AIAttackState)( this, false, true, true, NULL ), AI_IDLE, AI_IDLE );

	defineState( AI_ATTACK_AND_FOLLOW_OBJECT,		newInstance(AIAttackState)( this, true, true, false, NULL ), AI_IDLE, AI_IDLE );
	defineState( AI_ATTACK_SQUAD,								newInstance(AIAttackSquadState)( this, NULL ), AI_IDLE, AI_IDLE );
	defineState( AI_WANDER,											newInstance(AIWanderState)( this ), AI_IDLE, AI_MOVE_AWAY_FROM_REPULSORS );
	defineState( AI_PANIC,											newInstance(AIPanicState)( this ), AI_IDLE, AI_MOVE_AWAY_FROM_REPULSORS );
	defineState( AI_DEAD,												newInstance(AIDeadState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_DOCK,												newInstance(AIDockState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_ENTER,											newInstance(AIEnterState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_EXIT,												newInstance(AIExitState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_GUARD,											newInstance(AIGuardState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_GUARD_TUNNEL_NETWORK,				newInstance(AITunnelNetworkGuardState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_HUNT,												newInstance(AIHuntState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_ATTACK_AREA,								newInstance(AIAttackAreaState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_FACE_OBJECT,								newInstance(AIFaceState)( this, true ), AI_IDLE, AI_IDLE );
	defineState( AI_FACE_POSITION,							newInstance(AIFaceState)( this, false ), AI_IDLE, AI_IDLE );
	defineState( AI_PICK_UP_CRATE,							newInstance(AIPickUpCrateState)( this ), AI_IDLE, AI_IDLE );

	defineState( AI_RAPPEL_INTO,								newInstance(AIRappelState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_BUSY,												newInstance(AIBusyState)( this ), AI_IDLE, AI_IDLE );
}

//----------------------------------------------------------------------------------------------------------
AIStateMachine::~AIStateMachine()
{
	if (m_goalSquad) 
	{
		m_goalSquad->deleteInstance();
	}
}


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIStateMachine::crc( Xfer *xfer )
{
	StateMachine::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIStateMachine::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

 // extend base class
	StateMachine::xfer(xfer);

	Int i;
	Int count = m_goalPath.size();
	xfer->xferInt(&count);
	for (i=0; i<count; i++) {
		Coord3D pos;
		if (xfer->getXferMode() != XFER_LOAD)
		{
			pos = m_goalPath[i];
		} 
		xfer->xferCoord3D(&pos);
		if (xfer->getXferMode() == XFER_LOAD)
		{
			m_goalPath.push_back(pos);
		}
	}

	AsciiString waypointName;
	if (m_goalWaypoint) waypointName = m_goalWaypoint->getName();
	xfer->xferAsciiString(&waypointName);
	if (xfer->getXferMode() == XFER_LOAD)
	{
		if (waypointName.isNotEmpty()) {
			m_goalWaypoint = TheTerrainLogic->getWaypointByName(waypointName);
		}
	} 
	Bool hasSquad = (m_goalSquad!=NULL);
	xfer->xferBool(&hasSquad);
	if (xfer->getXferMode() == XFER_LOAD)
	{
		if (hasSquad && m_goalSquad==NULL) {
			m_goalSquad = newInstance( Squad );
		}
	} 
	if (hasSquad) {
		xfer->xferSnapshot(m_goalSquad);
	}

	StateID id = INVALID_STATE_ID;
	if (m_temporaryState) {
		id = m_temporaryState->getID();
		DEBUG_ASSERTCRASH(id!=INVALID_STATE_ID, ("State has invalid state id, no really. jba."));
	}
	xfer->xferUnsignedInt(&id);
	if (xfer->getXferMode() == XFER_LOAD && id != INVALID_STATE_ID) {
		m_temporaryState = internalGetState( id );
	}
	if (m_temporaryState!=NULL) {
		xfer->xferSnapshot(m_temporaryState);
	}

	xfer->xferUnsignedInt(&m_temporaryStateFramEnd);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIStateMachine::loadPostProcess( void )
{
	StateMachine::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
/**
 * Define a simple path
 */
void AIStateMachine::setGoalPath( const std::vector<Coord3D>* path )
{
	m_goalPath = *path;
}

#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
/**
 * Get the current state name.
 */
AsciiString AIStateMachine::getCurrentStateName(void) const
{
	AsciiString name = StateMachine::getCurrentStateName();

	if (m_temporaryState) {
		name.concat(" /T/");
		name.concat(m_temporaryState->getName()); 
	}
	return name;					
}
#endif

//-----------------------------------------------------------------------------
/**
 * Run one step of the machine
 */
StateReturnType AIStateMachine::updateStateMachine()
{
	if (m_temporaryState)
	{
		// execute this state
		StateReturnType status = m_temporaryState->update();
		if (m_temporaryStateFramEnd < TheGameLogic->getFrame()) {
			// ran out of time.
			if (status == STATE_CONTINUE) {
				status = STATE_SUCCESS;
			}
		}
		if (status==STATE_CONTINUE)	{
			return status;
		}
		m_temporaryState->onExit(EXIT_NORMAL);
		m_temporaryState = NULL;
	}
	return StateMachine::updateStateMachine();
}

//----------------------------------------------------------------------------------------------------------
/**
 * Change the temporary state of the machine, and number of frames limit.th
 */
StateReturnType AIStateMachine::setTemporaryState( StateID newStateID, Int frameLimitCoount )

{
	// extract the state associated with the given ID
	State *newState = internalGetState( newStateID );
#ifdef STATE_MACHINE_DEBUG
	if (getWantsDebugOutput()) 
	{
		StateID curState = INVALID_STATE_ID;
		if (m_temporaryState) {
			curState = m_temporaryState->getID();
		}
		DEBUG_LOG(("%d '%s' -(TEMP)- '%s' %x exit ", TheGameLogic->getFrame(), getOwner()->getTemplate()->getName().str(), getName().str(), this));
		if (m_temporaryState) {
			DEBUG_LOG((" '%s' ", m_temporaryState->getName().str()));
		} else {
			DEBUG_LOG((" INVALID_STATE_ID "));
		}
		if (newState) {
			DEBUG_LOG(("enter '%s' \n", newState->getName().str()));
		} else {
			DEBUG_LOG(("to INVALID_STATE\n"));
		}
	}
#endif
	if (m_temporaryState) {
		m_temporaryState->onExit(EXIT_RESET);
		m_temporaryState = NULL;
	}
	if (newState) {
		m_temporaryState = newState;
		StateReturnType ret = m_temporaryState->onEnter();
		if (ret != STATE_CONTINUE) {
			m_temporaryState->onExit(EXIT_NORMAL);
			m_temporaryState = NULL;
			return ret;
		}
		enum {FRAME_COUNT_MAX = 60*LOGICFRAMES_PER_SECOND};
		// If you need to up this check, ok, but 1 minute seems overly long for a temporary state override.  jba.
		DEBUG_ASSERTCRASH(frameLimitCoount<=FRAME_COUNT_MAX, ("Unusually long time to set temporary state."));
		if (frameLimitCoount>FRAME_COUNT_MAX) {
			frameLimitCoount = FRAME_COUNT_MAX;
		}
		m_temporaryStateFramEnd = TheGameLogic->getFrame()+frameLimitCoount; 
		return ret;
	}
	return STATE_FAILURE;
}

//----------------------------------------------------------------------------------------------------------
/**
 * Add a point to a simple path
 */
void AIStateMachine::addToGoalPath( const Coord3D *pathPoint)
{	
	if (m_goalPath.size()==0) {
		m_goalPath.push_back(*pathPoint);
	}	else {
		Coord3D *finalPoint = &m_goalPath[ m_goalPath.size() - 1 ];
		if( !finalPoint->equals( *pathPoint ) )
		{
			m_goalPath.push_back(*pathPoint);
		}
	}
}

//----------------------------------------------------------------------------------------------------------
/**
 * Return path position at index "i"
 */
const Coord3D *AIStateMachine::getGoalPathPosition( Int i ) const
{
	if (i < 0 || i >= m_goalPath.size())
		return NULL;

	return &m_goalPath[i];
}

//----------------------------------------------------------------------------------------------------------
/**
 * Set the current goal waypoint. If we reach this waypoint and there 
 * are connections to further points, continue on.
 */
void AIStateMachine::setGoalWaypoint( const Waypoint *way )
{
	m_goalWaypoint = way;
}

//----------------------------------------------------------------------------------------------------------
/**
 * Return the current goal waypoint
 */
const Waypoint *AIStateMachine::getGoalWaypoint()
{
	return m_goalWaypoint;
}

//----------------------------------------------------------------------------------------------------------
void AIStateMachine::clear()
{
	StateMachine::clear();
	m_goalPath.clear();
	m_goalWaypoint = NULL;
	m_goalSquad = NULL;

	AIUpdateInterface* ai = getOwner()->getAI();
	if (ai)
		ai->friend_notifyStateMachineChanged();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIStateMachine::resetToDefaultState()
{
	StateReturnType tmp = StateMachine::resetToDefaultState();

	AIUpdateInterface* ai = getOwner()->getAI();
	if (ai)
		ai->friend_notifyStateMachineChanged();

	return tmp;
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIStateMachine::setState(StateID newStateID)
{
	StateID oldID = getCurrentStateID();
	StateReturnType tmp = StateMachine::setState(newStateID);

	AIUpdateInterface* ai = getOwner()->getAI();
	if (ai && oldID != newStateID)
		ai->friend_notifyStateMachineChanged();

	return tmp;
}

//----------------------------------------------------------------------------------------------------------
void AIStateMachine::setGoalTeam( const Team *team )
{
	if (m_goalSquad == NULL) {
		m_goalSquad = newInstance( Squad );
	}

	m_goalSquad->squadFromTeam(team, true);
}

//----------------------------------------------------------------------------------------------------------
void AIStateMachine::setGoalSquad( const Squad *squad )
{
	if (m_goalSquad == NULL) {
		m_goalSquad = newInstance( Squad );
	}

	(*m_goalSquad) = (*squad);
}

//----------------------------------------------------------------------------------------------------------
void AIStateMachine::setGoalAIGroup( const AIGroup *group )
{
	if (m_goalSquad == NULL) {
		m_goalSquad = newInstance( Squad );
	}

	m_goalSquad->squadFromAIGroup(group, true);
}

//----------------------------------------------------------------------------------------------------------
Squad *AIStateMachine::getGoalSquad( void )
{
	return m_goalSquad;
}

// State transition conditions ----------------------------------------------------------------------------
/**
 * Return true if the machine's owner's current weapon's range 
 * cannot reach the goalObject.
 */
Bool outOfWeaponRangeObject( State *thisState, void* userData )
{
	Object *obj = thisState->getMachineOwner();
	Object *victim = thisState->getMachineGoalObject();
	Weapon *weapon = obj->getCurrentWeapon();

	CRCDEBUG_LOG(("outOfWeaponRangeObject()\n"));
	if (victim && weapon)
	{
		Bool viewBlocked = false;
		AIUpdateInterface *ai = obj->getAI();	 
		Bool onGround = true;
		if (ai) {
			onGround = ai->isDoingGroundMovement();
		}
		if( obj->isKindOf(KINDOF_IMMOBILE) ) {
			onGround = true;
		}
		// brutal special case for stinger soldiers, who
		// generally don't have locomotors, but are still on the ground.
		if (obj->isKindOf(KINDOF_SPAWNS_ARE_THE_WEAPONS))
		{
			onGround = true;
		}
		Object *containedBy = obj->getContainedBy();
		if( containedBy && (containedBy->isKindOf( KINDOF_STRUCTURE ) || !containedBy->isAirborneTarget()) )
		{
			//Contained objects on the ground -- garrisoned buildings for example!
			onGround = true;
		}
		// srj sez: at tiny ranges, isAttackViewBlockedByObstacle() can return false positives,
		// so just skip it for contact weapons
		if (victim && !weapon->isContactWeapon() && onGround && !victim->isSignificantlyAboveTerrain()) 
		{
			viewBlocked = TheAI->pathfinder()->isAttackViewBlockedByObstacle(obj, *obj->getPosition(), victim, *victim->getPosition());
		}	 
		// A weapon with leech range temporarily has unlimited range and is locked onto its target.
		if (!weapon->hasLeechRange() && viewBlocked) 
		{
			//CRCDEBUG_LOG(("outOfWeaponRangeObject() - object %d (%s) view is blocked for attacking %d (%s)\n",
			//	obj->getID(), obj->getTemplate()->getName().str(),
			//	victim->getID(), victim->getTemplate()->getName().str()));
			return true;
		}
		if (!weapon->hasLeechRange() && !weapon->isWithinAttackRange(obj, victim))
		{
			//CRCDEBUG_LOG(("outOfWeaponRangeObject() - object %d (%s) is out of range for attacking %d (%s)\n",
			//	obj->getID(), obj->getTemplate()->getName().str(),
			//	victim->getID(), victim->getTemplate()->getName().str()));
			return true;
		}
	}

	return false;
}

static Bool inWeaponRangeObject(State *thisState, void* userData)
{
	return !outOfWeaponRangeObject(thisState, userData);
}

Bool wantToSquishTarget( State *thisState, void* userData )
{
	Object *obj = thisState->getMachineOwner();
	Object *victim = thisState->getMachineGoalObject();

	if (obj && victim)
	{
		if (victim->getContainedBy()) {
			return false; // can't crush guys in buildings or vehicles.
		}
		if( obj->getAI() && (obj->getAI()->getWhichTurretForCurWeapon() != TURRET_INVALID) )
		{
			// I can only decide to crush-attack if I am attacking with a turreted weapon.
			if (TheAI->getAiData()->m_aiCrushesInfantry) {
				if (obj && obj->getControllingPlayer() && 
					obj->getControllingPlayer()->getPlayerType()==PLAYER_COMPUTER) {
					if (obj->canCrushOrSquish(victim)) {
						if (!obj->isKindOf(KINDOF_DONT_AUTO_CRUSH_INFANTRY)) {
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

Bool outOfWeaponRangePosition( State *thisState, void* userData )
{
	Object *obj = thisState->getMachineOwner();
	const Coord3D *pos = thisState->getMachineGoalPosition();
	Weapon *weapon = obj->getCurrentWeapon();

	if (weapon && pos)
	{
		AIUpdateInterface *ai = obj->getAI();
		Bool onGround = true;
		if (ai) {
			onGround = ai->isDoingGroundMovement();
		}
		if( obj->isKindOf(KINDOF_IMMOBILE) ) {
			onGround = true;
		}
		// brutal special case for stinger soldiers, who
		// generally don't have locomotors, but are still on the ground.
		if (obj->isKindOf(KINDOF_SPAWNS_ARE_THE_WEAPONS))
		{
			onGround = true;
		}
		Object *containedBy = obj->getContainedBy();
		if( containedBy && (containedBy->isKindOf( KINDOF_STRUCTURE ) || !containedBy->isAirborneTarget()) )
		{
			//Contained objects on the ground -- garrisoned buildings for example!
			onGround = true;
		}

		Bool viewBlocked = false;
		if (onGround) 
		{
			viewBlocked = TheAI->pathfinder()->isAttackViewBlockedByObstacle(obj, *obj->getPosition(), NULL, *pos);
		}	 
		if (viewBlocked)
		{
			return true;
		}
		if (!weapon->isWithinAttackRange(obj, pos))
		{
			return true;
		}
	}

	return false;
}

/**
 * Return true if the machine's owner's cannot attack in any way
 */
static Bool cannotPossiblyAttackObject( State *thisState, void* userData )
{
	AbleToAttackType attackType = (AbleToAttackType)(uintptr_t)userData;
	Object *obj = thisState->getMachineOwner();
	Object *victim = thisState->getMachineGoalObject();

	if (obj && victim)
	{
		if( !obj->isAbleToAttack() )
		{
			return TRUE; 
		}
		CanAttackResult result = obj->getAbleToAttackSpecificObject( attackType, victim, obj->getAI()->getLastCommandSource() );
		if( result != ATTACKRESULT_POSSIBLE && result != ATTACKRESULT_POSSIBLE_AFTER_MOVING )
		{
			return TRUE;
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

const UnsignedInt IDLE_COUNTDOWN_DELAY = (LOGICFRAMES_PER_SECOND * 2);

//----------------------------------------------------------------------------------------------
AIIdleState::AIIdleState( StateMachine *machine, AIIdleState::AIIdleTargetingType shouldLookForTargets ) : 
	State( machine,"AIIdleState"), 
	m_shouldLookForTargets(shouldLookForTargets == AIIdleState::LOOK_FOR_TARGETS)
{
		m_inited = FALSE;
		m_initialSleepOffset = -1;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIIdleState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIIdleState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );
 
	xfer->xferUnsignedShort(&m_initialSleepOffset);
	xfer->xferBool(&m_shouldLookForTargets);
	xfer->xferBool(&m_inited);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIIdleState::loadPostProcess( void )
{
}  // end loadPostProcess
//----------------------------------------------------------------------------------------------
/**
 * Stake out our space.
 */
DECLARE_PERF_TIMER(AIIdleState)
StateReturnType AIIdleState::onEnter()
{
	USE_PERF_TIMER(AIIdleState)
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	// We could possibly not have ai here if we were constructed this frame. Strange but true. :-<
	if (ai) 
		ai->resetNextMoodCheckTime();

	m_inited = true;

	// reset the idle countdown so that we don't do expensive checks too often,
	// randomized so we avoid spikes
	m_initialSleepOffset = (UnsignedShort)GameLogicRandomValue(0, IDLE_COUNTDOWN_DELAY);

	// never sleep at the start, since we have to do checkGoalPos first time thru
	return STATE_CONTINUE;
}

//----------------------------------------------------------------------------------------------
void AIIdleState::doInitIdleState()
{
	// Only do it once, but do it in the update cause onEnter for idle is called during object creation.  jba.

	if (!m_inited)
		return;

	m_inited = false;

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();
	const Locomotor* loco = ai->getCurLocomotor();
	Bool ultraAccurate = (loco != NULL && loco->isUltraAccurate());
#define NO_STOP_AND_SLIDE
	if (ai->isIdle() && ai->isDoingGroundMovement()) 
	{
		/*

			You may be asking yourself, "If I'm in an idle state, how can I be doing ground movement?"

			answer from jba:

			If a unit is moving, and you hit stop, it forces it into the idle state.  
			Depending where it is, it may be between pathfind grids.  
			This is a bad thing. 
			So it "cheat moves", to the nearest grid. 
			Also, for locos the "close enough" distance is 1 or so. 
			So it moves the rest of the way to it's goal location by cheating.

		*/
		// Update the goal.
		Coord3D goalPos = *obj->getPosition();
		// but only if we have a valid position.
		if (goalPos.x || goalPos.y || goalPos.z)
		{
			TheAI->pathfinder()->updateGoal(obj, &goalPos, obj->getLayer());
			if (!ultraAccurate && TheAI->pathfinder()->goalPosition(obj, &goalPos)) 
			{
				if (TheGameLogic->getFrame()<=1) {
					obj->setPosition(&goalPos);
				} else {
#ifdef STOP_AND_SLIDE
					ai->setFinalPosition(&goalPos);
#endif
				}
				TheAI->pathfinder()->updateGoal(obj, &goalPos, obj->getLayer());
			}
		}
	}

	ai->setLocomotorGoalNone();
	ai->setCurrentVictim(NULL);	 
}

//----------------------------------------------------------------------------------------------
/**
 * Just sit there.
 */
StateReturnType AIIdleState::update()
{
	USE_PERF_TIMER(AIIdleState)

	doInitIdleState();

	UnsignedInt timeToSleep = IDLE_COUNTDOWN_DELAY + m_initialSleepOffset;
	UnsignedInt oldSleepOffset = m_initialSleepOffset;
	m_initialSleepOffset = 0;

	// This state is used internally some places, so we don't necessarily want to be looking for targets
	// Places that use AI_IDLE internally should set this to false in the constructor. jkmcd
	if ( m_shouldLookForTargets && !getMachine()->isLocked() )
	{
		// if we are here, it's time to check again
		Object *obj = getMachineOwner();
		AIUpdateInterface *ai = obj->getAI();

		// do repulsor logic
		if (obj->isKindOf(KINDOF_CAN_BE_REPULSED) && ai->isIdle()) 
		{
			Object* enemy = TheAI->findClosestRepulsor(obj, obj->getVisionRange());
			if (enemy) 
			{
				getMachine()->setState(AI_MOVE_AWAY_FROM_REPULSORS);
				// since we just changed the state, it doesn't really matter what we return here.
				return STATE_CONTINUE;
			}
		}

		// Check to see if we have created a crate we need to pick up.
		Object* crate = ai->checkForCrateToPickup();
		if (crate)
		{
			ai->aiMoveToObject(crate, CMD_FROM_AI);
			// since we just changed the state, it doesn't really matter what we return here.
			return STATE_CONTINUE;
		}

		
		if (! obj->isDisabledByType( DISABLED_PARALYZED ) &&
				! obj->isDisabledByType( DISABLED_UNMANNED ) &&
				! obj->isDisabledByType( DISABLED_EMP ) &&
				! obj->isDisabledByType( DISABLED_HACKED ) )
		{
			// mood targeting
			UnsignedInt moodAdjust = ai->getMoodMatrixActionAdjustment(MM_Action_Idle);
			if ((moodAdjust & MAA_Affect_Range_IgnoreAll) == 0)
			{
				// If we're supposed to attack based on mood, etc, then we will do so.
				Object* enemy = ai->getNextMoodTarget( true, true );
				if (enemy) 
				{
	 				ai->aiAttackObject(enemy, NO_MAX_SHOTS_LIMIT, CMD_FROM_AI);
					// weird but true. return state_continue, because if we're here, we're actually an attack state
					// since we just changed the state, it doesn't really matter what we return here.
					return STATE_CONTINUE;
				}
			}
		}

		UnsignedInt now = TheGameLogic->getFrame();
		UnsignedInt nextMoodCheckTime = ai->getNextMoodCheckTime();
		if (nextMoodCheckTime > now)
		{
			// if we need to look for targets, might need to sleep less.
			UnsignedInt moodSleep = nextMoodCheckTime - now;
			if (moodSleep < timeToSleep)
			{
				timeToSleep = moodSleep;
				// if we do this, save the random sleep offset for next time.
				m_initialSleepOffset = oldSleepOffset;
			}
		}
	}  // end if, should look for targets
	
	return STATE_SLEEP(timeToSleep);
}

//----------------------------------------------------------------------------------------------
/**
 * Just sit there, but dead-like.
 */
StateReturnType AIDeadState::onEnter()
{
	Object *obj = getMachineOwner();

	// How can an object be NULL here? I don't think it actually can, but this check must be 
	// here for a reason. - jkmcd
	if (obj)
	{
		ModelConditionFlags nonDyingStuff;
		nonDyingStuff.set(MODELCONDITION_USING_WEAPON_A);
		nonDyingStuff.set(MODELCONDITION_USING_WEAPON_B);
		nonDyingStuff.set(MODELCONDITION_USING_WEAPON_C);
		nonDyingStuff.set(MODELCONDITION_FIRING_A);
		nonDyingStuff.set(MODELCONDITION_FIRING_B);
		nonDyingStuff.set(MODELCONDITION_FIRING_C);
		nonDyingStuff.set(MODELCONDITION_BETWEEN_FIRING_SHOTS_A);
		nonDyingStuff.set(MODELCONDITION_BETWEEN_FIRING_SHOTS_B);
		nonDyingStuff.set(MODELCONDITION_BETWEEN_FIRING_SHOTS_C);
		nonDyingStuff.set(MODELCONDITION_RELOADING_A);
		nonDyingStuff.set(MODELCONDITION_RELOADING_B);
		nonDyingStuff.set(MODELCONDITION_RELOADING_C);
		nonDyingStuff.set(MODELCONDITION_PREATTACK_A);
		nonDyingStuff.set(MODELCONDITION_PREATTACK_B);
		nonDyingStuff.set(MODELCONDITION_PREATTACK_C);
#ifdef ALLOW_SURRENDER
		nonDyingStuff.set(MODELCONDITION_SURRENDER);
#endif
		nonDyingStuff.set(MODELCONDITION_MOVING);

		// dying objects are NEVER firing, surrendered, etc, so clear 'em all here.
		obj->clearAndSetModelConditionFlags(nonDyingStuff, MAKE_MODELCONDITION_MASK(MODELCONDITION_DYING));
		TheScriptEngine->notifyOfObjectCreationOrDestruction();
	}

	return STATE_CONTINUE;
}


StateReturnType AIDeadState::update()
{

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	ai->setLocomotorGoalNone();

	// re-mark this every time, just in case our health miraculously heals from 0 to nonzero...
	obj->setEffectivelyDead(true);

	if (obj->isKindOf(KINDOF_INFANTRY))
	{
		PhysicsBehavior* phys = obj->getPhysics();
		if (phys)
		{
			// we want 'em to stop, but looks wonky if they stop dead in their tracks in onEnter.
			// this slows 'em down quickly 
			const Real FACTOR = 0.8f;	// 0.8 ^ 30 == 0.012, so they slow to 4% of speed over 1 sec
			Real vel = phys->getVelocityMagnitude();
			phys->scrubVelocity2D(vel * FACTOR);
		}
	}

	return STATE_CONTINUE;
}

void AIDeadState::onExit( StateExitType status )
{
	Object *obj = getMachineOwner();
	obj->clearModelConditionState(MODELCONDITION_DYING);
}

//----------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIInternalMoveToState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIInternalMoveToState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );
 
 // extend base class
	xfer->xferCoord3D(&m_goalPosition);
	xfer->xferUser(&m_goalLayer, sizeof(m_goalLayer));
	xfer->xferBool(&m_waitingForPath);
	xfer->xferCoord3D(&m_pathGoalPosition);
	xfer->xferUnsignedInt(&m_pathTimestamp);
	xfer->xferUnsignedInt(&m_blockedRepathTimestamp);
	xfer->xferBool(&m_adjustDestinations);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIInternalMoveToState::loadPostProcess( void )
{
	startMoveSound();
}  // end loadPostProcess

Bool AIInternalMoveToState::getAdjustsDestination() const 
{ 
	const Object *obj = getMachineOwner();
	if (obj->testStatus(OBJECT_STATUS_PARACHUTING))
		return false;

	const AIUpdateInterface* ai = obj->getAI();
	if (ai && !ai->isAllowedToAdjustDestination())
		return false;

	return m_adjustDestinations; 
}

/**
 * (Re)compute a path to the goal position, if we are on our own, 
 * or we are the leader of a group.
 */
Bool AIInternalMoveToState::computePath()
{
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	m_waitingForPath = true;
	ai->requestPath(&m_goalPosition, getAdjustsDestination());
	ai->friend_startingMove(); 
	return true;
}

/**
 * We are initiating a moveTo action.
 * Pathfind from m_goalPosition to goal.
 */
StateReturnType AIInternalMoveToState::onEnter()
{
	m_ambientPlayingHandle = AHSV_Error;
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();
	m_waitingForPath = ai->isWaitingForPath();

	if (ai->getCurLocomotor()) {
		ai->getCurLocomotor()->startMove();
		if (ai->getCurLocomotor()->isUltraAccurate()) 
		{
			setAdjustsDestination(false); // if we're being ultra accurate, we can't adjust the destination. jba.
		}
	}
	m_tryOneMoreRepath = true;  // We may try one more repath after the first one is finished.
	ai->friend_startingMove(); 

	// get object physics state
	PhysicsBehavior *physics = obj->getPhysics();
	if (physics && physics->isMotive()) {
		// If we were already moving, go immediately to the animation.
		// This is so if you redirect a moving infantry or other animated model, he doesn't
		// pop into idle for 1 frame.  jba.
		//Determine if we are on a cliff cell... if so, use the climbing model condition
		//instead of moving.
		Int cellX = REAL_TO_INT( obj->getPosition()->x / PATHFIND_CELL_SIZE );
		Int cellY = REAL_TO_INT( obj->getPosition()->y / PATHFIND_CELL_SIZE );
		
		PathfindCell* cell = TheAI->pathfinder()->getCell( obj->getLayer(), cellX, cellY );
		ModelConditionFlagType modelConditionFlag = (cell && cell->getType() != PathfindCell::CELL_CLIFF) ? MODELCONDITION_MOVING : MODELCONDITION_CLIMBING;
		obj->setModelConditionState( modelConditionFlag );
	}
	// This is a classic January 2 type of hack
	// Since the worker can go into its attack modelcondition in AIAttack::OnEnter(), 
	// it needs to get its moving modelcondition set timely enough to trigger a transition 
	// from conditionX into ATTACKING|MOVING... Thanks for reading, MLorenzen,  Jan 2, 2003
	else if ( obj->isKindOf( KINDOF_DOZER ) && obj->isKindOf( KINDOF_HARVESTER ) )
		obj->setModelConditionState( MODELCONDITION_MOVING );



	if (getAdjustsDestination()) 
	{
		if (!TheAI->pathfinder()->adjustDestination(obj, ai->getLocomotorSet(), &m_goalPosition)) 
		{
			TheAI->pathfinder()->snapClosestGoalPosition(obj, &m_goalPosition);
		}
		TheAI->pathfinder()->updateGoal(obj, &m_goalPosition, TheTerrainLogic->getLayerForDestination(&m_goalPosition));
	}

	// request a path to the destination
	if (!computePath()) 
	{
		ai->friend_endingMove();
		return STATE_FAILURE;
	}

	// Target to stop at the end of this path.  
	// This value will be overriden by the FollowWaypoint ai state.
	ai->setPathExtraDistance(0); 
	ai->setDesiredSpeed( FAST_AS_POSSIBLE );

	startMoveSound();
	return STATE_CONTINUE;
}

/**
 * Start playing the object's move sound.
 */
void AIInternalMoveToState::startMoveSound(void)
{
	Object *obj = getMachineOwner();
	const BodyModuleInterface *objBody = obj->getBodyModule();
	if (objBody && IS_CONDITION_WORSE(objBody->getDamageState(), BODY_DAMAGED))
	{
		AudioEventRTS soundEventMoveDamaged = *obj->getTemplate()->getSoundMoveStartDamaged();
		if (!soundEventMoveDamaged.getEventName().isEmpty()) 
		{
			soundEventMoveDamaged.setObjectID(obj->getID());
			TheAudio->addAudioEvent( &soundEventMoveDamaged );
		} 
		else 
		{
			soundEventMoveDamaged = *obj->getTemplate()->getSoundMoveLoopDamaged();
			if (!soundEventMoveDamaged.getEventName().isEmpty())
			{
				soundEventMoveDamaged.setObjectID(obj->getID());
				m_ambientPlayingHandle = TheAudio->addAudioEvent( &soundEventMoveDamaged );
			}
		}
	} 
	else 
	{
		AudioEventRTS soundEventMove = *obj->getTemplate()->getSoundMoveStart();
		soundEventMove.setObjectID(obj->getID());

		if (!soundEventMove.getEventName().isEmpty()) 
		{
			TheAudio->addAudioEvent( &soundEventMove );
		} 
		else 
		{
			soundEventMove = *obj->getTemplate()->getSoundMoveLoop();
			soundEventMove.setObjectID(obj->getID());
			if (!soundEventMove.getEventName().isEmpty())
			{
				m_ambientPlayingHandle = TheAudio->addAudioEvent( &soundEventMove );
			}
		}
	}

}

/**
 * We are leaving the moveTo state.
 */
void AIInternalMoveToState::onExit( StateExitType status )
{
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	// stop ambient sound associated with movement
	TheAudio->removeAudioEvent( m_ambientPlayingHandle );

 	// If this onExit is the result of the state machine being deleted, then there is no AI.
	// (This is why destructors should not do game logic)
	if (ai) {
		ai->friend_endingMove();
		DEBUG_ASSERTLOG(obj->getTeam(), ("AIInternalMoveToState::onExit obj has NULL team.\n"));
		if (obj->getTeam() && ai->isDoingGroundMovement() && ai->getCurLocomotor() && 
								ai->getCurLocomotor()->isUltraAccurate()) {
			Real dx = m_goalPosition.x-obj->getPosition()->x;
			Real dy = m_goalPosition.y-obj->getPosition()->y;
			if (dx*dx+dy*dy<PATHFIND_CELL_SIZE_F*PATHFIND_CELL_SIZE_F) 
			{
				// We are doing accurate ground movement, so make sure we end exactly at the goal.
				ai->setFinalPosition(&m_goalPosition);
			}
		}
	}
}

/**
 * Execute the moveTo behavior towards GoalPosition.
 */

StateReturnType AIInternalMoveToState::update()
{

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	Path *thePath = ai->getPath();
	if (m_waitingForPath) 
	{
		// bump the timer.
		m_pathTimestamp = TheGameLogic->getFrame();
		if (ai->isWaitingForPath()) {
			/// @todo srj -- find a way to sleep for a number of frames here, if possible
			return STATE_CONTINUE;
		}
		if (thePath==NULL) {
			return STATE_FAILURE;
		}
		m_waitingForPath = false;
		m_pathGoalPosition = m_goalPosition;
		if (this->getAdjustsDestination()) {
				TheAI->pathfinder()->updateGoal(obj, thePath->getLastNode()->getPosition(), thePath->getLastNode()->getLayer());
		} else {
				TheAI->pathfinder()->removeGoal(obj);
		}
		if (!ai->getRetryPath()) {
			m_tryOneMoreRepath = false;
		}
	}

	Bool forceRecompute = false;
	if (thePath==NULL) {
		forceRecompute = true;
	}
	Bool blocked=false;
	if (ai->isBlockedAndStuck() || ai->getNumFramesBlocked()>2*LOGICFRAMES_PER_SECOND) {
		forceRecompute = true;
		blocked = true;
		m_blockedRepathTimestamp = TheGameLogic->getFrame();
		// Intense debug logging jba.
		//DEBUG_LOG(("Info - Blocked - recomputing.\n"));
	}

	//Determine if we are on a cliff cell... if so, use the climbing model condition
	//instead of moving.
	Int cellX = REAL_TO_INT( obj->getPosition()->x / PATHFIND_CELL_SIZE );
	Int cellY = REAL_TO_INT( obj->getPosition()->y / PATHFIND_CELL_SIZE );
	
	PathfindCell* cell = TheAI->pathfinder()->getCell( obj->getLayer(), cellX, cellY );
	ModelConditionFlagType setConditionFlag = MODELCONDITION_MOVING;
	// Totally hacky set of conditions to make col. burton's monkey ass not slide down 
	// the cliffs backwards.  This could use some improvement at some point.  jba. 31DEC2002
	if (cell && cell->getType() == PathfindCell::CELL_CLIFF && !cell->getPinched()) {

		if (ai->getCurLocomotor() && ai->getCurLocomotor()->isMovingBackwards()) {
			setConditionFlag = MODELCONDITION_RAPPELLING;
			obj->clearModelConditionState( MODELCONDITION_CLIMBING );
		} else {
			setConditionFlag = MODELCONDITION_CLIMBING;
			obj->clearModelConditionState( MODELCONDITION_RAPPELLING );
		}
	}
	if (ai->getNumFramesBlocked()>LOGICFRAMES_PER_SECOND/4) 
	{
		obj->clearModelConditionState( MODELCONDITION_MOVING );
	}	
	else 
	{
		//Clear climbing if modelConditionFlag is not climbing
		if( setConditionFlag == MODELCONDITION_MOVING )
		{
			obj->clearModelConditionState( MODELCONDITION_CLIMBING );
			obj->clearModelConditionState( MODELCONDITION_RAPPELLING );
		}

		obj->setModelConditionState(MODELCONDITION_MOVING);

		if (setConditionFlag!=MODELCONDITION_MOVING) {
			obj->setModelConditionState( setConditionFlag );
		}

	}

	if (ai->canComputeQuickPath()) {
		if (obj->isKindOf(KINDOF_PROJECTILE)) {
			m_pathTimestamp = 0; // We can compute paths cheap, so don't delay to recompute.  jba.
			forceRecompute = true;
		}
	}

	if (thePath != NULL) 
		ai->setLocomotorGoalPositionOnPath();

	// if our goal has moved, recompute our path
	if (forceRecompute || TheGameLogic->getFrame() - m_pathTimestamp > MIN_REPATH_TIME)
	{
		if (forceRecompute || !isSamePosition(obj->getPosition(), &m_pathGoalPosition, &m_goalPosition ))
		{
			// goal moved - repath
			if (!computePath()) 
				return STATE_FAILURE;

			// srj sez: must re-set setLocoGoal after computePath, since computePath
			// can set the loco goal to NONE...
			if (ai->getPath() != NULL) 
				ai->setLocomotorGoalPositionOnPath();
			else
				return STATE_CONTINUE;
		}
	}

	//
	// Check if we have reached our destination
	//
	Real onPathDistToGoal = ai->getLocomotorDistanceToGoal();
	//DEBUG_LOG(("onPathDistToGoal = %f %s\n",onPathDistToGoal, obj->getTemplate()->getName().str()));
	if (ai->getCurLocomotor() && (onPathDistToGoal < ai->getCurLocomotor()->getCloseEnoughDist()))
	{
		if (ai->isDoingGroundMovement()) {
			// sanity check 
			Coord3D delta;
			Coord3D goalPos = m_goalPosition;
			if (ai->getPath()->getLastNode()) {
				goalPos = *ai->getPath()->getLastNode()->getPosition();
			}
			delta.x = obj->getPosition()->x - goalPos.x;
			delta.y = obj->getPosition()->y - goalPos.y;
			delta.z = 0;
			if (delta.length() > 4*PATHFIND_CELL_SIZE_F) {
				//DEBUG_LOG(("AIInternalMoveToState Trying to finish early.  Continuing...\n"));
				onPathDistToGoal = ai->getLocomotorDistanceToGoal();
				return STATE_CONTINUE;
			}
		}
		// we have reached the end of the path
		if (getAdjustsDestination()) 
		{
			ai->setLocomotorGoalNone();
		}
		DEBUG_ASSERTLOG(!getMachine()->getWantsDebugOutput(), ("AIInternalMoveToState::update: reached end of path, exiting state with success\n"));
		return STATE_SUCCESS;
	}

	return STATE_CONTINUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------
class AIAttackMoveStateMachine : public StateMachine
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( AIAttackMoveStateMachine, "AIAttackMoveStateMachine" );

public:

	AIAttackMoveStateMachine( Object *owner, AsciiString name );

protected:
	// snapshot interface
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess();
};

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackMoveStateMachine::crc( Xfer *xfer )
{
	StateMachine::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackMoveStateMachine::xfer( Xfer *xfer )
{
	XferVersion cv = 1;	
	XferVersion v = cv; 
	xfer->xferVersion( &v, cv );

	StateMachine::xfer(xfer);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackMoveStateMachine::loadPostProcess( void )
{
	StateMachine::loadPostProcess();
}  // end loadPostProcess

//-----------------------------------------------------------------------------------------------------------
AIAttackMoveStateMachine::AIAttackMoveStateMachine(Object *owner, AsciiString name) : StateMachine(owner, name)
{
	// order matters: first state is the default state.
	defineState( AI_IDLE, newInstance(AIIdleState)( this, AIIdleState::DO_NOT_LOOK_FOR_TARGETS ), AI_IDLE, AI_IDLE );
	defineState( AI_PICK_UP_CRATE, newInstance(AIPickUpCrateState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_ATTACK_OBJECT,	newInstance(AIAttackState)(this, false, true, false, NULL ), AI_IDLE, AI_IDLE);
}

//----------------------------------------------------------------------------------------------------------
AIAttackMoveStateMachine::~AIAttackMoveStateMachine()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//  
// note - has no crc/xfer as has no member vars. jba.

//-------------------------------------------------------------------------------------------------
AIMoveToState::AIMoveToState(StateMachine *machine) : m_isMoveTo(true), AIInternalMoveToState( machine, "AIMoveToState" )
{
	// m_isMoveTo is a boolean that specifies that this thing is ACTUALLY A MOVE TO.
	// child classes should set it false. (We don't have or want RTTI, so...)
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveToState::onEnter()
{
	setAdjustsDestination(true);

	//If we have a goal object and are trying to ignore it as an obstacle...
	//This is used in the case of units trying to get really close.
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if( getMachineGoalObject() ) 
	{
		if( ai && getMachineGoalObject()->getID() == ai->getIgnoredObstacleID() ) 
		{
			setAdjustsDestination( false );
		}
	}

	// if we have a goal object, move to it, otherwise move to goal position
	if (getMachineGoalObject())	{
		m_goalPosition = *getMachineGoalObject()->getPosition();
		if (getMachineOwner()->isKindOf(KINDOF_PROJECTILE)) {
			Real halfHeight = getMachineGoalObject()->getGeometryInfo().getMaxHeightAbovePosition()/2.0f;
			m_goalPosition.z += halfHeight;
			if (getMachineGoalObject()->getPosition()->z < m_goalPosition.z) {
				m_goalPosition.z += halfHeight;
			}
		}
	} else
		m_goalPosition = *getMachineGoalPosition();

	StateReturnType ret = AIInternalMoveToState::onEnter();
	if (getMachineOwner()->getFormationID() != NO_FORMATION_ID) {
		AIGroup *group = ai->getGroup();
		if (group) {
			Real speed = group->getSpeed();
			ai->setDesiredSpeed(speed);
		}
	}
	return ret;
}

//----------------------------------------------------------------------------------------------------------
void AIMoveToState::onExit( StateExitType status )
{
	AIInternalMoveToState::onExit( status );
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveToState::update()
{
	AIUpdateInterface *ai = getMachineOwner()->getAI();

	UnsignedInt adjustment = ai->getMoodMatrixActionAdjustment(MM_Action_Move);
	if (m_isMoveTo && (adjustment & MAA_Action_To_AttackMove))
		ai->aiAttackMoveToPosition(&m_goalPosition, NO_MAX_SHOTS_LIMIT, CMD_FROM_AI);

	// if we have a goal object, move to it, as it may have moved
	Object* goalObj = getMachineGoalObject();
	Object *obj = getMachineOwner();
	if (goalObj)
	{
		m_goalPosition = *goalObj->getPosition();
		Bool gotPhysics = obj->getPhysics()!=NULL && goalObj->getPhysics()!=NULL;
		Bool isMissile = obj->isKindOf(KINDOF_PROJECTILE);
		if (isMissile) {
			Real halfHeight = getMachineGoalObject()->getGeometryInfo().getMaxHeightAbovePosition()/2.0f;
			m_goalPosition.z += halfHeight;
			Real zDelta = m_goalPosition.z - obj->getPosition()->z;
			if (zDelta>0) {
				m_goalPosition.z += zDelta;
			}
		}
		//gotPhysics = false;
		if (gotPhysics && isMissile && !goalObj->isKindOf(KINDOF_IMMOBILE)) {
			Coord3D ourPos = *obj->getPosition();
			Coord3D delta;
			delta.x = m_goalPosition.x - ourPos.x;
			delta.y = m_goalPosition.y - ourPos.y;
			delta.z = m_goalPosition.z - ourPos.z;
			Real mySpeed = obj->getPhysics()->getVelocityMagnitude();
			Real goalSpeed = goalObj->getPhysics()->getVelocityMagnitude();
			if (mySpeed<5.0f) mySpeed = 5.0f; // avoid divide by 0.
			Real leadDistance = (0.5*delta.length()) * goalSpeed / mySpeed;
			Coord3D dir;
			goalObj->getUnitDirectionVector3D(dir);
			m_goalPosition.x += dir.x*leadDistance;
			m_goalPosition.y += dir.y*leadDistance;
			m_goalPosition.z += dir.z*leadDistance;
		}
		//DEBUG_LOG(("update goal pos to %f %f %f\n",m_goalPosition.x,m_goalPosition.y,m_goalPosition.z));
	} else {
		Bool isMissile = obj->isKindOf(KINDOF_PROJECTILE);
		if (isMissile) {
			// When missiles are moving uphill, they need to start up quickly to clear hills.  jba.
			m_goalPosition = *getMachineGoalPosition();
			Real zDelta = m_goalPosition.z - obj->getPosition()->z;
			if (zDelta>0) {
				m_goalPosition.z += zDelta;
			}
		}

	}

	return AIInternalMoveToState::update();
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//  
// note - has no crc/xfer as has no member vars. jba.

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveOutOfTheWayState::onEnter()
{
	setAdjustsDestination(true);
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	if (ai->getPath()==NULL) {
		// Must have existing path.
		return STATE_FAILURE;
	}
	m_goalPosition = *ai->getPath()->getLastNode()->getPosition();
	return AIInternalMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
Bool AIMoveOutOfTheWayState::computePath() 
{
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();
	m_waitingForPath = true;
	if (ai->isBlockedAndStuck()) {
		ai->setCanPathThroughUnits(true);
		return true; // don't repath, just stop.
	}
	return true; // just use the existing path.  See above.
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveOutOfTheWayState::update()
{
	return AIInternalMoveToState::update();
}

//----------------------------------------------------------------------------------------------------------
void AIMoveOutOfTheWayState::onExit( StateExitType status )
{
	AIInternalMoveToState::onExit(status);
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (ai) {
		ai->destroyPath();
		ai->setCanPathThroughUnits(false);
		ai->clearMoveOutOfWay();
	}
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIMoveAndTightenState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIMoveAndTightenState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );
 
 // extend base class
	AIInternalMoveToState::xfer(xfer);
	xfer->xferInt(&m_okToRepathTimes);
	xfer->xferBool(&m_checkForPath);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIMoveAndTightenState::loadPostProcess( void )
{
	AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveAndTightenState::onEnter()
{
	setAdjustsDestination(false);
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();
	m_okToRepathTimes = 1;
	m_checkForPath = true;
	TheAI->pathfinder()->removeGoal(obj);
	m_goalPosition = *getMachineGoalPosition();
	ai->requestApproachPath(&m_goalPosition);
	return AIInternalMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveAndTightenState::update()
{		 
	if (m_checkForPath) {
		Object *obj = getMachineOwner();
		AIUpdateInterface *ai = obj->getAI();
		Path *thePath = ai->getPath();
		if (thePath && !ai->isWaitingForPath()) {
			setAdjustsDestination(true);
			m_checkForPath = false;
		}
	}
	return AIInternalMoveToState::update();
}

//----------------------------------------------------------------------------------------------------------
Bool AIMoveAndTightenState::computePath() 
{
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();
	if (ai->isBlockedAndStuck()) {
		if (m_okToRepathTimes>0) {
			m_okToRepathTimes--;
			m_waitingForPath = true;
			ai->requestPath(&m_goalPosition, true);
			return true;
		}
		//DEBUG_LOG(("AIMoveAndTightenState::computePath - stuck, failing.\n"));
		return false;		 // don't repath for now.  jba.
	}
	return true; // just use the existing path.  See above.
}



//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

StateReturnType AIMoveAwayFromRepulsorsState::onEnter()	  
{
	setAdjustsDestination(false);
	Object *obj = getMachineOwner();
	Object* enemy = TheAI->findClosestRepulsor(getMachineOwner(), obj->getVisionRange());
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (!enemy || !ai) {
		return STATE_FAILURE;
	}
	ai->chooseLocomotorSet(LOCOMOTORSET_PANIC);

	if (obj)
	{
		obj->setModelConditionState(MODELCONDITION_PANICKING);
	}
	m_okToRepathTimes = 1;
	m_checkForPath = true;
	TheAI->pathfinder()->removeGoal(obj);
	ai->requestSafePath(enemy->getID());
	return AIInternalMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveAwayFromRepulsorsState::update()
{		 
	if (m_checkForPath) {
		Object *obj = getMachineOwner();
		AIUpdateInterface *ai = obj->getAI();
		Path *thePath = ai->getPath();
		if (thePath && !ai->isWaitingForPath()) {
			m_goalPosition = *thePath->getLastNode()->getPosition();
			setAdjustsDestination(false);
			m_checkForPath = false;
		}
	}
	return AIInternalMoveToState::update();
}

//----------------------------------------------------------------------------------------------------------
Bool AIMoveAwayFromRepulsorsState::computePath() 
{	
	if (m_okToRepathTimes>0) {
		m_okToRepathTimes--;
		return true;
	}
	return false;  // don't recompute path, just stop moving.
}

//----------------------------------------------------------------------------------------------------------
void AIMoveAwayFromRepulsorsState::onExit( StateExitType status )
{
	AIInternalMoveToState::onExit( status );
	Object *obj = getMachineOwner();
	if (obj)
	{
		obj->clearModelConditionState(MODELCONDITION_PANICKING);
	}
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

/**
 * Returns true if we can pursue the unit.  Requires that :
 * 1. We are faster than the other unit.
 * 2. The other unit is moving.
 * 3. The other unit is moving away from us.
 */
static Bool canPursue(Object *source, Weapon *weapon, Object *victim) 
{
	/* This state is only used if the target is moving away from us, and has physics. */
	if (!victim->getPhysics()) {
		return false;
	}
	AIUpdateInterface *ai = source->getAI();
	if (!ai) {
		return false;
	}	

	// Have to have a turret to pursue.
	WhichTurretType tur = ai->getWhichTurretForCurWeapon();
	if (tur == TURRET_INVALID) {
		return false;
	}

	if (TheAI->getAiData()->m_aiCrushesInfantry) {
		if ( source->getControllingPlayer() && 
			(source->getControllingPlayer()->getPlayerType() == PLAYER_COMPUTER) &&
			source->canCrushOrSquish(victim) ) {
			return true;	// Always pursue if we can squish.
		}
	}

	if (weapon->isTooClose(source, victim)) {
		return false;		// Don't chase it if we are already too close.
	}

	Real ourMaxSpeed = source->getAI()->getCurLocomotorSpeed();

	Real victimSpeed = victim->getPhysics()->getForwardSpeed2D();
	if (victimSpeed >= ourMaxSpeed) {
		return false; // we can't catch them.
	}
	if (victimSpeed < ourMaxSpeed/10) {
		return false; // They aren't moving very fast, so don't chase.
	}
	Real dx = victim->getPosition()->x - source->getPosition()->x;
	Real dy = victim->getPosition()->y - source->getPosition()->y;
	Coord3D victimVector = *victim->getUnitDirectionVector2D();
	if (dx*victimVector.x + dy*victimVector.y < 0 ) {
		return false; // they are moving towards us.
	}
	return true;
}

//----------------------------------------------------------------------------------------------------------
/**
 * Compute a valid spot to fire our weapon from.
 * Result in m_goalPosition.
 * Return false if can't find a good spot.
 */
Bool AIAttackApproachTargetState::computePath()
{
	Bool forceRepath = false;

	// if we're immobile we can't possibly approach the target
	if( getMachineOwner()->isMobile() == false )
		return false;

	//CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - begin for object %d\n", getMachineOwner()->getID()));

	AIUpdateInterface *ai = getMachineOwner()->getAI();

	if (ai->isBlockedAndStuck()) 
	{
		forceRepath = true;
		// Intense logging. jba
		//CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - stuck, recomputing for object %d\n", getMachineOwner()->getID()));
	}
	if (m_waitingForPath) return true;

	if (!forceRepath && ai->getPath()==NULL && !ai->isWaitingForPath()) 
	{
		forceRepath = true;
	}

	// force minimum time between recomputation
	/// @todo Unify recomputation conditions & account for obj ID so everyone doesnt compute on the same frame (MSB)
	if (!forceRepath && TheGameLogic->getFrame() - m_approachTimestamp < MIN_RECOMPUTE_TIME)
	{
		//CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - bailing because of min time for object %d\n", getMachineOwner()->getID()));
		return true;
	}

	m_approachTimestamp = TheGameLogic->getFrame();

	// if we have a goal object, move to it, otherwise move to goal position
	if (getMachineGoalObject())
	{

		Object* source = getMachineOwner();
		// if our victim's position hasn't changed, don't re-path
		if (!forceRepath && isSamePosition(source->getPosition(), &m_prevVictimPos, getMachineGoalObject()->getPosition() ))
		{
			CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - bailing because victim in same place for object %d\n", getMachineOwner()->getID()));
			return true;
		}

		Weapon* weapon = source->getCurrentWeapon();
		if (!weapon) 
		{
			CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - bailing because of no weapon for object %d\n", getMachineOwner()->getID()));
			return false;
		}

		// remember where we think our victim is, so if it moves, we can re-path
		Object *victim = getMachineGoalObject();	 
		m_prevVictimPos = *victim->getPosition();
		if (canPursue(source, weapon, victim)) 
		{
			return false;  // break out, and do the pursuit state.
		}
		setAdjustsDestination(true);
		if (weapon->isContactWeapon()) 
		{
			// Weapon is basically a contact weapon, so let the attacker pathfind into the target.
			ai->ignoreObstacle(victim);
			setAdjustsDestination(false); // We want to run into the target.
			ai->setPathExtraDistance(10*PATHFIND_CELL_SIZE_F); // We don't want it to slow down.
		}

		if (ai->isBlockedAndStuck()) 
		{
			m_waitingForPath = true;
			m_goalPosition = m_prevVictimPos;
			CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - requestPath() for object %d\n", getMachineOwner()->getID()));
			ai->requestPath(&m_goalPosition, getAdjustsDestination());
		} 
		else 
		{
			m_goalPosition = m_prevVictimPos;
			m_waitingForPath = true;

			Coord3D pos;
			victim->getGeometryInfo().getCenterPosition( *victim->getPosition(), pos );
			
			CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - requestAttackPath() for object %d\n", getMachineOwner()->getID()));
			ai->requestAttackPath(victim->getID(), &pos );
			m_stopIfInRange = false; // we have calculated a position to shoot from, so go there.
		}
		CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - bailing after repathing for object %d\n", getMachineOwner()->getID()));
		return true;
	} 
	else 
	{
		// goal position.
		setAdjustsDestination(true);
		m_stopIfInRange = false; // Attack position is used by missiles, and they hit the position.
		m_goalPosition = *getMachineGoalPosition();
		if (!forceRepath) 
		{
			CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - bailing because we're aiming for a fixed position for object %d\n", getMachineOwner()->getID()));
			return true; // fixed positions don't move.
		}
		// must use computeAttackPath so that min ranges are considered.
		m_waitingForPath = true;
		ai->requestAttackPath(INVALID_ID, &m_goalPosition);
		CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - bailing after repathing at a fixed position for object %d\n", getMachineOwner()->getID()));
		return true;
	}


	CRCDEBUG_LOG(("AIAttackApproachTargetState::computePath - bailing at end of function for object %d\n", getMachineOwner()->getID()));
	return true;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackApproachTargetState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackApproachTargetState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

 // extend base class
  AIInternalMoveToState::xfer( xfer );
 
	xfer->xferCoord3D(&m_prevVictimPos);
	xfer->xferUnsignedInt(&m_approachTimestamp);
	xfer->xferBool(&m_follow);
	xfer->xferBool(&m_isAttackingObject);
	xfer->xferBool(&m_stopIfInRange);
	xfer->xferBool(&m_isInitialApproach);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackApproachTargetState::loadPostProcess( void )
{
 // extend base class
  AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackApproachTargetState::onEnter()
{
	// contained by AIAttackState, so no separate timer
	// urg. hacky. if we are a projectile, turn on precise z-pos.
	//CRCDEBUG_LOG(("AIAttackApproachTargetState::onEnter() - object %d\n", getMachineOwner()->getID()));
	Object* source = getMachineOwner();
	AIUpdateInterface* ai = source->getAI();
	if (source->isKindOf(KINDOF_PROJECTILE))
	{
		if (ai->getCurLocomotor())
			ai->getCurLocomotor()->setUsePreciseZPos(true);
	}

	if (getMachine()->isGoalObjectDestroyed()) 
	{
		return STATE_SUCCESS; // Already killed victim.
	}

	m_prevVictimPos.x = 0.0f;
	m_prevVictimPos.y = 0.0f;
	m_prevVictimPos.z = 0.0f;

	m_approachTimestamp = -MIN_RECOMPUTE_TIME;

	// See if we're close enough.
	Object *victim = getMachineGoalObject();
	if (victim) 
	{
		Weapon* weapon = source->getCurrentWeapon();
		if (!weapon) 
		{
			return STATE_FAILURE;
		}
		if (weapon->isWithinAttackRange(source, victim)) 
		{
			Bool viewBlocked = false;
			if (source && victim && ai->isDoingGroundMovement() && !victim->isSignificantlyAboveTerrain()) 
			{
				viewBlocked = TheAI->pathfinder()->isAttackViewBlockedByObstacle(source, *source->getPosition(), victim, *victim->getPosition());
			}
			if (!viewBlocked) 
			{
				return STATE_SUCCESS;
			}
		}

		// Check here:  If we are a player, and we got to this state via an ai command (ie we auto-acquired), 
		// we don't want to chase the unit. isAllowedToChase is set when we are in a deploy and attack state (troop crawler).

		if (source->getControllingPlayer()->getPlayerType() == PLAYER_HUMAN) {
			if (ai->getLastCommandSource() == CMD_FROM_AI && !ai->isAllowedToChase() ) {
				if (!weapon->isContactWeapon()) {
					return STATE_FAILURE; 
				}
			}
		}
		if (canPursue(source, weapon, victim)) 
		{
			return STATE_SUCCESS;  // break out, and do the pursuit state.
		}
	}
	// If we have a turret, start aiming.
	WhichTurretType tur = ai->getWhichTurretForCurWeapon();
	if (tur != TURRET_INVALID)
	{
		if (m_isAttackingObject)
		{
			ai->setTurretTargetObject(tur, victim, m_isForceAttacking);
		}
		else
		{
			ai->setTurretTargetPosition(tur, getMachineGoalPosition());
		}
	}

	// find a good spot to shoot from
	//CRCDEBUG_LOG(("AIAttackApproachTargetState::onEnter() - calling computePath() for object %d\n", getMachineOwner()->getID()));
	if (computePath() == false)
		return STATE_FAILURE;

	return AIInternalMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackApproachTargetState::updateInternal()
{
	AIUpdateInterface* ai = getMachineOwner()->getAI();
	//CRCDEBUG_LOG(("AIAttackApproachTargetState::updateInternal() - object %d\n", getMachineOwner()->getID()));
	if (getMachine()->isGoalObjectDestroyed()) 
	{
		ai->notifyVictimIsDead();
		ai->setCurrentVictim(NULL);
		return STATE_FAILURE;
	}
	m_stopIfInRange = !ai->isAttackPath();

	StateReturnType code = STATE_FAILURE;
 	Object* source = getMachineOwner();
	Weapon* weapon = source->getCurrentWeapon();
	Object *victim = getMachineGoalObject();
	if (victim) 
	{ 
		if( victim->testStatus( OBJECT_STATUS_STEALTHED ) && !victim->testStatus( OBJECT_STATUS_DETECTED ) ) {
			return STATE_FAILURE;	// If obj is stealthed, can no longer approach.
		}
		ai->setCurrentVictim(victim);
		// Attacking an object.
		if (weapon && weapon->isContactWeapon() && weapon->isWithinAttackRange(source, victim)) 
		{
			return STATE_SUCCESS;
		}
		if (m_stopIfInRange && weapon && weapon->isWithinAttackRange(source, victim)) 
		{
			Bool viewBlocked = false;
			if (victim && ai->isDoingGroundMovement() && !victim->isSignificantlyAboveTerrain()) 
			{
				viewBlocked = TheAI->pathfinder()->isAttackViewBlockedByObstacle(source, *source->getPosition(), victim, *victim->getPosition());
			}
			if (!viewBlocked) 
			{
				return STATE_SUCCESS;
			}
		}
		// find a good spot to shoot from
		//CRCDEBUG_LOG(("AIAttackApproachTargetState::updateInternal() - calling computePath() to victim for object %d\n", getMachineOwner()->getID()));
		if (computePath() == false)
			return STATE_SUCCESS;
		code = AIInternalMoveToState::update();


		if (code != STATE_CONTINUE) 
		{
			return STATE_SUCCESS;	// Always return state success, as state failure exits the attack.
			// we may need to aim & do another approach if the target moved.  jba.
		}
	}																			
	else 
	{
		// Attacking a position.
		// find a good spot to shoot from
		//CRCDEBUG_LOG(("AIAttackApproachTargetState::updateInternal() - calling computePath() to position for object %d\n", getMachineOwner()->getID()));
		if (m_stopIfInRange && weapon && weapon->isWithinAttackRange(source, &m_goalPosition)) 
		{
			Bool viewBlocked = false;
			if ( ai->isDoingGroundMovement() ) 
			{
				viewBlocked = TheAI->pathfinder()->isAttackViewBlockedByObstacle(source, *source->getPosition(), NULL, m_goalPosition);
			}
			if (!viewBlocked) 
			{
				return STATE_SUCCESS;
			}
		}
		if (computePath() == false)
			return STATE_FAILURE;
		code = AIInternalMoveToState::update();
	}
	return code;
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackApproachTargetState::update()
{
	// contained by AIAttackState, so no separate timer

	StateReturnType code = updateInternal();
	Object* source = getMachineOwner();
	AIUpdateInterface *ai = source->getAI();
	
	if (m_follow && m_isAttackingObject)
	{
		// Basically, if the object is alive, we continue, in case the target moves.
		Object* victim = getMachineGoalObject();
		if (source && victim && source->isMobile() && !victim->getTemplate()->isKindOf(KINDOF_IMMOBILE)) 
		{
			if (code != STATE_CONTINUE) 
			{
				m_isInitialApproach = false;
			}
			// Object is still alive (and so are we)
			// It could move (and so can we), so just continue & keep checking.
			code = STATE_CONTINUE;
		}
	}

	if (m_isInitialApproach) 
	{
		WhichTurretType tur = ai->getWhichTurretForCurWeapon();
		if (tur != TURRET_INVALID) 
		{
			Object *temporaryTarget = ai->getNextMoodTarget( true, false );
			if (temporaryTarget) 
			{
				ai->setTurretTargetObject(tur, temporaryTarget, m_isForceAttacking);
			}
		}
	}

	return code;
}

//----------------------------------------------------------------------------------------------------------
void AIAttackApproachTargetState::onExit( StateExitType status )
{
	// contained by AIAttackState, so no separate timer
	AIInternalMoveToState::onExit( status );

	AIUpdateInterface *ai = getMachineOwner()->getAI();
	Object *obj = getMachineOwner();
	if (ai) {
		ai->ignoreObstacle(NULL);
		
		// Per JohnA, this state should not be calling ai->destroyPath, because we can have spastic users
		// that click the target repeadedly. This will prevent the unit from stuttering for said spastic 
		// users.
		// ai->destroyPath();
		// urg. hacky. if we are a projectile, reset precise z-pos.
		if (getMachineOwner()->isKindOf(KINDOF_PROJECTILE))
		{
			if (ai && ai->getCurLocomotor())
				ai->getCurLocomotor()->setUsePreciseZPos(false);
		}
		if (ai->isDoingGroundMovement()) {
			Real dx = m_goalPosition.x-obj->getPosition()->x;
			Real dy = m_goalPosition.y-obj->getPosition()->y;
			if (dx*dx+dy*dy<PATHFIND_CELL_SIZE_F*PATHFIND_CELL_SIZE_F*0.125) 
			{
				// We are doing accurate ground movement, so make sure we end exactly at the goal.
				obj->setPosition(&m_goalPosition);
			}
		}
	}

	m_isInitialApproach = false;	// We only want to allow turreted things to fire at enemies during their
																// first approach
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------
/**
 * Compute a valid spot to fire our weapon from.
 * Result in m_goalPosition.
 * Return false if can't find a good spot.
 */
Bool AIAttackPursueTargetState::computePath()
{
	Bool forceRepath = false;

	// if we're immobile we can't possibly approach the target
	if( getMachineOwner()->isMobile() == false )
		return false;

	AIUpdateInterface *ai = getMachineOwner()->getAI();

	if (ai->isBlockedAndStuck()) 
	{
		return false;
	}
	if (m_waitingForPath) return true;

	if (!forceRepath && ai->getPath()==NULL && !ai->isWaitingForPath()) 
	{
		forceRepath = true;
	}

	// force minimum time between recomputation
	/// @todo Unify recomputation conditions & account for obj ID so everyone doesnt compute on the same frame (MSB)
	if (!forceRepath && TheGameLogic->getFrame() - m_approachTimestamp < MIN_RECOMPUTE_TIME)
	{
		return true;
	}

	m_approachTimestamp = TheGameLogic->getFrame();

	DEBUG_ASSERTLOG(getMachineGoalObject(), ("***************************Should only be pursuing objects.  jba"));
	// if we have a goal object, move to it, otherwise fail & continue to AIAttackApproachTargetState
	if (getMachineGoalObject())
	{

		Object* source = getMachineOwner();
		// if our victim's position hasn't changed, don't re-path
		if (!forceRepath && isSamePosition(source->getPosition(), &m_prevVictimPos, getMachineGoalObject()->getPosition() ))
			return true;

		Weapon* weapon = source->getCurrentWeapon();
		if (!weapon) 
		{
			return false;
		}
		if (!canPursue(source, weapon, getMachineGoalObject())) {
			return false;
		}

		// remember where we think our victim is, so if it moves, we can re-path
		Object *victim = getMachineGoalObject();
		m_prevVictimPos = *victim->getPosition();

		setAdjustsDestination(true);

		m_goalPosition = m_prevVictimPos;
		m_waitingForPath = true;
		ai->requestPath(&m_goalPosition, false);
		m_stopIfInRange = false; // we have calculated a position to shoot from, so go there.
		return true;
	} 

	return false;
}


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackPursueTargetState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackPursueTargetState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

 // extend base class
  AIInternalMoveToState::xfer( xfer );
 
	xfer->xferCoord3D(&m_prevVictimPos);
	xfer->xferUnsignedInt(&m_approachTimestamp);
	xfer->xferBool(&m_follow);
	xfer->xferBool(&m_isAttackingObject);
	xfer->xferBool(&m_stopIfInRange);
	xfer->xferBool(&m_isInitialApproach);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackPursueTargetState::loadPostProcess( void )
{
 // extend base class
  AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackPursueTargetState::onEnter()
{
	// contained by AIAttackState, so no separate timer
	// If we return STATE_SUCCESS or STATE_FAILURE, we proceed to AIAttackApproachTargetState.
	Object* source = getMachineOwner();
	AIUpdateInterface* ai = source->getAI();	 

	if (source->isKindOf(KINDOF_PROJECTILE))
	{
		//CRCDEBUG_LOG(("AIAttackPursueTargetState::onEnter() - is a projectile for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
		return STATE_SUCCESS;	 // Projectiles go directly to AIAttackApproachTargetState.
	}

	if (getMachine()->isGoalObjectDestroyed()) 
	{
		//CRCDEBUG_LOG(("AIAttackPursueTargetState::onEnter() - goal object is destroyed for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
		return STATE_SUCCESS; // Already killed victim.
	}
	if (!m_isAttackingObject)	{
		//CRCDEBUG_LOG(("AIAttackPursueTargetState::onEnter() - not attacking for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
		return STATE_SUCCESS; // only pursue objects - positions don't move.
	}

	setAdjustsDestination(false);
	// Check here:  If we are a player, and we got to this state via an ai command (ie we auto-acquired), 
	// we don't want to chase the unit. 
	if (source->getControllingPlayer()->getPlayerType() == PLAYER_HUMAN) {
		if (ai->getLastCommandSource() == CMD_FROM_AI) {
			return STATE_SUCCESS;
		}
	}

	m_prevVictimPos.x = 0.0f;
	m_prevVictimPos.y = 0.0f;
	m_prevVictimPos.z = 0.0f;

	m_approachTimestamp = -MIN_RECOMPUTE_TIME;

	// See if we're close enough.
	Object *victim = getMachineGoalObject();
	if (victim) {	
		Weapon* weapon = source->getCurrentWeapon();
		if (!weapon) 
		{
			return STATE_FAILURE;
		}
		if (!canPursue(source, weapon, victim) ) 
		{
			//CRCDEBUG_LOG(("AIAttackPursueTargetState::onEnter() - can't pursue for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
			return STATE_SUCCESS;
		}
	}	else {
		//CRCDEBUG_LOG(("AIAttackPursueTargetState::onEnter() - no victim for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
		return STATE_SUCCESS; // gotta have a victim.
	}
	// If we have a turret, start aiming.
	WhichTurretType tur = ai->getWhichTurretForCurWeapon();
	if (tur != TURRET_INVALID)
	{
		ai->setTurretTargetObject(tur, victim, m_isForceAttacking);
	} else {
		//CRCDEBUG_LOG(("AIAttackPursueTargetState::onEnter() - no turret for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
		return STATE_SUCCESS; // we only pursue with turrets, as non-turreted weapons can't fire on the run.
	}

	// find a good spot to shoot from
	if (computePath() == false)
		return STATE_SUCCESS;

	return AIInternalMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackPursueTargetState::updateInternal()
{
	AIUpdateInterface* ai = getMachineOwner()->getAI();	  
	if (getMachine()->isGoalObjectDestroyed()) 
	{
		ai->notifyVictimIsDead();
		ai->setCurrentVictim(NULL);
		return STATE_FAILURE;
	}
	m_stopIfInRange = false;

	Object* source = getMachineOwner();
	StateReturnType code = STATE_FAILURE;
 	Object *victim = getMachineGoalObject();
	if (victim) 
	{ 
		if( victim->testStatus( OBJECT_STATUS_STEALTHED ) && !victim->testStatus( OBJECT_STATUS_DETECTED ) ){
			return STATE_FAILURE;	// If obj is stealthed, can no longer pursue.
		}
		ai->setCurrentVictim(victim);
		// Attacking an object.
		// find a good spot to shoot from
		if (computePath() == false)
			return STATE_FAILURE;
		code = AIInternalMoveToState::update();
		if (code != STATE_CONTINUE) 
		{
			//CRCDEBUG_LOG(("AIAttackPursueTargetState::updateInternal() - failed internal update() for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
			return STATE_SUCCESS;	// Always return state success, as state failure exits the attack.
			// we may need to aim & do another approach if the target moved.  jba.
		}
		Weapon* weapon = source->getCurrentWeapon();
		if (!weapon) 
			return STATE_FAILURE;
		// If we have a turret, start aiming.
		WhichTurretType tur = ai->getWhichTurretForCurWeapon();
		if (tur == TURRET_INVALID)
		{
			//CRCDEBUG_LOG(("AIAttackPursueTargetState::updateInternal() - no turret for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
			return STATE_SUCCESS; // We currently only pursue with a turret weapon.
		}

		Bool viewBlocked = false;
		if (ai->isDoingGroundMovement() && !victim->isSignificantlyAboveTerrain()) 
		{
			viewBlocked = TheAI->pathfinder()->isAttackViewBlockedByObstacle(source, *source->getPosition(), victim, *victim->getPosition());
		}
		if (!viewBlocked && victim->getPhysics() && weapon->isWithinAttackRange(source, victim)) {
			// If we have a turret, start aiming.
			ai->setTurretTargetObject(tur, victim, m_isForceAttacking);
			//  match speeds;
			m_isInitialApproach = false;
			Real victimSpeed = victim->getPhysics()->getForwardSpeed2D();
			if (weapon->isGoalPosWithinAttackRange(source, source->getPosition(), victim, victim->getPosition())){
				victimSpeed *= 0.95f;
			}
			if (source->canCrushOrSquish(victim)) {
				victimSpeed = FAST_AS_POSSIBLE;
			}
			ai->setDesiredSpeed(victimSpeed);
			// Really intense debug info.  jba.
			// DEBUG_LOG(("VS %f, OS %f, goal %f\n", victim->getPhysics()->getForwardSpeed2D(), source->getPhysics()->getForwardSpeed2D(), victimSpeed));
		}	else {
			ai->setDesiredSpeed(FAST_AS_POSSIBLE);
		}
	}																			
	return code;
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackPursueTargetState::update()
{
	// contained by AIAttackState, so no separate timer

	StateReturnType code = updateInternal();
	Object* source = getMachineOwner();
	AIUpdateInterface *ai = source->getAI();

	if (m_isInitialApproach) 
	{
		WhichTurretType tur = ai->getWhichTurretForCurWeapon();
		if (tur != TURRET_INVALID) 
		{
			Object *temporaryTarget = ai->getNextMoodTarget( true, false );
			if (temporaryTarget) 
			{
				ai->setTurretTargetObject(tur, temporaryTarget, m_isForceAttacking);
			}
		}
	}

	return code;
}

//----------------------------------------------------------------------------------------------------------
void AIAttackPursueTargetState::onExit( StateExitType status )
{
	// contained by AIAttackState, so no separate timer
	//CRCDEBUG_LOG(("AIAttackPursueTargetState::onExit() for object %d (%s)\n", getMachineOwner()->getID(), getMachineOwner()->getTemplate()->getName().str()));
	AIInternalMoveToState::onExit( status );

	m_isInitialApproach = false;	// We only want to allow turreted things to fire at enemies during their
																// first approach
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
Bool AIPickUpCrateState::computePath()
{
	return AIInternalMoveToState::computePath();
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIPickUpCrateState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIPickUpCrateState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

 // extend base class
  AIInternalMoveToState::xfer( xfer );
 
	xfer->xferInt(&m_delayCounter);
	xfer->xferCoord3D(&m_goalPosition);

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIPickUpCrateState::loadPostProcess( void )
{
 // extend base class
  AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIPickUpCrateState::onEnter()
{
	Object* goalObj = getMachineGoalObject();
	if (!goalObj) {
		return STATE_FAILURE;
	}
	setAdjustsDestination(true);
	m_goalPosition = *goalObj->getPosition();
	m_delayCounter = 3;
	return STATE_CONTINUE;

}

//----------------------------------------------------------------------------------------------------------
void AIPickUpCrateState::onExit( StateExitType status )
{
	AIInternalMoveToState::onExit( status );

}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIPickUpCrateState::update()
{
			/// @todo srj -- find a way to sleep for a number of frames here, if possible

	if (m_delayCounter) {
		m_delayCounter--;
		if (m_delayCounter == 0) {
			return AIInternalMoveToState::onEnter();
		}
		return STATE_CONTINUE;
	}
	// do movement
	StateReturnType status = AIInternalMoveToState::update();

	return status;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIFollowPathState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIFollowPathState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIInternalMoveToState::xfer(xfer);

	xfer->xferInt(&m_index);
	xfer->xferBool(&m_adjustFinal);
	xfer->xferBool(&m_adjustFinalOverride);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIFollowPathState::loadPostProcess( void )
{
	AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIFollowPathState::onEnter()
{
	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	m_index = 0;
	const Coord3D *pos = ai->friend_getGoalPathPosition( m_index );

	if (pos == NULL)
		return STATE_FAILURE;

	// set initial movement goal
	m_goalPosition = *pos;
 	const Coord3D *nextPos = ai->friend_getGoalPathPosition( m_index+1 );
	m_adjustFinal = true;

	//Assign this value to the AIUpdateInterface so object's can access this value while
	//determine which waypoints to plot in the waypoint renderer.
	ai->friend_setCurrentGoalPathIndex( m_index ); 


 	if (getID() == AI_FOLLOW_EXITPRODUCTION_PATH) {
		ai->setCanPathThroughUnits(true);
		setAdjustsDestination(false);
		m_adjustFinal = true;
	}
	StateReturnType ret = AIInternalMoveToState::onEnter();
	if (obj->getFormationID() != NO_FORMATION_ID) {
		AIGroup *group = ai->getGroup();
		if (group) {
			Real speed = group->getSpeed();
			ai->setDesiredSpeed(speed);
		}
	}
 	if (nextPos) 
	{
		Coord2D delta;
		delta.x = nextPos->x - pos->x;
		delta.y = nextPos->y - pos->y;
		Real offset = delta.length();
 		const Coord3D *followingPos = ai->friend_getGoalPathPosition( m_index+2 );
		if (followingPos) offset += 4*PATHFIND_CELL_SIZE_F;
		ai->setPathExtraDistance(offset);
		// We are in the middle of a path, so don't set the final goal location yet.
		setAdjustsDestination(false);
	} 
	else 
	{
		setAdjustsDestination(m_adjustFinal);
		ai->setPathExtraDistance(0);

		// urg. hacky. if we are a projectile on the last segment, turn on precise z-pos.
		if (obj->isKindOf(KINDOF_PROJECTILE))
		{
			if (ai && ai->getCurLocomotor())
				ai->getCurLocomotor()->setUsePreciseZPos(true);
		}
	}	
	return ret;
}

//----------------------------------------------------------------------------------------------------------
void AIFollowPathState::onExit( StateExitType status )
{
	AIInternalMoveToState::onExit( status );

	// turn off precision-z-pos when we exit, just in case.
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (!ai) return;
	ai->setCanPathThroughUnits(false);
	if (ai->getCurLocomotor())
		ai->getCurLocomotor()->setUsePreciseZPos(false);

	//Assign this value to the AIUpdateInterface so object's can access this value while
	//determine which waypoints to plot in the waypoint renderer.
	ai->friend_setCurrentGoalPathIndex( -1 ); 
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIFollowPathState::update()
{
	getMachine()->setGoalPosition(&m_goalPosition);
	// do movement
	StateReturnType status = AIInternalMoveToState::update();
	// if move to has finished, move to next point on path
	if (status == STATE_SUCCESS || status == STATE_FAILURE)
	{
		Object *obj = getMachineOwner();
		AIUpdateInterface *ai = obj->getAI();
		if (status == STATE_FAILURE && m_retryCount>0) { 
			// If we failed, & haven't reached retry limit, try again.  jba.
			m_retryCount--;
		}	else {
			++m_index;
		}
		const Coord3D *pos = ai->friend_getGoalPathPosition( m_index );

		Bool tooClose=true;
		while (pos && tooClose) {
			Real dx = pos->x - obj->getPosition()->x;
			Real dy = pos->y - obj->getPosition()->y;
			tooClose = false;
			if (sqr(dx) + sqr(dy) < sqr(PATHFIND_CELL_SIZE_F)) {
				tooClose = true;
			}
			if (tooClose) {
				m_index++;
				pos = ai->friend_getGoalPathPosition(m_index);
			}
		}
		

		//Assign this value to the AIUpdateInterface so object's can access this value while
		//determine which waypoints to plot in the waypoint renderer.
		ai->friend_setCurrentGoalPathIndex( m_index ); 
		ai->ignoreObstacleID(INVALID_ID); // we have exited whatever object we are leaving, if any.  jba.
		if (pos == NULL)
		{
			// reached the end of the path
			return STATE_SUCCESS;
		}

		ai->friend_startingMove();
		// set next movement goal
		m_goalPosition = *pos;
 		const Coord3D *nextPos = ai->friend_getGoalPathPosition( m_index+1 );

 		if (nextPos) 
		{
			Coord2D delta;
			delta.x = nextPos->x - pos->x;
			delta.y = nextPos->y - pos->y;
			Real offset = delta.length();
 			const Coord3D *followingPos = ai->friend_getGoalPathPosition( m_index+2 );
			if (followingPos) offset += 4*PATHFIND_CELL_SIZE_F;
			ai->setPathExtraDistance(offset);
			// We are in the middle of a path, so don't set the final goal location yet.
			setAdjustsDestination(false);
		} 
		else 
		{
			setAdjustsDestination(m_adjustFinal && (m_adjustFinalOverride || ai->isDoingGroundMovement()));
			if (getAdjustsDestination()) 
			{
				if (!TheAI->pathfinder()->adjustDestination(getMachineOwner(), ai->getLocomotorSet(), &m_goalPosition)) {
					return STATE_FAILURE;
				}
				TheAI->pathfinder()->updateGoal(getMachineOwner(), &m_goalPosition, TheTerrainLogic->getLayerForDestination(&m_goalPosition));
			}

			// urg. hacky. if we are a projectile on the last segment, turn on precise z-pos.
			if (obj->isKindOf(KINDOF_PROJECTILE))
			{
				if (ai && ai->getCurLocomotor())
					ai->getCurLocomotor()->setUsePreciseZPos(true);
			}
		}
		computePath();
		return STATE_CONTINUE;
	}

	return status;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIMoveAndEvacuateState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIMoveAndEvacuateState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIInternalMoveToState::xfer(xfer);

	xfer->xferCoord3D(&m_origin);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIMoveAndEvacuateState::loadPostProcess( void )
{
	AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveAndEvacuateState::onEnter()
{
	Object *obj = getMachineOwner();

	getMachine()->lock("AIMoveAndEvacuateState::onEnter");		// This state is not user interruptable.

	m_origin = *obj->getPosition();
	setAdjustsDestination(true);

	// if we have a goal object, move to it, otherwise move to goal position
	if (getMachine()->getGoalObject())
		m_goalPosition = *getMachine()->getGoalObject()->getPosition();
	else
		m_goalPosition = *getMachine()->getGoalPosition();
	return AIInternalMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveAndEvacuateState::update()
{
	Object *obj = getMachine()->getOwner();
	if (obj->isEffectivelyDead()) 
	{
		return STATE_FAILURE;
	}

	// do movement
	StateReturnType status = AIInternalMoveToState::update();
	if (status != STATE_CONTINUE) 
	{
		Object *obj = getMachineOwner();
		if (obj->isEffectivelyDead()) 
		{
			return STATE_FAILURE;
		}

		AIUpdateInterface *ai = obj->getAI();
		ai->aiEvacuate(FALSE, CMD_FROM_AI);
		obj->getTeam()->setActive();
	}

	return status;
}

//----------------------------------------------------------------------------------------------------------
void AIMoveAndEvacuateState::onExit( StateExitType status )
{
	getMachine()->unlock();
	getMachine()->setGoalPosition(&m_origin); // In case we follow with a AIMoveAndDeleteState.
	AIInternalMoveToState::onExit( status );
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
AIAttackMoveToState::AIAttackMoveToState( StateMachine *machine ) : AIMoveToState(machine)
{
#ifdef STATE_MACHINE_DEBUG
	setName("AIAttackMoveToState");
#endif	// set up the state
	m_isMoveTo = false;
	m_frameToSleepUntil = 0;
	m_retryCount = ATTACK_RETRY_COUNT;
	m_attackMoveMachine = newInstance(AIAttackMoveStateMachine)(getMachineOwner(), "AIAttackMoveMachine");
	m_attackMoveMachine->initDefaultState();
}

//----------------------------------------------------------------------------------------------------------
AIAttackMoveToState::~AIAttackMoveToState()
{
	m_attackMoveMachine->deleteInstance();
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackMoveToState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackMoveToState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 2;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

 // extend base class
  AIMoveToState::xfer( xfer );

	if (version>=2) {
		xfer->xferUnsignedInt(&m_frameToSleepUntil);
		xfer->xferInt(&m_retryCount);
	}
	xfer->xferSnapshot(m_attackMoveMachine);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackMoveToState::loadPostProcess( void )
{
}  // end loadPostProcess

#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AIAttackMoveToState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_attackMoveMachine) name.concat(m_attackMoveMachine->getCurrentStateName());
	else name.concat("*NULL m_deployMachine");
	return name;
}
#endif

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackMoveToState::onEnter()
{
	Object *owner = getMachineOwner();
	AIUpdateInterface *ai = owner->getAI();
	m_attackMoveMachine->clear();
	m_attackMoveMachine->setState( AI_IDLE );	
	m_commandSrc = ai->getLastCommandSource();
	m_retryCount = ATTACK_RETRY_COUNT;
	m_frameToSleepUntil = 0;

	return AIMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
void AIAttackMoveToState::onExit( StateExitType status )
{
	m_attackMoveMachine->setState(AI_IDLE);
	AIMoveToState::onExit(status);
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackMoveToState::update()
{

	Object *owner = getMachineOwner();
	AIUpdateInterface *ai = owner->getAI();

	Bool forceRetargetThisFrame = false;
	Bool shouldRepathThisFrame = false;
	if (!m_attackMoveMachine->isInIdleState()) 
	{
		ai->setLocomotorGoalNone();
		owner->clearModelConditionState(MODELCONDITION_MOVING);
		m_attackMoveMachine->updateStateMachine();
		
		// if the machine is now idling, then we need to attempt to get a new target
		if (m_attackMoveMachine->isInIdleState()) {
			forceRetargetThisFrame = true;
			shouldRepathThisFrame = true;
			ai->friend_setLastCommandSource(m_commandSrc);
		} else {
			return STATE_CONTINUE;
		}
	}

	if (m_attackMoveMachine->isInIdleState())
	{

		// Check to see if we have created a crate we need to pick up.
		Object* crate = ai->checkForCrateToPickup();
		if (crate)
		{
			m_attackMoveMachine->setGoalObject(crate);
			m_attackMoveMachine->setState( AI_PICK_UP_CRATE );
			return STATE_CONTINUE;
		}
		
		Object* nextObjectToAttack;
		nextObjectToAttack = ai->getNextMoodTarget( !forceRetargetThisFrame, false );
		if (nextObjectToAttack != NULL)
		{
			ai->friend_endingMove();
			m_attackMoveMachine->setGoalObject(nextObjectToAttack);
			m_attackMoveMachine->setState( AI_ATTACK_OBJECT );
			shouldRepathThisFrame = false;	// we're about to drop out of this function, but this is semantic emphasis.
			// Note that we picked up this command from the ai.
			ai->friend_setLastCommandSource(CMD_FROM_AI);
			// we don't want an update to take place 'till next frame.
			return STATE_CONTINUE;
		}
	}

	if (m_frameToSleepUntil>TheGameLogic->getFrame()) {
		return STATE_CONTINUE;
	} else if (m_frameToSleepUntil == TheGameLogic->getFrame()) {
		shouldRepathThisFrame = true;
	}

	if (shouldRepathThisFrame) 
	{
		AIMoveToState::onEnter();
		forceRepath();
	}

	StateReturnType ret = AIMoveToState::update();
	if (ret != STATE_CONTINUE) {
		if (m_retryCount<1) return ret;
		/* check for close enough. */
		Real distSqr = sqr(owner->getPosition()->x - m_pathGoalPosition.x) + sqr(owner->getPosition()->y-m_pathGoalPosition.y);
		if (distSqr < sqr(ATTACK_CLOSE_ENOUGH_CELLS*PATHFIND_CELL_SIZE_F)) {
			return ret;
		}
		DEBUG_LOG(("AIAttackMoveToState::update Distance from goal %f, retrying.\n", sqrt(distSqr)));

		ret = STATE_CONTINUE;
		m_retryCount--;
		// Sleep 3 seconds.  We can attack during these frames, just not move.
		m_frameToSleepUntil = TheGameLogic->getFrame() + 3*LOGICFRAMES_PER_SECOND;
	}
	return ret;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIMoveAndDeleteState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIMoveAndDeleteState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIInternalMoveToState::xfer(xfer);

	xfer->xferBool(&m_appendGoalPosition);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIMoveAndDeleteState::loadPostProcess( void )
{
	AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveAndDeleteState::onEnter()
{
	setAdjustsDestination(false);
	getMachine()->lock("AIMoveAndDeleteState::onEnter");
	// if we have a goal object, move to it, otherwise move to goal position
	if (getMachine()->getGoalObject())
		m_goalPosition = *getMachine()->getGoalObject()->getPosition();
	else
		m_goalPosition = *getMachine()->getGoalPosition();
	m_appendGoalPosition = true; // We may be moving off the map.
	return AIInternalMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIMoveAndDeleteState::update()
{
	Object *obj = getMachine()->getOwner();
	if (obj->isEffectivelyDead()) 
	{
		return STATE_FAILURE;
	}
	// do movement
	AIUpdateInterface *ai = obj->getAI();
	if (ai->getCurLocomotor()) 
	{
		ai->getCurLocomotor()->setAllowInvalidPosition(true);
	}
	if (m_appendGoalPosition) 
	{
		Path *thePath = ai->getPath();
		if (!ai->isWaitingForPath() && ai->getPath()) 
		{
			m_goalPosition.z = TheTerrainLogic->getGroundHeight(m_goalPosition.x, m_goalPosition.y);
			thePath->appendNode( &m_goalPosition, LAYER_GROUND);
			m_appendGoalPosition = false; // just did it.
		}
	}
	StateReturnType status = AIInternalMoveToState::update();
	if (status != STATE_CONTINUE) 
	{
		Object *obj = getMachineOwner();
		TheGameLogic->destroyObject(obj);
	}
	return status;
}

//----------------------------------------------------------------------------------------------------------
void AIMoveAndDeleteState::onExit( StateExitType status )
{
	getMachine()->unlock();
	AIInternalMoveToState::onExit( status );
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

#define ALLOW_BACKTRACK
//----------------------------------------------------------------------------------------------------------
const Waypoint * AIFollowWaypointPathState::getNextWaypoint(void)
{
#ifdef ALLOW_BACKTRACK
	Int linkCount = m_currentWaypoint->getNumLinks();
	Int which = GameLogicRandomValue( 0, linkCount-1 );
	const Waypoint *nextWay = m_currentWaypoint->getLink( which );
	m_priorWaypoint = m_currentWaypoint;

	getMachine()->setGoalPosition(m_currentWaypoint->getLocation());// THANKS, JOHN
	return nextWay;
#else 
	if (!hasNextWaypoint()) {
		m_priorWaypoint = m_currentWaypoint;
		return NULL;
	}
	Int skip = -1;
	Int i;
	Int linkCount = m_currentWaypoint->getNumLinks();
	for (i=0; i<linkCount; i++) {
		if (m_priorWaypoint == m_currentWaypoint->getLink(i)) {
			skip = i;
			break;
		}
	}
	Int which = 0;
	if (skip >= 0) {
		which = GameLogicRandomValue( 0, linkCount-2 );
		if (which == skip) which = linkCount-1;
	}	else {
		// pick a random link
		which = GameLogicRandomValue( 0, linkCount-1 );
	}
	const Waypoint *nextWay = m_currentWaypoint->getLink( which );
	m_priorWaypoint = m_currentWaypoint;
	return nextWay;
#endif
}

//----------------------------------------------------------------------------------------------------------
Bool AIFollowWaypointPathState::hasNextWaypoint(void)
{
#ifdef ALLOW_BACKTRACK
	return m_currentWaypoint->getNumLinks()>0;
#else 
	if (m_currentWaypoint->getNumLinks()==0) {
		return false; // no links, no next.
	}
	if (m_priorWaypoint==NULL) {
		return m_currentWaypoint->getNumLinks()>0;
	}
	if (m_currentWaypoint->getNumLinks()>1) {
		// Two links, always works.
		return true;
	}
	// We have a prior waypoint, and 1 link. 
	if (m_priorWaypoint == m_currentWaypoint->getLink(0)) {
		return false; // don't go back to same waypoint.
	}
	return true;
#endif
}

//----------------------------------------------------------------------------------------------------------
Real AIFollowWaypointPathState::calcExtraPathDistance(void)
{
	Real extra = PATHFIND_CELL_SIZE_F/10.0f;
	const Waypoint *curWay = m_currentWaypoint;
	Int limit = 5; // just look ahead 5, in case of circular paths.  jba
	while (curWay && limit>0) {
		limit--;
		Int linkCount = curWay->getNumLinks();
		if (linkCount == 0) return extra;
		Int which = 0;
		const Waypoint *nextWay = curWay->getLink( which );
		Coord2D delta;
		delta.x = nextWay->getLocation()->x - curWay->getLocation()->x;
		delta.y = nextWay->getLocation()->y - curWay->getLocation()->y;
		extra += delta.length();
		curWay = nextWay;
	}
	return extra;
}

//----------------------------------------------------------------------------------------------------------
void AIFollowWaypointPathState::computeGoal(Bool useGroupOffsets)
{
	if (m_currentWaypoint == NULL)
		return;

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();
	Coord3D dest = *m_currentWaypoint->getLocation();

	m_goalLayer = LAYER_GROUND; // waypoints are always on the ground.

	if (TheAI->pathfinder()->isPointOnWall(&dest)) {
		// except when they're on the wall.  jba.
		dest.z = TheAI->pathfinder()->getWallHeight();
		m_goalLayer = LAYER_WALL; 
	}

	ai->setPathExtraDistance(calcExtraPathDistance());
	if (hasNextWaypoint()) {
		// We are in the middle of a path, so don't set the final goal location yet.
		setAdjustsDestination(false);
	} else {
		setAdjustsDestination(true);
		// urg. hacky. if we are a projectile on the last segment, turn on precise z-pos.
		if (obj->isKindOf(KINDOF_PROJECTILE))
		{
			if (ai && ai->getCurLocomotor())
				ai->getCurLocomotor()->setUsePreciseZPos(true);
		}
	}	

#define NO_ROTATE_OFFSETS
#ifdef ROTATE_OFFSETS
	Real dx = (dest.x) - (obj->getPosition()->x - m_groupOffset.x);
	Real dy = (dest.y) - (obj->getPosition()->y - m_groupOffset.y);
	Real angle;
	if (m_priorWaypoint) {
		dx = dest.x - m_priorWaypoint->getLocation()->x;
		dy = dest.y - m_priorWaypoint->getLocation()->y;
		angle = atan2(dy, dx);
		Real deltaAngle = angle - m_angle;
		Real s = sin(deltaAngle);
		Real c = cos(deltaAngle);
		Real x = m_groupOffset.x * c - m_groupOffset.y * s;
		Real y = m_groupOffset.y * c + m_groupOffset.x * s;
		m_groupOffset.x = x;
		m_groupOffset.y = y;
	}	else {
		angle = atan2(dy, dx);
	}
	m_angle = angle;
#endif
	

	m_goalPosition = dest;
	m_goalPosition.x += m_groupOffset.x;
	m_goalPosition.y += m_groupOffset.y;
	if (m_goalLayer == LAYER_WALL) {
		if (!TheAI->pathfinder()->isPointOnWall(&m_goalPosition)) {
			m_goalPosition = dest;
		}
	} else {
		m_goalPosition.z = TheTerrainLogic->getGroundHeight(m_goalPosition.x, m_goalPosition.y);
	}	
	Region3D extent;
	TheTerrainLogic->getMaximumPathfindExtent(&extent);
	if (!extent.isInRegionNoZ(&m_goalPosition)) {
		setAdjustsDestination(false); // moving off the map.
		ai->getCurLocomotor()->setAllowInvalidPosition(true); // allow it to move off the map.
		m_appendGoalPosition = true; // Moving off the map.
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIFollowWaypointPathState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIFollowWaypointPathState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIInternalMoveToState::xfer(xfer);

	xfer->xferCoord2D(&m_groupOffset);
	xfer->xferReal(&m_angle);
	xfer->xferInt(&m_framesSleeping);

	UnsignedInt id = INVALID_WAYPOINT_ID;
	if (m_currentWaypoint) {
		id = m_currentWaypoint->getID();
	}
	xfer->xferUnsignedInt(&id);
	if (xfer->getXferMode() == XFER_LOAD)
	{
		m_currentWaypoint = TheTerrainLogic->getWaypointByID(id);
	}
	id = INVALID_WAYPOINT_ID;
	if (m_priorWaypoint) {
		id = m_priorWaypoint->getID();
	}
	xfer->xferUnsignedInt(&id);
	if (xfer->getXferMode() == XFER_LOAD)
	{
		m_priorWaypoint = TheTerrainLogic->getWaypointByID(id);
	}

	xfer->xferBool(&m_appendGoalPosition);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIFollowWaypointPathState::loadPostProcess( void )
{
	AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIFollowWaypointPathState::onEnter()
{
	m_appendGoalPosition = false; // not moving off the map at this point.
	m_priorWaypoint = NULL;
	m_currentWaypoint = ((AIStateMachine *)getMachine())->getGoalWaypoint();
	AIUpdateInterface *ai = getMachineOwner()->getAI();

	if (m_currentWaypoint == NULL && !m_moveAsGroup)		return STATE_FAILURE;

	getMachine()->setGoalPosition(m_currentWaypoint->getLocation());

	m_framesSleeping = 0;
	m_groupOffset.x = m_groupOffset.y = 0;

	Object *obj = getMachineOwner();
/*	Interesting thought experiment.  Didn't work well. jba
	Real distSqrLimit = 9*obj->getGeometryInfo().getMajorRadius()*obj->getGeometryInfo().getMajorRadius();
	const Waypoint *way = m_currentWaypoint;
	Bool doPrecise = false;
	while (way) {
		if (way->getNext()) {
			Real dx = way->getLocation()->x - way->getNext()->getLocation()->x;
			Real dy = way->getLocation()->y - way->getNext()->getLocation()->y;
			Real distSqr = dx*dx + dy*dy;
			if (distSqr < distSqrLimit) {
				doPrecise = true;
			}
		}
		way = way->getNext();
	}
	if (doPrecise && ai->getCurLocomotor()) {
		//ai->getCurLocomotor()->setUltraAccurate(true);
	}
*/


	Real speed = FAST_AS_POSSIBLE;
	if (m_moveAsGroup && m_currentWaypoint) {
		obj->getTeam()->setCurrentWaypoint(m_currentWaypoint);
		AIGroup *group = ai->getGroup();
		if (group) {
			speed = group->getSpeed();
			Coord3D center;
			
			group->getCenter( &center );
			m_groupOffset.x = obj->getPosition()->x - center.x;
			m_groupOffset.y = obj->getPosition()->y - center.y;
		}
	}
	if (m_currentWaypoint==NULL && m_moveAsGroup) {
		m_currentWaypoint = obj->getTeam()->getCurrentWaypoint();
	}
	// set initial movement goal
	computeGoal(m_moveAsGroup);
	StateReturnType ret = AIInternalMoveToState::onEnter();

	ai->setDesiredSpeed(speed);

	// Update the extra path distance.   AIInternalMoveToState::onEnter resets it.
	ai->setPathExtraDistance(calcExtraPathDistance());
	if (hasNextWaypoint()) {
		// We are in the middle of a path, so don't set the final goal location yet.
		setAdjustsDestination(false);
	} else {
		setAdjustsDestination(ai->isDoingGroundMovement());
		if (getAdjustsDestination()) {
			if (!TheAI->pathfinder()->adjustDestination(getMachineOwner(), ai->getLocomotorSet(), &m_goalPosition)) {
				DEBUG_LOG(("Breaking out of follow waypoint path\n"));
				return STATE_FAILURE;
			}
			TheAI->pathfinder()->updateGoal(getMachineOwner(), &m_goalPosition, m_goalLayer);
		}
		// urg. hacky. if we are a projectile on the last segment, turn on precise z-pos.
		if (obj->isKindOf(KINDOF_PROJECTILE))
		{
			if (ai && ai->getCurLocomotor())
				ai->getCurLocomotor()->setUsePreciseZPos(true);
		}
	}
	if (ret != STATE_CONTINUE) {
		DEBUG_LOG(("Breaking out of follow waypoint path\n"));
	}
	return ret;
}

//----------------------------------------------------------------------------------------------------------
void AIFollowWaypointPathState::onExit( StateExitType status )
{
	AIInternalMoveToState::onExit( status );

	// turn off precision-z-pos when we exit, just in case.
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (ai && ai->getCurLocomotor()) {
		ai->getCurLocomotor()->setUsePreciseZPos(false);
		ai->getCurLocomotor()->setUltraAccurate(false);
	}
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIFollowWaypointPathState::update()
{
	if (m_framesSleeping>0) {
		m_framesSleeping--;
		return STATE_CONTINUE;
	}
	Object *obj = getMachineOwner();
 	AIUpdateInterface *ai = obj->getAI();


	getMachine()->setGoalPosition(m_currentWaypoint->getLocation());
	

	UnsignedInt adjustment = ai->getMoodMatrixActionAdjustment(MM_Action_Move);
	if (m_isFollowWaypointPathState && (adjustment & MAA_Action_To_AttackMove))	{
		if (m_moveAsGroup) {
			ai->aiAttackFollowWaypointPathAsTeam(m_currentWaypoint, NO_MAX_SHOTS_LIMIT, CMD_FROM_AI);
		}	else {
			ai->aiAttackFollowWaypointPath(m_currentWaypoint, NO_MAX_SHOTS_LIMIT, CMD_FROM_AI);
		}
	}

	if (m_appendGoalPosition) {	 
		Path *thePath = ai->getPath();
		if (!ai->isWaitingForPath() && ai->getPath()) {
			//Coord3D pathEnd = *thePath->getLastNode()->getPosition();
			thePath->appendNode(&m_goalPosition, LAYER_GROUND);	// waypoints are always on the ground.
			m_appendGoalPosition = false; // just did it.
		}
	}
	if (m_moveAsGroup && m_currentWaypoint != obj->getTeam()->getCurrentWaypoint()) {
		m_priorWaypoint = m_currentWaypoint;
		m_currentWaypoint = obj->getTeam()->getCurrentWaypoint();
		if (m_currentWaypoint == NULL) {
			return STATE_SUCCESS;
		}			 
		computeGoal(false);
		if (getAdjustsDestination() && ai->isDoingGroundMovement()) {
			if (!TheAI->pathfinder()->adjustDestination(obj, ai->getLocomotorSet(), &m_goalPosition)) {
				if (m_currentWaypoint) {
					DEBUG_LOG(("Breaking out of follow waypoint path %s of %s\n", 
					m_currentWaypoint->getName().str(), m_currentWaypoint->getPathLabel1().str()));
				}
				return STATE_FAILURE;
			}
		}
		ai->friend_startingMove();
		computePath();
		if (getAdjustsDestination()) {
			TheAI->pathfinder()->updateGoal(obj, &m_goalPosition, m_goalLayer);
		}
	}

	// do movement
	StateReturnType status = AIInternalMoveToState::update();
	
	// We may want to allow ourselves to bail out of this one early. In order to do this, we check and
	// see if we're moving as a group, and then if our team is owned by an AI Skirmish player. 
	// If it is, then we compute the group centroid, and see if it is within some distance of the 
	if (m_moveAsGroup) {
		if (obj->getControllingPlayer()->isSkirmishAIPlayer()) {
			Team *team = obj->getTeam();
			AIGroup *group = TheAI->createGroup();
			team->getTeamAsAIGroup(group);

			Coord3D pos;
			group->getCenter(&pos);

			pos.x -= m_goalPosition.x;
			pos.y -= m_goalPosition.y;
			pos.z = 0;

			Int numInGroup = group->getCount();
			if (pos.length() <= (numInGroup * TheAI->getAiData()->m_skirmishGroupFudgeValue)) {
				// Consider ourselves close enough.
				status = STATE_SUCCESS;
			}
		}
	}

	// if move to has finished, move to next point on waypoint path
	if (status != STATE_CONTINUE)
	{
		m_currentWaypoint = getNextWaypoint();

		//LORENZEN ADDED LORENZEN ADDED LORENZEN ADDED 
		Object *obj = getMachineOwner();
		AIUpdateInterface *ai = obj->getAI();
		if ( m_priorWaypoint )
			ai->setPriorWaypointID( m_priorWaypoint->getID() );
		if ( m_currentWaypoint )
			ai->setCurrentWaypointID( m_currentWaypoint->getID() );
		//LORENZEN ADDED LORENZEN ADDED LORENZEN ADDED 



		// if there are no links from this waypoint, we're done
		if (m_currentWaypoint==NULL)	{
			/// Trigger "end of waypoint path" scripts (jba)
			ai->setCompletedWaypoint(m_priorWaypoint);			
			return STATE_SUCCESS;
		}
		if (m_moveAsGroup) {
			obj->getTeam()->setCurrentWaypoint(m_currentWaypoint);
		} 
		
		computeGoal(false);
		if (getAdjustsDestination() && ai->isDoingGroundMovement()) {
			if (!TheAI->pathfinder()->adjustDestination(obj, ai->getLocomotorSet(), &m_goalPosition)) {
				if (m_currentWaypoint) {
					DEBUG_LOG(("Breaking out of follow waypoint path %s of %s\n", 
					m_currentWaypoint->getName().str(), m_currentWaypoint->getPathLabel1().str()));
				}
				return STATE_FAILURE;
			}
		}
		ai->friend_startingMove();
		computePath();
		if (getAdjustsDestination()) {
			TheAI->pathfinder()->updateGoal(obj, &m_goalPosition, m_goalLayer);
		}

		return STATE_CONTINUE;
	}
	if (status != STATE_CONTINUE) {
		DEBUG_LOG(("Breaking out of follow waypoint path\n"));
	}
	return status;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIFollowWaypointPathExactState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIFollowWaypointPathExactState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIInternalMoveToState::xfer(xfer);
	UnsignedInt id = INVALID_WAYPOINT_ID;
	if (m_lastWaypoint) {
		id = m_lastWaypoint->getID();
	}
	xfer->xferUnsignedInt(&id);
	if (xfer->getXferMode() == XFER_LOAD)
	{
		m_lastWaypoint = TheTerrainLogic->getWaypointByID(id);
	}
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIFollowWaypointPathExactState::loadPostProcess( void )
{
	AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIFollowWaypointPathExactState::onEnter()
{
	const Waypoint *currentWaypoint = ((AIStateMachine *)getMachine())->getGoalWaypoint();
	AIUpdateInterface *ai = getMachineOwner()->getAI();

	if (currentWaypoint == NULL) return STATE_FAILURE;

	getMachine()->setGoalPosition(currentWaypoint->getLocation());

	Coord2D groupOffset;
	groupOffset.x = groupOffset.y = 0;

	Object *obj = getMachineOwner();

	Real speed = FAST_AS_POSSIBLE;
	if (m_moveAsGroup) {
		AIGroup *group = ai->getGroup();
		if (group) {
			speed = group->getSpeed();
			Coord3D center;
			
			group->getCenter( &center );
			groupOffset.x = obj->getPosition()->x - center.x;
			groupOffset.y = obj->getPosition()->y - center.y;
		}
	}
	ai->setCanPathThroughUnits(true);
	setAdjustsDestination(false);
	// set initial movement goal
	StateReturnType ret = AIInternalMoveToState::onEnter();
	ai->setPathFromWaypoint(currentWaypoint, &groupOffset);	
	m_lastWaypoint = currentWaypoint;
	ai->getCurLocomotor()->setAllowInvalidPosition(true); // allow it to move off the map.

	//Kris: October 4, 2002 -- Commented out by guidance of John A.
	//			Artist couldn't load his map, and turned out that it was because
	//			there was a waypoint path that pointed to itself (2 point path).
	//			John said that this code only needs "a" point, not the "last" point.
	//while (m_lastWaypoint && m_lastWaypoint->getLink(0)) {
	//	m_lastWaypoint = m_lastWaypoint->getLink(0);
	//}
	ai->setDesiredSpeed(speed);

	return ret;
}

//----------------------------------------------------------------------------------------------------------
void AIFollowWaypointPathExactState::onExit( StateExitType status )
{
	AIInternalMoveToState::onExit( status );

	// turn off precision-z-pos when we exit, just in case.
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (ai) {
		ai->setCompletedWaypoint(m_lastWaypoint);			
		ai->setCanPathThroughUnits(false);
		ai->getCurLocomotor()->setAllowInvalidPosition(false); // turn off allow it to move off the map.
	}
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIFollowWaypointPathExactState::update()
{

	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (ai) ai->setCanPathThroughUnits(true);
	// do movement
	StateReturnType status = AIInternalMoveToState::update();

	return status;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
AIAttackFollowWaypointPathState::AIAttackFollowWaypointPathState ( StateMachine *machine, Bool asGroup ) : 
AIFollowWaypointPathState ( machine, asGroup, false ) 
{
#ifdef STATE_MACHINE_DEBUG
	setName("AIAttackFollowWaypointPathState");
#endif	
	m_attackFollowMachine = newInstance(AIAttackMoveStateMachine)(getMachineOwner(), "AIAttackFollowMachine");
	m_attackFollowMachine->initDefaultState();
}

//-------------------------------------------------------------------------------------------------
AIAttackFollowWaypointPathState::~AIAttackFollowWaypointPathState()
{
	m_attackFollowMachine->deleteInstance();
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackFollowWaypointPathState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackFollowWaypointPathState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

 // extend base class
  AIFollowWaypointPathState::xfer( xfer );
 
	xfer->xferSnapshot(m_attackFollowMachine);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackFollowWaypointPathState::loadPostProcess( void )
{
}  // end loadPostProcess

#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AIAttackFollowWaypointPathState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_attackFollowMachine) name.concat(m_attackFollowMachine->getCurrentStateName());
	else name.concat("*NULL m_attackFollowMachine");
	return name;
}
#endif

//-------------------------------------------------------------------------------------------------
StateReturnType AIAttackFollowWaypointPathState ::onEnter()
{
	m_attackFollowMachine->clear();
	m_attackFollowMachine->setState( AI_IDLE );	

	return AIFollowWaypointPathState::onEnter();
}

//-------------------------------------------------------------------------------------------------
StateReturnType AIAttackFollowWaypointPathState::update()
{

	Object *owner = getMachineOwner();
	AIUpdateInterface *ai = owner->getAI();

	Bool forceRetargetThisFrame = false;
	Bool shouldRepathThisFrame = false;
	if (!m_attackFollowMachine->isInIdleState()) 
	{
		ai->setLocomotorGoalNone();
		owner->clearModelConditionState(MODELCONDITION_MOVING);
		m_attackFollowMachine->updateStateMachine();
		
		// if the machine is now idling, then we need to attempt to get a new target
		if (m_attackFollowMachine->isInIdleState()) 
		{
			forceRetargetThisFrame = true;
			shouldRepathThisFrame = true;
		} 
		else 
		{
			return STATE_CONTINUE;
		}
	}

	if (m_attackFollowMachine->isInIdleState())
	{
		// Check to see if we have created a crate we need to pick up.
		Object* crate = ai->checkForCrateToPickup();
		if (crate)
		{
			m_attackFollowMachine->setGoalObject(crate);
			m_attackFollowMachine->setState( AI_PICK_UP_CRATE );
			return STATE_CONTINUE;
		}

		Object* nextObjectToAttack;
		{

			nextObjectToAttack = ai->getNextMoodTarget( !forceRetargetThisFrame, false );
		}
		if (nextObjectToAttack != NULL)
		{
			m_attackFollowMachine->setGoalObject(nextObjectToAttack);
			m_attackFollowMachine->setState( AI_ATTACK_OBJECT );
			shouldRepathThisFrame = false;	// we're about to drop out of this function, but this is semantic emphasis.

			// we don't want an update to take place 'till next frame.
			return STATE_CONTINUE;
		}
	}

	if (shouldRepathThisFrame) 
	{
		// Update the goal waypoint if we've moved along the path.
		computeGoal(m_moveAsGroup);
		computePath();
	}

	return AIFollowWaypointPathState::update();
}

//-------------------------------------------------------------------------------------------------
void AIAttackFollowWaypointPathState ::onExit( StateExitType status )
{
	m_attackFollowMachine->setState(AI_IDLE);
	AIFollowWaypointPathState::onExit(status);
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
/**
 * Wander along a waypoint path.
 */

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIWanderState::crc( Xfer *xfer )
{
	AIFollowWaypointPathState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIWanderState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIFollowWaypointPathState::xfer(xfer);

	xfer->xferInt(&m_waitFrames);
	xfer->xferInt(&m_timer);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIWanderState::loadPostProcess( void )
{
	AIFollowWaypointPathState::loadPostProcess();
}  // end loadPostProcess

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
StateReturnType AIWanderState::onEnter()
{
	m_currentWaypoint = ((AIStateMachine *)getMachine())->getGoalWaypoint();

	AIUpdateInterface *ai = getMachineOwner()->getAI();
	m_priorWaypoint = NULL;
	if (m_currentWaypoint == NULL || ai==NULL)
		return STATE_FAILURE;
	m_groupOffset.x = m_groupOffset.y = 0;
	Locomotor* curLoco = ai->getCurLocomotor();
	if (curLoco && curLoco->getWanderWidthFactor() > 0.0f) {
		Int delta = REAL_TO_INT_FLOOR(curLoco->getWanderWidthFactor()+0.5f);
		if (delta<1) delta = 1;
		m_groupOffset.x = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
		m_groupOffset.y = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
	}
	m_timer = 0;
	m_waitFrames = 10 + (getMachineOwner()->getID() & 0x7);
	// set initial movement goal
	computeGoal(false);
	StateReturnType ret = AIInternalMoveToState::onEnter();
	ai->setPathExtraDistance(calcExtraPathDistance());
	return ret;
}


//----------------------------------------------------------------------------------------------------------
StateReturnType AIWanderState::update()
{
	// do movement
	Object *obj = getMachineOwner();
	StateReturnType status = AIInternalMoveToState::update();
	if (obj->isKindOf(KINDOF_CAN_BE_REPULSED)) {
		m_timer--;
		if (m_timer<0) {
			m_timer = m_waitFrames;
			Object* enemy = TheAI->findClosestRepulsor(getMachineOwner(), obj->getVisionRange());
			if (enemy) {
				return STATE_FAILURE;
			}
		}
	}
	// if move to has finished, move to next point on waypoint path
	if (status != STATE_CONTINUE)
	{
		AIUpdateInterface *ai = obj->getAI();

		m_currentWaypoint = getNextWaypoint();
		// if there are no links from this waypoint, we're done
		if (m_currentWaypoint == NULL)	{
			/// Trigger "end of waypoint path" scripts (jba)
			ai->setCompletedWaypoint(m_priorWaypoint);
			
			return STATE_SUCCESS;
		}

		Locomotor* curLoco = ai->getCurLocomotor();
		if (curLoco && curLoco->getWanderWidthFactor() > 0.0f) {
			Int delta = REAL_TO_INT_FLOOR(curLoco->getWanderWidthFactor()+0.5f);
			if (delta<1) delta = 1;
			m_groupOffset.x = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
			m_groupOffset.y = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
		}
		computeGoal(false);
		computePath();
		return STATE_CONTINUE;
	}
	// Never leave this state until told to.
	return STATE_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------
void AIWanderState::onExit( StateExitType status )
{
	AIFollowWaypointPathState::onExit( status );
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
/**
 * Wander around a point.
 */

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIWanderInPlaceState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIWanderInPlaceState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIInternalMoveToState::xfer(xfer);

	xfer->xferCoord3D(&m_origin);
	xfer->xferInt(&m_waitFrames);
	xfer->xferInt(&m_timer);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** LoadPostProcess */
// ------------------------------------------------------------------------------------------------
void AIWanderInPlaceState::loadPostProcess( void )
{
	AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

// ------------------------------------------------------------------------------------------------
StateReturnType AIWanderInPlaceState::onEnter()
{
	m_origin = *getMachineOwner()->getPosition();

	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (ai) {
		ai->chooseLocomotorSet(LOCOMOTORSET_WANDER);
	}

	Int delta = 3;
	if (ai->getCurLocomotor()) {
		delta = REAL_TO_INT_FLOOR( (ai->getCurLocomotor()->getWanderAboutPointRadius()/PATHFIND_CELL_SIZE_F) + 0.5f);
	}
	Coord3D offset;
	offset.x = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
	offset.y = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
	m_goalPosition = m_origin;
	m_goalPosition.x += offset.x;
	m_goalPosition.y += offset.y;
	m_timer = 0;
	m_waitFrames = 10 + (getMachineOwner()->getID() & 0x7);
	StateReturnType ret = AIInternalMoveToState::onEnter();
	return ret;
}


//----------------------------------------------------------------------------------------------------------
StateReturnType AIWanderInPlaceState::update()
{
	// do movement
	StateReturnType status = AIInternalMoveToState::update();

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (!ai) return STATE_FAILURE;
	if (obj->isKindOf(KINDOF_CAN_BE_REPULSED)) {
		m_timer--;
		if (m_timer<0) {
			m_timer = m_waitFrames;
			Object* enemy = TheAI->findClosestRepulsor(getMachineOwner(), obj->getVisionRange());
			if (enemy) {
				return STATE_FAILURE;
			}
		}
	}
	// if move to has finished, move to next point on waypoint path
	if (status != STATE_CONTINUE)
	{
		Int delta = 3;
		if (ai->getCurLocomotor()) {
			delta = REAL_TO_INT_FLOOR( (ai->getCurLocomotor()->getWanderAboutPointRadius()/PATHFIND_CELL_SIZE_F) + 0.5f);
		}
		Coord3D offset;
		offset.x = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
		offset.y = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
		m_goalPosition = m_origin;
		m_goalPosition.x += offset.x;
		m_goalPosition.y += offset.y;
		AIInternalMoveToState::onEnter();
		return STATE_CONTINUE;
	}
	// Never leave this state until told to.
	return STATE_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------
void AIWanderInPlaceState::onExit( StateExitType status )
{
	AIInternalMoveToState::onExit( status );
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIPanicState::crc( Xfer *xfer )
{
	AIFollowWaypointPathState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIPanicState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIFollowWaypointPathState::xfer(xfer);

	xfer->xferInt(&m_waitFrames);
	xfer->xferInt(&m_timer);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIPanicState::loadPostProcess( void )
{
	AIFollowWaypointPathState::loadPostProcess();
}  // end loadPostProcess

/**
 * Panic and run screaming along a waypoint path.
 */
StateReturnType AIPanicState::onEnter()
{
	m_currentWaypoint = ((AIStateMachine *)getMachine())->getGoalWaypoint();

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();
	if (m_currentWaypoint == NULL)
		return STATE_FAILURE;
	// set initial movement goal
	Locomotor* curLoco = ai->getCurLocomotor();
	if (curLoco && curLoco->getWanderWidthFactor() > 0.0f) {
		Int delta = REAL_TO_INT_FLOOR(curLoco->getWanderWidthFactor()+0.5f);
		if (delta<1) delta = 1;
		m_groupOffset.x = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
		m_groupOffset.y = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
	}
	computeGoal(false);
	StateReturnType ret = AIInternalMoveToState::onEnter();

	m_timer = 0;
	m_waitFrames = 10 + (getMachineOwner()->getID() & 0x7);
	// Update the extra path distance.   AIInternalMoveToState::onEnter resets it.
	ai->setPathExtraDistance(calcExtraPathDistance());
	if (obj)
	{
		obj->setModelConditionState(MODELCONDITION_PANICKING);
	}

	return ret;
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIPanicState::update()
{
	// do movement
	StateReturnType status = AIInternalMoveToState::update();

	Object *obj = getMachineOwner();
	if (obj->isKindOf(KINDOF_CAN_BE_REPULSED)) {
		m_timer--;
		if (m_timer<0) {
			m_timer = m_waitFrames;
			Object* enemy = TheAI->findClosestRepulsor(getMachineOwner(), obj->getVisionRange());
			if (enemy) {
				return STATE_FAILURE;
			}
		}
	}

	// if move to has finished, move to next point on waypoint path
	if (status == STATE_SUCCESS)
	{
		Object *obj = getMachineOwner();
		AIUpdateInterface *ai = obj->getAI();

		m_currentWaypoint = getNextWaypoint();
		// if there are no links from this waypoint, we're done
		if (m_currentWaypoint == NULL)	{
			/// Trigger "end of waypoint path" scripts (jba)
			ai->setCompletedWaypoint(m_priorWaypoint);
			
			return STATE_SUCCESS;
		}
		Locomotor* curLoco = ai->getCurLocomotor();
		if (curLoco && curLoco->getWanderWidthFactor() > 0.0f) {
			Int delta = REAL_TO_INT_FLOOR(curLoco->getWanderWidthFactor()+0.5f);
			if (delta<1) delta = 1;
			m_groupOffset.x = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
			m_groupOffset.y = GameLogicRandomValue(-delta, delta)*PATHFIND_CELL_SIZE_F;
		}
		computeGoal(false);
		computePath();
		return STATE_CONTINUE;
	}
	// Never leave this state until told to.
	return STATE_CONTINUE;
}


//----------------------------------------------------------------------------------------------------------
void AIPanicState::onExit( StateExitType status )
{
	Object *obj = getMachineOwner();
	obj->clearModelConditionState(MODELCONDITION_PANICKING);
	AIInternalMoveToState::onExit( status );
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackAimAtTargetState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackAimAtTargetState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferBool(&m_canTurnInPlace);
	xfer->xferBool(&m_setLocomotor);

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackAimAtTargetState::loadPostProcess( void )
{
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackAimAtTargetState::onEnter()
{
	// contained by AIAttackState, so no separate timer
	Object* source = getMachineOwner();
	Weapon* weapon = source->getCurrentWeapon();
	Object* victim = getMachineGoalObject();
	const Coord3D* targetPos = getMachineGoalPosition();
	AIUpdateInterface* sourceAI = source->getAI();
	AIUpdateInterface* victimAI = victim ? victim->getAI() : NULL;

	Locomotor* curLoco = sourceAI->getCurLocomotor();
	m_canTurnInPlace = curLoco ? curLoco->getMinSpeed() == 0.0f : false;

//	if (!victim) 
//		return STATE_CONTINUE; // Just continue till we get a victim.
// Ick.  This was originally a safety to a single line that required victim, and was never meant
// as an early return to all cases.  We now want to use preattack frames on ground position targets

	Bool inFiringRange = FALSE;

	//If the object is garrisoning a building, then we want to force the object
	//to move to the best fire point, check if it's in firing range, and if not
	//move it back so another unit with longer range can!
	Object *containedBy = source->getContainedBy();
	if( containedBy && weapon )
	{
		ContainModuleInterface *contain = containedBy->getContain();
		if (victim)
		{
			inFiringRange = contain->attemptBestFirePointPosition( source, weapon, victim );
		}
		else
		{
			inFiringRange = contain->attemptBestFirePointPosition( source, weapon, targetPos );
		}
	}
	else if( victim && weapon )
		inFiringRange = weapon->isWithinAttackRange( source, victim );
	else if( weapon )
		inFiringRange = weapon->isWithinAttackRange( source, targetPos );//See, I can be attacking the ground.  Perfectly valid.
	else
		return STATE_FAILURE; // can't happen.

	// add ourself as a targeter BEFORE calling isTemporarilyPreventingAimSuccess().
	if (victimAI)
		victimAI->addTargeter(source->getID(), true);
	
	WhichTurretType tur = sourceAI->getWhichTurretForCurWeapon();
	if (tur != TURRET_INVALID)
	{
		if (m_isAttackingObject)
		{
			sourceAI->setTurretTargetObject(tur, victim, m_isForceAttacking);
		}
		else
		{
			sourceAI->setTurretTargetPosition(tur, getMachineGoalPosition());
		}
	}
	else
	{
		// GS moved contact weapon check in here, because Success can never be given to a unit in this state 
		// using a turret to attack.  Check out ::update and you will see.

		Bool preventing = victimAI && victimAI->isTemporarilyPreventingAimSuccess();

		// Contact weapons don't aim.  They just go boom.  jba.
		if( weapon && weapon->isContactWeapon() && inFiringRange && !preventing ) 
			return STATE_SUCCESS;
	}
	m_setLocomotor = false;

	source->setStatus( OBJECT_STATUS_IS_AIMING_WEAPON, true );
	return STATE_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------
/**
 * Orient the machine's owner to face towards the given target
 */

StateReturnType AIAttackAimAtTargetState::update()
{
	// contained by AIAttackState, so no separate timer
	Object* source = getMachineOwner();
	AIUpdateInterface* sourceAI = source->getAI();

	if (!source->hasAnyWeapon())
		return STATE_FAILURE;

	Object* victim = getMachineGoalObject();
	if (m_isAttackingObject)
	{
		if (!victim || victim->isEffectivelyDead())
			return STATE_FAILURE;	// can't aim at dead things
	}

	WhichTurretType tur = sourceAI->getWhichTurretForCurWeapon();
	if (tur != TURRET_INVALID)
	{
		if (m_isAttackingObject)
		{
			sourceAI->setTurretTargetObject(tur, victim, m_isForceAttacking);
		}
		else
		{
			sourceAI->setTurretTargetPosition(tur, getMachineGoalPosition());
		}
		// if we have a turret, but it is incapable of turning, turn ourself.
		// (gotta do this for units like the Comanche, which have fake "turrets"
		// solely to allow for attacking-on-the-move...)
		if (sourceAI->getTurretTurnRate(tur) != 0.0f)	
		{
			// The Body can never return Success if the weapon is on the turret, or else we end
			// up shooting the current weapon (which is on the turret) in the wrong direction.
			// We always say Continue, so the Turret can do its own Aiming state.
//			if (m_isAttackingObject && source->canCrushOrSquish(victim)) {
//				return STATE_SUCCESS;
//			}
			return STATE_CONTINUE;
		}

		// else fall thru!
	}
	
	// no else here!
	{
		Real relAngle = m_isAttackingObject ?
											ThePartitionManager->getRelativeAngle2D( source, victim ) : 
											ThePartitionManager->getRelativeAngle2D( source, getMachineGoalPosition() );

		const Real REL_THRESH = 0.035f;	// about 2 degrees. (getRelativeAngle2D is current only accurate to about 1.25 degrees)

		Weapon* weapon = source->getCurrentWeapon();
		Real aimDelta = weapon ? weapon->getAimDelta() : 0.0f;

		if (aimDelta < REL_THRESH) aimDelta = REL_THRESH;

		//DEBUG_LOG(("AIM: desired %f, actual %f, delta %f, aimDelta %f, goalpos %f %f\n",rad2deg(obj->getOrientation() + relAngle),rad2deg(obj->getOrientation()),rad2deg(relAngle),rad2deg(aimDelta),victim->getPosition()->x,victim->getPosition()->y));
		if (m_canTurnInPlace)
		{
			if (fabs(relAngle) > aimDelta) 
			{
				Real desiredAngle = source->getOrientation() + relAngle;
				sourceAI->setLocomotorGoalOrientation(desiredAngle);
				m_setLocomotor = true;
			}
		}
		else
		{
			sourceAI->setLocomotorGoalPositionExplicit(m_isAttackingObject ? *victim->getPosition() : *getMachineGoalPosition());
		}

		if (fabs(relAngle) < aimDelta /*&& !m_preAttackFrames*/ )
		{
			AIUpdateInterface* victimAI = victim ? victim->getAI() : NULL;
			// add ourself as a targeter BEFORE calling isTemporarilyPreventingAimSuccess().
			// we do this every time thru, just in case we get into a squabble with our turret-ai
			// over whether or not we are a targeter... (srj)
			if (victimAI)
				victimAI->addTargeter(source->getID(), true);
			Bool preventing = victimAI && victimAI->isTemporarilyPreventingAimSuccess();
			if (preventing)
			{
				return STATE_CONTINUE;
			}
			else
			{
				return STATE_SUCCESS;
			}
		}
	}

	if (source->isDisabledByType(DISABLED_HELD))
	{
		// We are contained by something (transport, building). This means we can't 
		// actually move to the location and if we are firing on a specific target
		// and that target is no longer in range, then we need to abort the state
		// so it can fire on other targets.

		// Are we still in range?
		Weapon *weapon = source->getCurrentWeapon();
		const Coord3D *pos = m_isAttackingObject ? victim->getPosition() : getMachineGoalPosition();
		if( !weapon || !weapon->isWithinAttackRange( source, pos ) )
		{
			// We're no longer in range, so exit with failure so we can automatically 
			// reacquire a closer target if possible.
			return STATE_FAILURE;
		}
	}

	return STATE_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------
void AIAttackAimAtTargetState::onExit( StateExitType status )
{
	// contained by AIAttackState, so no separate timer
	if (m_canTurnInPlace)
	{
		AIUpdateInterface* sourceAI = getMachineOwner()->getAI();
		// Tell the ai we are done moving, if we set the locomotor goal.
		if (sourceAI && m_setLocomotor) 
			sourceAI->setLocomotorGoalNone();
	}
	else
	{
		// don't do the loco call, or else we will "wiggle"... we already have an appropriate goal
	}

	getMachineOwner()->setStatus( OBJECT_STATUS_IS_AIMING_WEAPON, false );

	//getMachineOwner()->clearModelConditionState( MODELCONDITION_PREATTACK );
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
/**
 * Start firing.
 */
StateReturnType AIAttackFireWeaponState::onEnter()
{
	// contained by AIAttackState, so no separate timer
	DEBUG_ASSERTCRASH(m_att != NULL, ("m_att may not be null"));

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	// Passive stuff will approach but not attack, so we check here (after approach is complete)
	UnsignedInt adjust = ai->getMoodMatrixActionAdjustment(MM_Action_Attack);
	if ((adjust & MAA_Action_Ok) == 0) 
	{
		return STATE_FAILURE;
	}
	Object *victim = getMachineGoalObject();

	if (victim && obj->getTeam()->getPrototype()->getTemplateInfo()->m_attackCommonTarget) {
		if (obj->getTeam()->getTeamTargetObject()==NULL) {
			obj->getTeam()->setTeamTargetObject(victim);
		}
	}

	obj->setStatus( OBJECT_STATUS_IS_FIRING_WEAPON, true );
	obj->preFireCurrentWeapon( getMachineGoalObject() );
	return STATE_CONTINUE;	
}

//----------------------------------------------------------------------------------------------------------
/**
 * Fire the owner's weapon once and exit.
 */

StateReturnType AIAttackFireWeaponState::update()
{
	// contained by AIAttackState, so no separate timer

	Object *obj = getMachineOwner();
	Object* victim = getMachineGoalObject();

	if (m_att->isAttackingObject())
	{
		// if our target is dead, go ahead and stop.
		if (!victim || victim->isEffectivelyDead())
			return STATE_FAILURE;
	}
	WeaponSlotType wslot;
	Weapon* weapon = obj->getCurrentWeapon(&wslot);
	if (!weapon)
	{
		return STATE_FAILURE;
	}
	
	WeaponStatus status = weapon->getStatus();
	if (status == PRE_ATTACK)
	{
		return STATE_CONTINUE;
	} 
	else if (status != READY_TO_FIRE)
	{
		return STATE_FAILURE;
	}

	/**
		this is the weird case where we have multi turrets, and turret 'a' wants
		to fire, but someone has changed the current weapon to be one not on him.
		rather than addressing the situation, we just punt and wait for it to clear
		up on its own.
	*/
	if (m_att && !m_att->isWeaponSlotOkToFire(wslot))
	{
		return STATE_FAILURE;
	}

	// must adjust the state BEFORE calling fireWeapon, for FX to work correctly...
	obj->setFiringConditionForCurrentWeapon();

	if (m_att->isAttackingObject())
	{
		obj->fireCurrentWeapon(victim);
		// clear this, just in case.
		obj->setStatus( OBJECT_STATUS_IGNORING_STEALTH, false );
		Real continueRange = weapon->getContinueAttackRange();
		if (
			continueRange > 0.0f &&
			victim && 
			(victim->isDestroyed() || victim->isEffectivelyDead() || (victim->isKindOf(KINDOF_MINE) && victim->testStatus(OBJECT_STATUS_MASKED)))
		)
		{
			const Coord3D* originalVictimPos = m_att ? m_att->getOriginalVictimPos() : NULL;
			if (originalVictimPos)
			{
				// note that it is important to use getLastCommandSource here; this allows
				// dozers that were ordered to clear mines by the human to continue to autoacquire,
				// but not if they were ordered by ai.
				AIUpdateInterface* ai = obj->getAI();
				CommandSourceType lastCmdSource = ai ? ai->getLastCommandSource() : CMD_FROM_AI;
				PartitionFilterSamePlayer filterPlayer( victim->getControllingPlayer() );
				PartitionFilterSameMapStatus filterMapStatus(obj);
				PartitionFilterPossibleToAttack filterAttack(ATTACK_NEW_TARGET, obj, lastCmdSource);
				PartitionFilter *filters[] = { &filterAttack, &filterPlayer, &filterMapStatus, NULL };
				// note that we look around originalVictimPos, *not* the current victim's pos.
				victim = ThePartitionManager->getClosestObject( originalVictimPos, continueRange, FROM_CENTER_2D, filters );// could be null. this is ok.
				if (victim)
				{
					getMachine()->setGoalObject(victim);
					m_att->notifyNewVictimChosen(victim);
				}
			}
		}
	}
	else
	{
		obj->fireCurrentWeapon(getMachineGoalPosition());
		// clear this, just in case.
		obj->setStatus( OBJECT_STATUS_IGNORING_STEALTH, false );
	}
		
	m_att->notifyFired();

	return STATE_SUCCESS;
}

//----------------------------------------------------------------------------------------------------------
/** 
	Stop firing.
	*/
void AIAttackFireWeaponState::onExit( StateExitType status )
{
	// contained by AIAttackState, so no separate timer
	Object *obj = getMachineOwner();
	obj->setStatus( OBJECT_STATUS_IS_FIRING_WEAPON, false );
	// clear this, just in case.
	obj->setStatus( OBJECT_STATUS_IGNORING_STEALTH, false );

	// this can occur if we start a preattack (eg, bayonet)
	// and the target moves out range before we can actually "fire"...
	// leaving us thinking we're still "pre attacking". cancel this state
	// to avoid confusion. (srj)
	Weapon* weapon = obj->getCurrentWeapon();
	if (weapon && weapon->getStatus() == PRE_ATTACK)
	{
		weapon->setPreAttackFinishedFrame(0);
	}
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
/**
 * Do nothing for a period of time.
 */

StateReturnType AIWaitState::update()
{
			/// @todo srj -- find a way to sleep for a number of frames here, if possible
	return STATE_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
AIAttackState::AIAttackState( StateMachine *machine, Bool follow, Bool attackingObject, Bool forceAttacking, AttackExitConditionsInterface* attackParameters) : 
	State( machine , "AIAttackState"), 
	m_attackMachine(NULL),
	m_attackParameters(attackParameters), 
	m_lockedWeaponOnEnter(NULL), 
	m_follow(follow),
	m_isAttackingObject(attackingObject),
	m_isForceAttacking(forceAttacking),
	m_victimTeam( NULL )
{
	m_originalVictimPos.zero();
}

//----------------------------------------------------------------------------------------------------------
AIAttackState::~AIAttackState()
{
	// nope, don't do this, since we may well still have it targeted
	// even though we're leaving this state.
	// turn it off when we do setCurrentVictim(NULL).
	//addSelfAsTargeter(false);

	if (m_attackMachine) 
	{
		m_attackMachine->halt();
		m_attackMachine->deleteInstance();
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	Bool hasMachine = m_attackMachine!=NULL;
	
	xfer->xferBool(&hasMachine);
	xfer->xferCoord3D(&m_originalVictimPos);

	if (hasMachine && m_attackMachine==NULL)	{
		// create new state machine for attack behavior
		m_attackMachine = newInstance(AttackStateMachine)(getMachineOwner(), this, "AIAttackMachine", m_follow, m_isAttackingObject, m_isForceAttacking  );
	}
	if (hasMachine) {
		xfer->xferSnapshot(m_attackMachine);						///< state sub-machine for attack behavior
	}
	/* Not saved or loaded - passed in on creation.
	Bool										m_follow;
	Bool										m_isAttackingObject;								// if false, attacking position
	AttackExitConditionsInterface*	m_attackParameters;					///< these are not owned by this, and will not be deleted on destruction
	Bool										m_isForceAttacking
	*/
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackState::loadPostProcess( void )
{
	Object* victim = getMachineGoalObject();
	if (victim) 
	{
		m_victimTeam = victim->getTeam();
	}
	Object* source = getMachineOwner();
	m_lockedWeaponOnEnter = source->isCurWeaponLocked() ? source->getCurrentWeapon() : NULL;
}  // end loadPostProcess

#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AIAttackState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_attackMachine) name.concat(m_attackMachine->getCurrentStateName());
	else name.concat("*NULL m_attackMachine");
	return name;
}
#endif

//----------------------------------------------------------------------------------------------------------
void AIAttackState::chooseWeapon()
{
	Object* victim = getMachineGoalObject();
	if (m_isAttackingObject && !victim)
		return;

	Object* source = getMachineOwner();
	AIUpdateInterface *ai = source->getAI();

	if (victim)
	{
		/*bool found =*/ source->chooseBestWeaponForTarget(victim, PREFER_MOST_DAMAGE, ai->getLastCommandSource());
		//DEBUG_ASSERTLOG(found, ("unable to autochoose any weapon for %s\n",source->getTemplate()->getName().str()));
	}

	// Check if we need to update because of the weapon choice switch.
	source->adjustModelConditionForWeaponStatus();
}

//----------------------------------------------------------------------------------------------------------
void AIAttackState::notifyNewVictimChosen(Object* victim)
{
	// do NOT update m_originalVictimPos here. It's a new victim, not the original!
	getMachine()->setGoalObject(victim);
	if (m_attackMachine)
		m_attackMachine->setGoalObject(victim);
}

//----------------------------------------------------------------------------------------------------------
/**
 * Begin an attack on the machine's goal object.
 * To do this complex behavior, instantiate another state machine as a "sub-machine" of 
 * the attack state.
 */
DECLARE_PERF_TIMER(AIAttackState)
StateReturnType AIAttackState::onEnter()
{
	USE_PERF_TIMER(AIAttackState)
	//CRCDEBUG_LOG(("AIAttackState::onEnter() - start for object %d\n", getMachineOwner()->getID()));
	Object* source = getMachineOwner();
	AIUpdateInterface *ai = source->getAI();
	// if we are in sleep mode, we will not attack
	if ((ai->getMoodMatrixActionAdjustment(MM_Action_Attack) & MAA_Action_Ok) == 0)
		return STATE_SUCCESS;

	// if we've met the conditions specified by m_attackParameters, we consider ourselves "successful."
	if (m_attackParameters && m_attackParameters->shouldExit(getMachine())) 
		return STATE_SUCCESS;

	// if all of our weapons are out of ammo, can't attack.
	// (this can happen for units which never auto-reload, like the Raptor)
	if (source->isOutOfAmmo() && !source->isKindOf(KINDOF_PROJECTILE))
		return STATE_FAILURE;

	// create new state machine for attack behavior
	//CRCDEBUG_LOG(("AIAttackState::onEnter() - constructing state machine for object %d\n", getMachineOwner()->getID()));
	m_attackMachine = newInstance(AttackStateMachine)(source, this, "AIAttackMachine", m_follow, m_isAttackingObject, m_isForceAttacking  );

	// tell the attack machine who the victim of the attack is
	if (m_isAttackingObject)
	{
		Object* victim = getMachineGoalObject();
		if (victim == NULL || victim->isEffectivelyDead())	
		{
			ai->notifyVictimIsDead();
			return STATE_FAILURE;	// we have nothing to attack!
		}
		m_victimTeam = victim->getTeam();
		m_attackMachine->setGoalObject( victim );
		m_originalVictimPos = *victim->getPosition();
	}
	else
	{
		m_attackMachine->setGoalPosition(getMachineGoalPosition());		
		m_originalVictimPos = *getMachineGoalPosition();
	}

	chooseWeapon();
	Weapon* curWeapon = source->getCurrentWeapon();
	if (curWeapon)
	{
		curWeapon->setMaxShotCount(NO_MAX_SHOTS_LIMIT);
		// icky special case for ignoring stealth units we might be targeting, that are currently stealthed. (srj)
		if (curWeapon->getContinueAttackRange() > 0.0f)
			source->setStatus(OBJECT_STATUS_IGNORING_STEALTH, true);
	}

	m_lockedWeaponOnEnter = source->isCurWeaponLocked() ? curWeapon : NULL;

	StateReturnType retType = m_attackMachine->initDefaultState();
	if( retType == STATE_CONTINUE )
	{
		source->setStatus( OBJECT_STATUS_IS_ATTACKING, true );
		source->setModelConditionState( MODELCONDITION_ATTACKING );
	}
	return retType;
}

//----------------------------------------------------------------------------------------------------------
/**
 * Execute one frame of "attack enemy" behavior.
 */

StateReturnType AIAttackState::update()
{
	USE_PERF_TIMER(AIAttackState)
	// if we've met the conditions specified by m_attackParameters, we consider ourselves "successful."
	if (m_attackParameters && m_attackParameters->shouldExit(getMachine())) 
	{
		return STATE_SUCCESS;
	}

	Object* source = getMachineOwner();
	
	// if all of our weapons are out of ammo, can't attack.
	// (this can happen for units which never auto-reload, like the Raptor)
	if (source->isOutOfAmmo() && !source->isKindOf(KINDOF_PROJECTILE))
	{
		return STATE_FAILURE;
	}

	if (m_isAttackingObject)
	{
		Object* victim = getMachineGoalObject();

		if (victim == NULL || victim->isEffectivelyDead()) 	
		{
			source->getAI()->notifyVictimIsDead();
			return STATE_SUCCESS;	// my, that was easy
		}

		if (victim) 
		{
			source->getAI()->setCurrentVictim(victim);
		}

		if( victim->getTeam() != m_victimTeam )
		{
			// If, while I have been attacking my victim, it has lost its ability to attack 
			//(a recently de-garrisoned building) I should bail here... 
			// We are not sure whether the problem occurs here or sometime before, but this is an edge case failsafe for it
			// Steven calls this hack 'greasy,' and I agreesy.-Lorenzen
			AIUpdateInterface *ai = source->getAI();
			if (ai)
			{
				if ( (victim->getStatusBits() & OBJECT_STATUS_CAN_ATTACK) == 0 )
				{
					if ( victim->getContain() != NULL )
					{
						if (victim->getContain()->isGarrisonable() && (victim->getContain()->getContainCount() == 0) )
						{
							if ( source->getRelationship( victim ) == NEUTRAL )
							{
								ai->friend_setGoalObject(NULL);
								if (victim==source->getTeam()->getTeamTargetObject()) {
									source->getTeam()->setTeamTargetObject(NULL);
								}
								ai->notifyVictimIsDead();	// well, not "dead", but longer attackable
								return STATE_FAILURE;
							}
						}
					}
				}
			}
			//The team of the victim has changed since we began attacking it. Evaluate
			//whether or not we should keep attacking it.
			// order matters: we want to know if I consider it to be an enemy, not vice versa
			if( source->getRelationship( victim ) != ENEMIES)
			{
				ai->friend_setGoalObject(NULL);
				if (victim==source->getTeam()->getTeamTargetObject()) {
					source->getTeam()->setTeamTargetObject(NULL);
				}
				ai->notifyVictimIsDead();	// well, not "dead", but longer attackable
				return STATE_FAILURE;
			}
		}	

		if (victim != m_attackMachine->getGoalObject()) 
		{
			// Our parent machine has changed the goal.  We need to reset to the new goal.  jba.
			m_attackMachine->setGoalObject( victim );
		}
	}

	// re-evaluate our weapon choice every frame, so the sub-states don't have to.
	chooseWeapon();
	Weapon* curWeapon = source->getCurrentWeapon();

	// if we entered with a locked weapon (ie, a special weapon), then we will
	// only keep attacking as long as that weapon remains the cur weapon...
	// if anything ever changes that weapon, we exit attack mode immediately.
	if (m_lockedWeaponOnEnter != NULL && m_lockedWeaponOnEnter != curWeapon)
		return STATE_FAILURE;

	// we've shot as many times as we are allowed to
	if (curWeapon == NULL || curWeapon->getMaxShotCount() <= 0)
		return STATE_FAILURE;

	/**
	 * Run the attack state sub-machine.
	 * If the attack state machine returns anything other than CONTINUE,
	 * it has finished. Propagating the return code will cause
	 * the containing state machine to do the right thing.
	 * Note the use of CONVERT_SLEEP_TO_CONTINUE; even if the sub-machine
	 * sleeps, we still need to be called every frame.
	 */
	return CONVERT_SLEEP_TO_CONTINUE(m_attackMachine->updateStateMachine());
}

//----------------------------------------------------------------------------------------------------------
void AIAttackState::onExit( StateExitType status )
{
	USE_PERF_TIMER(AIAttackState)
	// nope, don't do this, since we may well still have it targeted
	// even though we're leaving this state. turn it off when we
	// turn it off when we do setCurrentVictim(NULL).
	//addSelfAsTargeter(false);

	// destroy the attack machine
	if (m_attackMachine)
	{
		m_attackMachine->deleteInstance();
		m_attackMachine = NULL;
	}

	Object *obj = getMachineOwner();
	obj->setStatus( OBJECT_STATUS_IS_FIRING_WEAPON, false );
	obj->setStatus( OBJECT_STATUS_IS_AIMING_WEAPON, false );
	obj->setStatus( OBJECT_STATUS_IS_ATTACKING, false );
	obj->setStatus( OBJECT_STATUS_IGNORING_STEALTH, false );
	obj->clearModelConditionState( MODELCONDITION_ATTACKING );

	obj->clearLeechRangeModeForAllWeapons();

	AIUpdateInterface *ai = obj->getAI();
	if (ai) 
	{	
		//ai->notifyVictimIsDead();	no, do NOT do this here.
		ai->setCurrentVictim(NULL);
		for (int i = 0; i < MAX_TURRETS; ++i)
			ai->setTurretTargetObject((WhichTurretType)i, NULL, 0);
		ai->friend_setGoalObject(NULL);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------
class AIAttackThenIdleStateMachine : public StateMachine
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( AIAttackThenIdleStateMachine, "AIAttackThenIdleStateMachine" );

public:

	AIAttackThenIdleStateMachine( Object *owner, AsciiString name );
protected:
	// snapshot interface .
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess();
};

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackThenIdleStateMachine::crc( Xfer *xfer )
{
	StateMachine::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackThenIdleStateMachine::xfer( Xfer *xfer )
{
	XferVersion cv = 1;	
	XferVersion v = cv; 
	xfer->xferVersion( &v, cv );

	StateMachine::xfer(xfer);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackThenIdleStateMachine::loadPostProcess( void )
{
	StateMachine::loadPostProcess();
}  // end loadPostProcess

//-----------------------------------------------------------------------------------------------------------
AIAttackThenIdleStateMachine::AIAttackThenIdleStateMachine(Object *owner, AsciiString name) : StateMachine(owner, name)
{
	// order matters: first state is the default state.
	defineState( AI_ATTACK_OBJECT,	newInstance(AIAttackState)(this, false, true, false, NULL ), AI_IDLE, AI_IDLE );
	defineState( AI_PICK_UP_CRATE, newInstance(AIPickUpCrateState)( this ), AI_IDLE, AI_IDLE );
	defineState( AI_IDLE, newInstance(AIIdleState)( this, AIIdleState::DO_NOT_LOOK_FOR_TARGETS ), AI_IDLE, AI_IDLE );
}

//----------------------------------------------------------------------------------------------------------
AIAttackThenIdleStateMachine::~AIAttackThenIdleStateMachine()
{
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
AIAttackSquadState::~AIAttackSquadState()
{
	if (m_attackSquadMachine)	{
		m_attackSquadMachine->halt();
		m_attackSquadMachine->deleteInstance();
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackSquadState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackSquadState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );
	
	Bool hasMachine = m_attackSquadMachine!=NULL;
	
	xfer->xferBool(&hasMachine);

	if (hasMachine && m_attackSquadMachine==NULL)	{
		// create new state machine for attack behavior
		m_attackSquadMachine = newInstance(AIAttackThenIdleStateMachine)( getMachineOwner(), "AIAttackMachine"  );
	}

	if (hasMachine) {
		xfer->xferSnapshot(m_attackSquadMachine);
	}
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackSquadState::loadPostProcess( void )
{
}  // end loadPostProcess

#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AIAttackSquadState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_attackSquadMachine) name.concat(m_attackSquadMachine->getCurrentStateName());
	else name.concat("*NULL m_attackSquadMachine");
	return name;
}
#endif

//----------------------------------------------------------------------------------------------------------
/**
 * Begin an attack on the machine's goal team.
 * To do this, we use a sub attack machine.
 */
StateReturnType AIAttackSquadState::onEnter( void )
{
	// create new state machine for attack behavior
	m_attackSquadMachine = newInstance(AIAttackThenIdleStateMachine)( getMachineOwner(), "AIAttackMachine"  );
	
	Object *victim = chooseVictim();
	// tell the attack machine who the victim of the attack is
	m_attackSquadMachine->setGoalObject( victim );

	// initial state of attack state machine
	return m_attackSquadMachine->initDefaultState();
}

//----------------------------------------------------------------------------------------------------------
/**
 * Execute one frame of "attack enemy" behavior.
 */

StateReturnType AIAttackSquadState::update( void )
{

	if( !m_attackSquadMachine )
	{
		return STATE_FAILURE;
	}

	/* 
		Note the use of CONVERT_SLEEP_TO_CONTINUE; even if the sub-machine
		sleeps, we still need to be called every frame.
	*/
	StateReturnType attackStatus = CONVERT_SLEEP_TO_CONTINUE(m_attackSquadMachine->updateStateMachine());

	// if we're in attack state, 
	if (m_attackSquadMachine->getCurrentStateID() != AI_IDLE) 
	{
		return attackStatus;
	}

	// Check to see if we have created a crate we need to pick up.
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	Object* crate = ai->checkForCrateToPickup();
	if (crate)
	{
		m_attackSquadMachine->setGoalObject(crate);
		m_attackSquadMachine->setState( AI_PICK_UP_CRATE );
		return STATE_CONTINUE;
	}

	// choose a new target and start over.
	Object *victim = chooseVictim();
	if (!victim) 
	{
		return STATE_SUCCESS;
	}

	m_attackSquadMachine->setGoalObject( victim );
	m_attackSquadMachine->setState(AI_ATTACK_OBJECT);
	return STATE_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------
void AIAttackSquadState::onExit( StateExitType status )
{
	if( m_attackSquadMachine )
	{
		// destroy the attack machine
		m_attackSquadMachine->deleteInstance();
		m_attackSquadMachine = NULL;
	}
}

//----------------------------------------------------------------------------------------------------------
Object *AIAttackSquadState::chooseVictim(void)
{
	Squad *victimSquad = ((AIStateMachine*)getMachine())->getGoalSquad();
	if (!victimSquad) 
	{
		return NULL;
	}

	Object *owner = getMachineOwner();
	AIUpdateInterface *ai = owner->getAI();
	UnsignedInt moodVal = ai->getMoodMatrixValue();
	
	if (moodVal & MM_Controller_AI) 
	{
		if (moodVal & MM_Mood_Sleep) 
		{
			return NULL;
		}

		if (moodVal & MM_Mood_Passive) 
		{
			BodyModuleInterface *bmi = owner->getBodyModule();
			if (!bmi) {
				return NULL;
			}
			
			const DamageInfo *di = bmi->getLastDamageInfo();
			if (!di) 
			{
				return NULL;
			}

			return TheGameLogic->findObjectByID(di->in.m_sourceID);
		}
	}

	GameDifficulty difficulty = owner->getControllingPlayer()->getPlayerDifficulty();
	const CommandSourceType cmdSource = ai->getLastCommandSource();
	
	if (cmdSource == CMD_FROM_PLAYER) 
	{
		// if a player did this, we want to always give them the Seek and obliterate method.
		difficulty = DIFFICULTY_HARD;
	}

	if (TheScriptEngine->getChooseVictimAlwaysUsesNormal())
	{
		difficulty = DIFFICULTY_NORMAL;
	}	

	switch(difficulty)
	{
		case DIFFICULTY_EASY:
		{
			// pick a random unit
			VecObjectPtr objects = victimSquad->getLiveObjects();
			Int numUnits = objects.size();
			if (numUnits == 0) 
			{
				return NULL;
			}

			Int unitToAttack = GameLogicRandomValue(0, numUnits - 1);
			return objects[unitToAttack];
			break;
		}

		case DIFFICULTY_NORMAL:
		{
			// pick the closest unit

			PartitionFilterAcceptOnSquad f1(victimSquad);
			PartitionFilterSameMapStatus filterMapStatus(getMachineOwner());
			PartitionFilter *filters[] = { &f1, &filterMapStatus, NULL };
			
			Object *victim = ThePartitionManager->getClosestObject(getMachineOwner(), HUGE_DIST, FROM_CENTER_2D, filters, NULL, NULL);
			return victim;
			break;
		}

		case DIFFICULTY_HARD:
		{
			// everyone picks the same unit
			VecObjectPtr objects = victimSquad->getLiveObjects();
			if (objects.size() > 0) 
			{
				return objects[0];
			}

			return NULL;
			break;
		}
	};

	return NULL;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
AIDockState::~AIDockState()
{
	if (m_dockMachine) {
		m_dockMachine->halt();
		m_dockMachine->deleteInstance();
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIDockState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIDockState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	Bool hasMachine = m_dockMachine!=NULL;
	
	xfer->xferBool(&hasMachine);

	if (hasMachine && m_dockMachine==NULL)	{
		// create new state machine for attack behavior
		m_dockMachine = newInstance(AIDockMachine)( getMachineOwner());
	}
	if (hasMachine) {
		xfer->xferSnapshot(m_dockMachine);
	}
	xfer->xferBool(&m_usingPrecisionMovement);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIDockState::loadPostProcess( void )
{
}  // end loadPostProcess

#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AIDockState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_dockMachine) name.concat(m_dockMachine->getCurrentStateName());
	else name.concat("*NULL m_dockMachine");
	return name;
}
#endif

//----------------------------------------------------------------------------------------------
/**
 * Dock with the GoalObject.
 * When we enter the AI_DOCK state, create a docking state machine
 * that implements the details of the docking behavior.
 */
StateReturnType AIDockState::onEnter()
{
	// who are we docking with?
	Object *dockWithMe = getMachineGoalObject();
	if (dockWithMe == NULL)
	{
		// we have nothing to dock with!
		DEBUG_LOG(("No goal in AIDockState::onEnter - exiting.\n"));
		return STATE_FAILURE;
	}
  DockUpdateInterface *dock = NULL;
	dock = dockWithMe->getDockUpdateInterface();

	// if we have nothing to dock with, fail
	if (dock == NULL)	{
		DEBUG_LOG(("Goal is not a dock in AIDockState::onEnter - exiting.\n"));
		return STATE_FAILURE;
	}

	// tell the pathfinder to ignore the object we are docking with, so it doesn't block us
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if( ai ) 
	{
		ai->ignoreObstacle( dockWithMe );
	}

	// create new state machine for attack behavior
	m_dockMachine = newInstance(AIDockMachine)( getMachineOwner());

	// tell the docking machine what it is docking with
	m_dockMachine->setGoalObject( dockWithMe );
	// now that essential parameters are set, set the machine's initial state
	return m_dockMachine->initDefaultState( );
}

/**
 * For whatever reason, we are leaving the AI_DOCK state.
 * Destroy the docking sub-machine.
 */
void AIDockState::onExit( StateExitType status )
{
	// destroy the dock machine
	if (m_dockMachine) {
		m_dockMachine->halt();// GS, you have to halt before you delete to do cleanup.
		m_dockMachine->deleteInstance();
		m_dockMachine = NULL;
	}	else {
		DEBUG_LOG(("Dock exited immediately\n"));
	}

	// stop ignoring our goal object
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (ai)
	{
		ai->setCanPathThroughUnits(false);
		ai->ignoreObstacle( NULL );
	}
	
}

/**
 * We are in the AI_DOCK state, execute the docking behavior.
 */

StateReturnType AIDockState::update()
{

	/**
	 * Run the docking state sub-machine.
	 * If the dock state machine returns anything other than CONTINUE,
	 * it has finished. propagating the return code will cause
	 * the containing state machine to do the right thing.
	 */
	AIUpdateInterface *ai = getMachineOwner()->getAI();
	if (ai)
	{
		ai->setCanPathThroughUnits(true);
		//if (ai->isBlockedAndStuck()) {
			//DEBUG_LOG(("Blocked and stuck.\n"));
		//}
		//if (ai->getNumFramesBlocked()>5) {
			//DEBUG_LOG(("Blocked %d frames\n", ai->getNumFramesBlocked()));
		//}
	}
	/* 
		Note the use of CONVERT_SLEEP_TO_CONTINUE; even if the sub-machine
		sleeps, we still need to be called every frame.
	*/
	return CONVERT_SLEEP_TO_CONTINUE(m_dockMachine->updateStateMachine());
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIEnterState::crc( Xfer *xfer )
{
	AIInternalMoveToState::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIEnterState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	AIInternalMoveToState::xfer(xfer);

	xfer->xferObjectID(&m_entryToClear);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIEnterState::loadPostProcess( void )
{
	AIInternalMoveToState::loadPostProcess();
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIEnterState::onEnter()
{
	m_entryToClear = INVALID_ID;

	Object* obj = getMachineOwner();
	Object* goal = getMachineGoalObject();
	if (goal)
	{
		if( !TheActionManager->canEnterObject( obj, goal, obj->getAI()->getLastCommandSource(), CHECK_CAPACITY ) )
			return STATE_FAILURE;	

		m_goalPosition = *goal->getPosition();

		ContainModuleInterface* contain = goal->getContain();
		if (contain)
		{
			contain->onObjectWantsToEnterOrExit(obj, WANTS_TO_ENTER);
			m_entryToClear = goal->getID();
		}
	}
	else
	{
		return STATE_FAILURE;
	}

	// tell the pathfinder to ignore the enterable object
	AIUpdateInterface *ai = obj->getAI();
	ai->ignoreObstacle( getMachineGoalObject() );
	if (ai->getCurLocomotor()) 
	{
		ai->getCurLocomotor()->setAllowInvalidPosition(true);
	}
	setAdjustsDestination(false);
	return AIInternalMoveToState::onEnter();
}

//----------------------------------------------------------------------------------------------------------
void AIEnterState::onExit( StateExitType status )
{
	Object* obj = getMachineOwner();
	AIInternalMoveToState::onExit( status );

	// tell the pathfinder to stop ignoring the object
	AIUpdateInterface *ai = obj->getAI();
	if (ai) 
	{

		ai->ignoreObstacle( NULL );
		if (ai->getCurLocomotor()) 
		{
			ai->getCurLocomotor()->setAllowInvalidPosition(false);
		}
	}

	// use this, rather than getMachineGoalObject, in case the goal
	// is killed while we were waiting...
	if (m_entryToClear != INVALID_ID)
	{
		Object* goal = TheGameLogic->findObjectByID(m_entryToClear);
		if (goal)
		{
			ContainModuleInterface* contain = goal->getContain();
			if (contain)
			{
				contain->onObjectWantsToEnterOrExit(obj, WANTS_NEITHER);
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIEnterState::update()
{

	// update the goal position to coincide with the GoalObject
	Object* obj = getMachineOwner();
	Object* goal = getMachineGoalObject();
	if (goal)
	{
		// if our goal is contained by something else, give up. this is for the following bug:
		// -- tell some rangers to enter a humvee
		// -- tell the humvee to enter a chinook
		// -- fly the chinook around; the rangers follow the chinook like dopes
		// this just bails in this case. (srj)
		if (goal->getContainedBy() != NULL && goal->isAboveTerrain() && !obj->isAboveTerrain())
		{
			return STATE_FAILURE;	
		}

		m_goalPosition = *goal->getPosition();
		obj->getAI()->friend_setGoalObject(goal);
		if (!TheActionManager->canEnterObject(obj, goal, obj->getAI()->getLastCommandSource(), CHECK_CAPACITY))
		{
			/*
				special-case: if it's an enemy, try attacking it instead. this is to address this bug: (srj)

				Bug: Units stop instead of attacking if building they were trying to garrison is taken by enemy units first.

				1. any game/any faction
				2. build infantry units that can garrison neutral buildings
				3. have infantry  units garrison a neutral building, before they garrison the building have some enemy units garrison it first.

				Expected result: Based on test plan: Instead of just stopping when enemy units garrison building first, they should continue and attack the building.
			*/
			if( obj->getRelationship(goal) == ENEMIES && obj->getAI() )
			{
				CanAttackResult result = TheActionManager->getCanAttackObject(obj, goal, obj->getAI()->getLastCommandSource(), ATTACK_NEW_TARGET);
				if( result == ATTACKRESULT_POSSIBLE || result == ATTACKRESULT_POSSIBLE_AFTER_MOVING )
				{
	 				obj->getAI()->aiAttackObject(goal, NO_MAX_SHOTS_LIMIT, obj->getAI()->getLastCommandSource());
					// weird but true. return state_continue, because if we're here, we're actually an attack state
					// since we just changed the state, it doesn't really matter what we return here.
					return STATE_CONTINUE;
				}
				return STATE_FAILURE;	
			}
			return STATE_FAILURE;	
		}

		// If we are held, then we must have entered the goal.
		if( getMachineOwner()->isDisabledByType( DISABLED_HELD ) )
		{
			return STATE_SUCCESS;
		}
	} 
	else 
	{
		return STATE_FAILURE;
	}

	StateReturnType code = AIInternalMoveToState::update();

	// if it's airborne, wait for it to land
	if (code == STATE_SUCCESS && goal->isAboveTerrain() && !obj->isAboveTerrain())
	{
		code = STATE_CONTINUE;
	}

	if (code == STATE_SUCCESS) 
	{
		// Make sure we entered the container.
		// srj sez: I don't think we want to restrict this to HELD items. See the intro of GLA02.map
		// for an example of guys-off-the-border-but-not-held who need this check.
		//if( obj->isDisabledByType( DISABLED_HELD ) )
		{
			if (goal)
			{
				// we didn't enter.  See if we're close.
				Real dx = (obj->getPosition()->x - goal->getPosition()->x);
				Real dy = (obj->getPosition()->y - goal->getPosition()->y);
				Real radius = goal->getGeometryInfo().getMinorRadius();
				if (goal->getGeometryInfo().getGeomType()!=GEOMETRY_BOX) {
					radius = goal->getGeometryInfo().getMajorRadius();
				}
				Bool closeEnough = dx*dx+dy*dy < sqr(radius);
				if (closeEnough) {
					// Grab the container and force ourselves into it.
					// This case is primarily to handle transports on the map border for scripted setup.
					// The partition manager doesn't generate collisions in the border area, so we have to
					// add ourselves.  jba.
					ContainModuleInterface* contain = goal->getContain();
					if (contain)
					{
						contain->addToContain(obj);
					}
				}
			}
		}
	}

	return code;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIExitState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIExitState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferObjectID(&m_entryToClear);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIExitState::loadPostProcess( void )
{
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIExitState::onEnter()
{
	m_entryToClear = INVALID_ID;

	Object* obj = getMachineOwner();
	Object* goal = getMachineGoalObject();
	if (goal)
	{
		ContainModuleInterface* contain = goal->getContain();
		if (contain)
		{
			contain->onObjectWantsToEnterOrExit(obj, WANTS_TO_EXIT);
			m_entryToClear = goal->getID();
		}
		return STATE_CONTINUE;
	}
	else
	{
		return STATE_FAILURE;
	}
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIExitState::update()
{

	// update the goal position to coincide with the GoalObject
	Object* obj = getMachineOwner();
	Object* goal = getMachineGoalObject();
	if (goal)
	{
		AIUpdateInterface* goalAI = goal->getAI();
		if (goalAI && goalAI->getAiFreeToExit(obj) == WAIT_TO_EXIT)
			return STATE_CONTINUE;

		DEBUG_ASSERTCRASH(obj, ("obj must not be null here"));

		//GS.  The goal of unified ExitInterfaces dies a horrible death.  I can't ask Object for the exit,
		// as removeFromContain is only in the Contain type.  I'm spliting the names in shame.
		ExitInterface* goalExitInterface = goal->getContain() ? goal->getContain()->getContainExitInterface() : NULL;
		if( goalExitInterface == NULL )
			return STATE_FAILURE;

		if( goalExitInterface->isExitBusy() )
			return STATE_CONTINUE;// Just wait a sec.

		ExitDoorType exitDoor = goalExitInterface ? goalExitInterface->reserveDoorForExit(obj->getTemplate(), obj) : DOOR_NONE_NEEDED;
		if (exitDoor == DOOR_NONE_AVAILABLE)
			return STATE_FAILURE;

		goalExitInterface->exitObjectViaDoor(obj, exitDoor);
		if( getMachine()->getCurrentStateID() != getID() )
			return STATE_CONTINUE;// Not sucess, because exitViaDoor has changed us to FollowPath, and if we say Success, our machine will think FollowPath succeeded
		else
			return STATE_SUCCESS;
	} 
	else 
	{
		return STATE_FAILURE;
	}
}

//----------------------------------------------------------------------------------------------------------
void AIExitState::onExit( StateExitType status )
{
	Object* obj = getMachineOwner();

	// use this, rather than getMachineGoalObject, in case the goal
	// is killed while we were waiting...
	if (m_entryToClear != INVALID_ID)
	{
		Object* goal = TheGameLogic->findObjectByID(m_entryToClear);
		if (goal)
		{
			ContainModuleInterface* contain = goal->getContain();
			if (contain)
			{
				contain->onObjectWantsToEnterOrExit(obj, WANTS_NEITHER);
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
AIGuardState::~AIGuardState()
{
	if (m_guardMachine)	{
		m_guardMachine->halt();
		m_guardMachine->deleteInstance();
	}
}


#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AIGuardState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_guardMachine) name.concat(m_guardMachine->getCurrentStateName());
	else name.concat("*NULL guardMachine");
	return name;
}
#endif
// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIGuardState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIGuardState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	Bool hasMachine = m_guardMachine!=NULL;
	
	xfer->xferBool(&hasMachine);

	if (hasMachine && m_guardMachine==NULL)	{
		// create new state machine for guard behavior
		m_guardMachine = newInstance(AIGuardMachine)( getMachineOwner());
	}
	if (hasMachine) {
		xfer->xferSnapshot(m_guardMachine);	
	}

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIGuardState::loadPostProcess( void )
{
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
/**
 * Guard location.
 */

StateReturnType AIGuardState::onEnter()
{

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	m_guardMachine = newInstance(AIGuardMachine)( getMachineOwner());

	// tell the guarding machine what it is guarding with
	switch(ai->getGuardTargetType())
	{
		case GUARDTARGET_LOCATION: m_guardMachine->setTargetPositionToGuard( ai->getGuardLocation() ); break;
		case GUARDTARGET_OBJECT: m_guardMachine->setTargetToGuard( TheGameLogic->findObjectByID(ai->getGuardObject()) ); break;
		case GUARDTARGET_AREA: m_guardMachine->setAreaToGuard( ai->getAreaToGuard() ); break;
	}
	m_guardMachine->setGuardMode(ai->getGuardMode());

	// now that essential parameters are set, set the machine's initial state
	if (m_guardMachine->initDefaultState() == STATE_FAILURE) 
		return STATE_FAILURE;
	return m_guardMachine->setState(AI_GUARD_RETURN);
}

//----------------------------------------------------------------------------------------------------------
void AIGuardState::onExit( StateExitType status )
{
	m_guardMachine->deleteInstance();
	m_guardMachine = NULL;

	Object *obj = getMachineOwner();
	obj->getAI()->clearGuardTargetType();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIGuardState::update()
{
	//DEBUG_LOG(("AIGuardState frame %d: %08lx\n",TheGameLogic->getFrame(),getMachineOwner()));

	if (m_guardMachine == NULL) 
	{
		return STATE_FAILURE; // We actually already exited.
	}

	// if all of our weapons are out of ammo, can't attack.
	// (this can happen for units which never auto-reload, like the Raptor)
	Object* owner = getMachineOwner();
	if (owner->isOutOfAmmo() && !owner->isKindOf(KINDOF_PROJECTILE))
	{
		DEBUG_CRASH(("Hmm, this should probably never happen, since this case should be intercepted by JetAIUpdate\n"));
		return STATE_FAILURE;
	}

	getMachine()->lock("AIGuardState::update");	// We don't want to switch out of guard during the update.
	StateReturnType ret = m_guardMachine->updateStateMachine();
	getMachine()->unlock();
	return ret;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
AITunnelNetworkGuardState::~AITunnelNetworkGuardState()
{
	if (m_guardMachine)	{
		m_guardMachine->halt();
		m_guardMachine->deleteInstance();
	}
}


#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AITunnelNetworkGuardState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_guardMachine) name.concat(m_guardMachine->getCurrentStateName());
	else name.concat("*NULL guardMachine");
	return name;
}
#endif
// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AITunnelNetworkGuardState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AITunnelNetworkGuardState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	Bool hasMachine = m_guardMachine!=NULL;
	
	xfer->xferBool(&hasMachine);

	if (hasMachine && m_guardMachine==NULL)	{
		// create new state machine for guard behavior
		m_guardMachine = newInstance(AITNGuardMachine)( getMachineOwner());
	}
	if (hasMachine) {
		xfer->xferSnapshot(m_guardMachine);	
	}

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AITunnelNetworkGuardState::loadPostProcess( void )
{
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
/**
 * Guard location.
 */

StateReturnType AITunnelNetworkGuardState::onEnter()
{

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	m_guardMachine = newInstance(AITNGuardMachine)( getMachineOwner());

	// tell the guarding machine what it is guarding with
	m_guardMachine->setTargetPositionToGuard( ai->getGuardLocation() ); 
	m_guardMachine->setGuardMode(ai->getGuardMode());

	// now that essential parameters are set, set the machine's initial state
	if (m_guardMachine->initDefaultState() == STATE_FAILURE) 
		return STATE_FAILURE;
	return m_guardMachine->setState(AI_GUARD_RETURN);
}

//----------------------------------------------------------------------------------------------------------
void AITunnelNetworkGuardState::onExit( StateExitType status )
{
	m_guardMachine->deleteInstance();
	m_guardMachine = NULL;

	Object *obj = getMachineOwner();
	obj->getAI()->clearGuardTargetType();
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AITunnelNetworkGuardState::update()
{
	//DEBUG_LOG(("AITunnelNetworkGuardState frame %d: %08lx\n",TheGameLogic->getFrame(),getMachineOwner()));

	if (m_guardMachine == NULL) 
	{
		return STATE_FAILURE; // We actually already exited.
	}

	// if all of our weapons are out of ammo, can't attack.
	// (this can happen for units which never auto-reload, like the Raptor)
	Object* owner = getMachineOwner();
	if (owner->isOutOfAmmo() && !owner->isKindOf(KINDOF_PROJECTILE))
	{
		DEBUG_CRASH(("Hmm, this should probably never happen, since this case should be intercepted by JetAIUpdate\n"));
		return STATE_FAILURE;
	}

	getMachine()->lock("AITunnelNetworkGuardState::update");	// We don't want to switch out of guard during the update.
	StateReturnType ret = m_guardMachine->updateStateMachine();
	getMachine()->unlock();
	return ret;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
AIHuntState::~AIHuntState()
{
	if (m_huntMachine) 
	{
		m_huntMachine->halt();
		m_huntMachine->deleteInstance();
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIHuntState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIHuntState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	Bool hasMachine = m_huntMachine!=NULL;
	
	xfer->xferBool(&hasMachine);

	if (hasMachine && m_huntMachine==NULL)	{
		// create new state machine for hunt behavior
		m_huntMachine = newInstance(AIAttackThenIdleStateMachine)( getMachineOwner(), "AIAttackThenIdleStateMachine");
	}
	if (hasMachine) {
		xfer->xferSnapshot(m_huntMachine);
	}
	xfer->xferUnsignedInt(&m_nextEnemyScanTime);

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIHuntState::loadPostProcess( void )
{
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
/**
 * Hunt (seek and destroy).
 */

StateReturnType AIHuntState::onEnter()
{
	// create new state machine for hunt behavior
	m_huntMachine = newInstance(AIAttackThenIdleStateMachine)( getMachineOwner(), "AIAttackThenIdleStateMachine");

	// first time thru, use a random amount so that everyone doesn't scan on the same frame,
	// to avoid "spikes". 
	UnsignedInt sleepTime = GameLogicRandomValue(0, ENEMY_SCAN_RATE);
	UnsignedInt now = TheGameLogic->getFrame();
	m_nextEnemyScanTime = now + sleepTime;

	// initial state of hunt state machine
	return m_huntMachine->initDefaultState();
}

//----------------------------------------------------------------------------------------------------------
void AIHuntState::onExit( StateExitType status )
{
	// destroy the hunt machine
	m_huntMachine->deleteInstance();
	m_huntMachine = NULL;

	Object *obj = getMachineOwner();
	if (obj) 
	{
		obj->releaseWeaponLock(LOCKED_TEMPORARILY);	// release any temporary locks.
	}
}

#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AIHuntState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_huntMachine) name.concat(m_huntMachine->getCurrentStateName());
	else name.concat("*NULL huntMachine");
	return name;
}
#endif

//----------------------------------------------------------------------------------------------------------
StateReturnType AIHuntState::update()
{

	// look around for better victims every so often
	UnsignedInt now = TheGameLogic->getFrame();
	if (now >= m_nextEnemyScanTime)
	{
		Object* owner = getMachineOwner();

		// if all of our weapons are out of ammo, can't hunt.
		// (this can happen for units which never auto-reload, like the Raptor)
		if (owner->isOutOfAmmo() && !owner->isKindOf(KINDOF_PROJECTILE))
			return STATE_FAILURE;

		// Check to see if we have created a crate we need to pick up.
		AIUpdateInterface *ai = owner->getAI();
		Object* crate = ai->checkForCrateToPickup();
		if (crate)
		{
			m_huntMachine->setGoalObject(crate);
			m_huntMachine->setState( AI_PICK_UP_CRATE );
			return STATE_CONTINUE;
		}

		m_nextEnemyScanTime = now + ENEMY_SCAN_RATE;

		const AttackPriorityInfo *info = NULL;
		info = ai->getAttackInfo();

		// Check if team auto targets same victim.
		Object* teamVictim = NULL;
		if (owner->getTeam()->getPrototype()->getTemplateInfo()->m_attackCommonTarget) 
		{
			teamVictim = owner->getTeam()->getTeamTargetObject();
		}
		Object* victim = NULL;
		if (teamVictim && info==NULL) 
		{
			victim = teamVictim;
		} 
		else 
		{
			// do NOT do line of sight check - we want to find everything
			victim = TheAI->findClosestEnemy( owner, 9999.9f, AI::CAN_ATTACK, info );
			if (victim==NULL && owner->getControllingPlayer() && owner->getControllingPlayer()->getUnitsShouldHunt()) {
				// If we are doing an all hunt, try hunting without the attack priority info. jba.
				victim = TheAI->findClosestEnemy(owner, 9999.9f, AI::CAN_ATTACK, NULL);
			}
			if (owner->getTeam()->getPrototype()->getTemplateInfo()->m_attackCommonTarget) 
			{
				// Check priorities.
				if (teamVictim && info) {
					if (victim==NULL) {
						DEBUG_LOG(("Couldnt' find victim. hmm."));
						victim = teamVictim;
					}
					Int teamVictimPriority = info->getPriority(teamVictim->getTemplate());
					Int victimPriority;
					if( victim )
						victimPriority = info->getPriority(victim->getTemplate());
					else
						victimPriority = 0;

					if (teamVictimPriority>=victimPriority) {
						victim = teamVictim;
					}
				}
				owner->getTeam()->setTeamTargetObject(victim);
			}
		}
		m_huntMachine->setGoalObject( victim );

		if (m_huntMachine->getCurrentStateID() == AI_IDLE && victim)	
		{
			m_huntMachine->setState( AI_ATTACK_OBJECT );
		}
		if (owner->getControllingPlayer() && owner->getControllingPlayer()->getUnitsShouldHunt()==FALSE) {
			// If we are not doing an all hunt, then exit hunt state - no more victims.
			if (m_huntMachine->getCurrentStateID() == AI_IDLE && victim==NULL) {
				return STATE_SUCCESS; // we killed everything :) jba.
			}
		}
	}

	getMachine()->lock("AIHuntState::update");	// The idle state in the sub machine can sometimes acquire targets. 
												// It is important to not switch out of this state via a sub machine call. jba.
	/* 
		Note the use of CONVERT_SLEEP_TO_CONTINUE; even if the sub-machine
		sleeps, we still need to be called every frame.
	*/
			/// @todo srj -- find a way to sleep for a number of frames here, if possible
	StateReturnType ret = CONVERT_SLEEP_TO_CONTINUE(m_huntMachine->updateStateMachine());
	getMachine()->unlock();
	return ret;
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------
AIAttackAreaState::~AIAttackAreaState()
{
	if (m_attackMachine) {
		m_attackMachine->halt();
		m_attackMachine->deleteInstance();
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIAttackAreaState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIAttackAreaState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	Bool hasMachine = m_attackMachine!=NULL;
	
	xfer->xferBool(&hasMachine);

	if (hasMachine && m_attackMachine==NULL)	{
		// create new state machine for hunt behavior
		m_attackMachine = newInstance(AIAttackThenIdleStateMachine)( getMachineOwner(), "AIAttackThenIdleStateMachine");
	}
	if (hasMachine) {
		xfer->xferSnapshot(m_attackMachine);
	}
	xfer->xferUnsignedInt(&m_nextEnemyScanTime);

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIAttackAreaState::loadPostProcess( void )
{
}  // end loadPostProcess

#ifdef STATE_MACHINE_DEBUG
//----------------------------------------------------------------------------------------------------------
AsciiString AIAttackAreaState::getName(  ) const
{
	AsciiString name = m_name;
	name.concat("/");
	if (m_attackMachine) name.concat(m_attackMachine->getCurrentStateName());
	else name.concat("*NULL m_attackMachine");
	return name;
}
#endif

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackAreaState::onEnter()
{

	// create new state machine for hunt behavior
	m_attackMachine = newInstance(AIAttackThenIdleStateMachine)( getMachineOwner(), "AIAttackThenIdleStateMachine");

	// first time thru, use a random amount so that everyone doesn't scan on the same frame,
	// to avoid "spikes". 
	UnsignedInt now = TheGameLogic->getFrame();
	m_nextEnemyScanTime = now + GameLogicRandomValue(0, ENEMY_SCAN_RATE);

	// initial state of hunt state machine
	return m_attackMachine->initDefaultState();
}

//----------------------------------------------------------------------------------------------------------
void AIAttackAreaState::onExit( StateExitType status )
{
	// destroy the hunt machine
	m_attackMachine->deleteInstance();
	m_attackMachine = NULL;
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIAttackAreaState::update()
{
	// look around for better victims every so often
	UnsignedInt now = TheGameLogic->getFrame();
	if (now >= m_nextEnemyScanTime)
	{
		Object* owner = getMachineOwner();

		// if all of our weapons are out of ammo, can't hunt.
		// (this can happen for units which never auto-reload, like the Raptor)
		if (owner->isOutOfAmmo() && !owner->isKindOf(KINDOF_PROJECTILE))
			return STATE_FAILURE;

		// first time thru, add a random amount so that everyone doesn't scan on the same frame,
		// to avoid "spikes". Note that this implementation ensures that this unit checks immediately
		// upon entering this state, then wait a possibly-longer-than-usual time (due to randomness),
		// then settle into a regular schedule.
		m_nextEnemyScanTime = now + ENEMY_SCAN_RATE;

		AIUpdateInterface *ai = owner->getAI();
		if (ai->getAreaToGuard() == NULL) 
			return STATE_FAILURE;

		const AttackPriorityInfo *info = NULL;
		info = ai->getAttackInfo();
		PartitionFilterPolygonTrigger polyFilter(ai->getAreaToGuard());

		// do NOT do line of sight check - we want to find everything
		Object *victim = TheAI->findClosestEnemy( owner, 9999.9f, AI::CAN_ATTACK, info, &polyFilter );
		m_attackMachine->setGoalObject( victim );

		if (m_attackMachine->getCurrentStateID() == AI_IDLE && victim)	
		{
			m_attackMachine->setState( AI_ATTACK_OBJECT );
		}
		if (victim==NULL) {
			return STATE_SUCCESS;
		}
	}

	getMachine()->lock("AIAttackAreaState::update");	// The idle state in the sub machine can sometimes acquire targets. 
												// It is important to not switch out of this state via a sub machine call. jba.
	/* 
		Note the use of CONVERT_SLEEP_TO_CONTINUE; even if the sub-machine
		sleeps, we still need to be called every frame.
	*/
			/// @todo srj -- find a way to sleep for a number of frames here, if possible
	StateReturnType ret = CONVERT_SLEEP_TO_CONTINUE(m_attackMachine->updateStateMachine());
	getMachine()->unlock();
	return ret;
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AIFaceState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void AIFaceState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferBool(&m_canTurnInPlace);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AIFaceState::loadPostProcess( void )
{
	// empty.  jba.
}  // end loadPostProcess

//----------------------------------------------------------------------------------------------------------
StateReturnType AIFaceState::onEnter()
{
	Object* source = getMachineOwner();

	AIUpdateInterface* ai = source->getAI();
	Locomotor* curLoco = ai->getCurLocomotor();
	m_canTurnInPlace = curLoco ? curLoco->getMinSpeed() == 0.0f : false;

	Object* target = getMachineGoalObject();
	if (m_obj && target == NULL )
	{
		// Nothing to face...
		return STATE_FAILURE; 
	}

	return STATE_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------
void AIFaceState::onExit( StateExitType status )
{
}

//----------------------------------------------------------------------------------------------------------
StateReturnType AIFaceState::update()
{

	Object *obj = getMachineOwner();
	AIUpdateInterface *ai = obj->getAI();

	const Coord3D* pos = getMachineGoalPosition();
	if (m_obj)
	{
		Object *target = getMachineGoalObject();
		if (!target)
		{
			// Nothing to face.
			return STATE_FAILURE;	
		}
		pos = target->getPosition();
	}
	Real relAngle = ThePartitionManager->getRelativeAngle2D( obj, pos );

	const Real REL_THRESH = 0.035f;	// about 2 degrees. (getRelativeAngle2D is current only accurate to about 1.25 degrees)
	if( fabs( relAngle ) < REL_THRESH )
	{
		return STATE_SUCCESS;
	}

	// turnDelta = yawRate()	NO, do not get this, it is not useful. (srj)

	if (m_canTurnInPlace)
	{
		Real desiredAngle = obj->getOrientation() + relAngle;
		ai->setLocomotorGoalOrientation( desiredAngle );
	}
	else
	{
		ai->setLocomotorGoalPositionExplicit(*pos);
	}

	return STATE_CONTINUE;
}
