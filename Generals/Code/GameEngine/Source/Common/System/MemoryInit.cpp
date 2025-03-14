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

// FILE: MemoryInit.cpp 
//-----------------------------------------------------------------------------
//                                                                          
//                       Westwood Studios Pacific.                          
//                                                                          
//                       Confidential Information                           
//                Copyright (C) 2001 - All Rights Reserved                  
//                                                                          
//-----------------------------------------------------------------------------
//
// Project:   RTS3
//
// File name: MemoryInit.cpp
//
// Created:   Steven Johnson, August 2001
//
// Desc:      Memory manager
//
// ----------------------------------------------------------------------------
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

// SYSTEM INCLUDES

// USER INCLUDES 
#include "Lib/BaseType.h"
#include "Common/GameMemory.h"

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

//-----------------------------------------------------------------------------
void userMemoryManagerGetDmaParms(Int *numSubPools, const PoolInitRec **pParms)
{
	static const PoolInitRec defaultDMA[7] = 
	{
		// name, allocsize, initialcount, overflowcount
		{ "dmaPool_16", 16,			65536,	1024 },
		{ "dmaPool_32", 32,			150000,	1024 },
		{ "dmaPool_64", 64,			60000,	1024 },
		{ "dmaPool_128", 128,		32768,	1024 },
		{ "dmaPool_256", 256,		8192,		1024 },
		{ "dmaPool_512", 512,		8192,		1024 },
		{ "dmaPool_1024", 1024, 24000,	1024 }
	};

	*numSubPools = 7;
	*pParms = defaultDMA;
}

//-----------------------------------------------------------------------------
struct PoolSizeRec
{
	const char* name;
	Int initial;
	Int overflow;
};

//-----------------------------------------------------------------------------
// And please be careful of duplicates.  They are not rejected.
// not const -- we might override from INI
static PoolSizeRec sizes[] = 
{
	{ "PartitionContactListNode", 2048, 512 },
	{ "BattleshipUpdate", 32, 32 },
	{ "FlyToDestAndDestroyUpdate", 32, 32 },
	{ "MusicTrack", 32, 32 },
	{ "PositionalSoundPool", 32, 32 },
	{ "GameMessage", 2048, 32 },
	{ "NameKeyBucketPool", 4096, 32 },
	{ "ObjectSellInfo", 16, 16 },
	{ "ProductionPrerequisitePool", 1024, 32 },
	{ "RadarObject", 512, 32 },
	{ "ResourceGatheringManager", 16, 16 },
	{ "SightingInfo", 8192, 2048 },// Looks big, but all 3000 objects used to have 4 just built in.
	{ "SpecialPowerTemplate", 64, 32 },
	{ "StateMachinePool", 32, 32 },
	{ "TeamPool", 128, 32 },	// if you increase this, increase player/team relation map pools
	{ "PlayerRelationMapPool", 128, 32 },
	{ "TeamRelationMapPool", 128, 32 },
	{ "TeamPrototypePool", 256, 32 },
	{ "TerrainType", 256, 32 },
	{ "ThingTemplatePool", 1200, 32 },
	{ "TunnelTracker", 16, 16 },
	{ "Upgrade", 16, 16 },
	{ "UpgradeTemplate", 128, 16 },
	{ "Anim2D", 32, 32 },
	{ "CommandButton", 300, 16 },
	{ "CommandSet", 256, 16 },
	{ "DisplayString", 32, 32 },
	{ "WebBrowserURL", 16, 16 },
	{ "Drawable", 4096, 32 },
	{ "Image", 2048, 32 },
	{ "ParticlePool", 4096, 256 },  
	{ "ParticleSystemTemplatePool", 768, 32 },  
	{ "ParticleSystemPool", 1024, 32 },  
	{ "TerrainRoadType", 64, 64, },
	{ "WindowLayoutPool", 32, 32 },
	{ "AnimatedParticleSysBoneClientUpdate", 16, 16 },
	{ "SwayClientUpdate", 4096, 32 },
	{ "BeaconClientUpdate", 64, 32 },
	{ "AIGroupPool", 64, 32 },  
	{ "AIDockMachinePool", 256, 32 },
	{ "AIGuardMachinePool", 32, 32 },
	{ "AITNGuardMachinePool", 32, 32 },
	{ "PathNodePool", 8192, 1024 },  
	{ "PathPool", 256, 16 },       
	{ "WorkOrder", 32, 32 },  
	{ "TeamInQueue", 32, 32 },
	{ "AIPlayer", 8, 8 },  
	{ "AISkirmishPlayer", 8, 8 },  
	{ "AIStateMachine",  600, 32 },
	{ "JetAIStateMachine",  64, 32 },
	{ "HeliAIStateMachine",  64, 32 },
	{ "AIAttackMoveStateMachine", 2048, 32 },
	{ "AIAttackThenIdleStateMachine", 512, 32 },
	{ "AttackStateMachine",  512, 32 },
	{ "CrateTemplate", 32, 32 },
	{ "ExperienceTrackerPool", 4096, 256 }, 
	{ "FiringTrackerPool", 4096, 256 }, 
	{ "ObjectRepulsorHelper", 1024, 256 }, 
	{ "ObjectSMCHelperPool", 4096, 256 }, 
	{ "ObjectWeaponStatusHelperPool", 4096, 256 }, 
	{ "ObjectDefectionHelperPool", 4096, 256 }, 
	{ "Locomotor", 2048, 32 },
	{ "LocomotorTemplate", 128, 32	},
	{ "ObjectPool", 4096, 32 },
	{ "SimpleObjectIteratorPool", 32, 32 },
	{ "SimpleObjectIteratorClumpPool", 4096, 32 },
	{ "PartitionDataPool", 4096, 32 },
	{ "BuildEntry", 32, 32 },
	{ "Weapon", 4096, 32 },
	{ "WeaponTemplate", 192, 32 },
	{ "AIUpdateInterface", 600, 32 },
	{ "ActiveBody", 1024, 32 },
	{ "ActiveShroudUpgrade", 32, 32 },
	{ "AssistedTargetingUpdate", 32, 32 },
	{ "AudioEventInfo", 1200, 64 },
	{ "AudioRequest", 256, 8 },
	{ "AutoHealBehavior", 4096, 32 },
	{ "BaseRegenerateUpdate", 64, 32 },
	{ "BoneFXDamage", 64, 32 },
	{ "BoneFXUpdate", 64, 32 },
	{ "BridgeBehavior", 32, 32 },
	{ "BridgeTowerBehavior", 32, 32 },
	{ "BridgeScaffoldBehavior", 32, 32 },
	{ "CaveContain", 16, 16 },
	{ "HealContain", 32, 32 },
	{ "CreateCrateDie", 256, 128 },
	{ "CreateObjectDie", 1024, 32 },
	{ "EjectPilotDie", 1024, 32 },
	{ "CrushDie", 1024, 32 },
	{ "DamDie", 8, 8 },
	{ "DelayedUpgrade", 32, 32 },
	{ "DelayedWeaponSetUpgradeUpdate", 32, 32 },
	{ "DeliverPayloadStateMachine", 32, 32 },
	{ "DeliverPayloadAIUpdate", 32, 32 },
	{ "DeletionUpdate", 128, 32 },
	{ "HackInternetStateMachine", 32, 32 },
	{ "HackInternetAIUpdate", 32, 32 },
	{ "MissileAIUpdate", 512, 32 },
	{ "DumbProjectileBehavior", 64, 32 },
	{ "DestroyDie", 1024, 32 },
	{ "UpgradeDie", 128, 32 },
	{ "KeepObjectDie", 128, 32 },
	{ "DozerAIUpdate", 32, 32 },
	{ "DynamicGeometryInfoUpdate", 16, 16 },
	{ "DynamicShroudClearingRangeUpdate", 128, 16 },
	{ "FXListDie", 1024, 32 },
	{ "FireSpreadUpdate", 2048, 128 },
	{ "FirestormDynamicGeometryInfoUpdate", 16, 16 },
	{ "FireWeaponCollide", 2048, 32 },
	{ "FireWeaponUpdate", 32, 32 },
	{ "FlammableUpdate", 4096, 256 },
	{ "FloatUpdate", 512, 128 },
	{ "TensileFormationUpdate", 256, 32 },
	{ "GarrisonContain", 256, 32 },
	{ "HealCrateCollide", 32, 32 },
	{ "HeightDieUpdate", 32, 32 },
	{ "FireWeaponWhenDamagedBehavior", 32, 32 },
	{ "FireWeaponWhenDeadBehavior", 64, 32 },
	{ "GenerateMinefieldBehavior", 32, 32 },
	{ "HelicopterSlowDeathBehavior", 64, 32 },
	{ "ParkingPlaceBehavior", 32, 32 },
#ifdef ALLOW_SURRENDER
	{ "POWTruckAIUpdate", 32, 32, },
	{ "POWTruckBehavior", 32, 32, },
	{ "PrisonBehavior", 32, 32 },
	{ "PrisonVisual", 32, 32 },
	{ "PropagandaCenterBehavior", 16, 16 },
#endif
	{ "PropagandaTowerBehavior", 16, 16 },
	{ "ObjectTracker", 128, 32 },
	{ "OCLUpdate", 16, 16 },
	{ "BodyParticleSystem", 128, 64 },
	{ "HighlanderBody", 2048, 128 },
	{ "HordeUpdate", 128, 32 },
	{ "ImmortalBody", 2048, 128 },
	{ "InactiveBody", 2048, 32 },
	{ "InstantDeathBehavior", 512, 32 },
	{ "LaserUpdate", 32, 32 },
	{ "PointDefenseLaserUpdate", 32, 32 },
	{ "CleanupHazardUpdate", 32, 32 },
	{ "AutoFindHealingUpdate", 256, 32 },
	{ "CommandButtonHuntUpdate", 512, 8 },
	{ "PilotFindVehicleUpdate", 256, 32 },
	{ "DemoTrapUpdate", 32, 32 },
	{ "ParticleUplinkCannonUpdate", 16, 16 },
	{ "BaikonurLaunchPower", 4, 4 },
	{ "RadiusDecalUpdate", 16, 16 },
	{ "BattlePlanUpdate", 32, 32 },
	{ "LifetimeUpdate", 256, 32 },
	{ "LocomotorSetUpgrade", 512, 128 },
	{ "AutoDepositUpdate", 256, 32 },
	{ "NeutronMissileUpdate", 512, 32 },
	{ "MoneyCrateCollide", 32, 32 },
	{ "NeutronMissileSlowDeathBehavior", 8, 8 },
	{ "OpenContain", 128, 32 },
	{ "OverchargeBehavior", 32, 32 },
	{ "OverlordContain", 32, 32 },
	{ "ParachuteContain", 128, 32 },
	{ "PhysicsBehavior", 600, 32 },
	{ "PoisonedBehavior", 512, 64 },
	{ "ProductionEntry", 32, 32 },
	{ "ProductionUpdate", 256, 32 },
	{ "ProjectileStreamUpdate", 32, 32 },
	{ "ProneUpdate", 128, 32 },
	{ "QueueProductionExitUpdate", 32, 32 },
	{ "RadarUpdate", 16, 16 },
	{ "RadarUpgrade", 16, 16 },
	{ "SupplyWarehouseCripplingBehavior", 16, 16 },
	{ "CostModifierUpgrade", 32, 32 },
	{ "CashBountyPower", 32, 32 },
	{ "CleanupAreaPower", 32, 32 },
	{ "ObjectCreationUpgrade", 128, 32 },
	{ "MinefieldBehavior", 256, 32 },
	{ "JetSlowDeathBehavior", 64, 32 },
	{ "RebuildHoleBehavior", 64, 32 },
	{ "RebuildHoleExposeDie", 64, 32 },
	{ "RepairDockUpdate", 32, 32 },
#ifdef ALLOW_SURRENDER
	{ "PrisonDockUpdate", 32, 32 },
#endif
	{ "RailedTransportDockUpdate", 16, 16 },
	{ "RailedTransportAIUpdate", 16, 16 },
	{ "RailedTransportContain", 16, 16 },
	{ "RailroadBehavior", 16, 16 },
	{ "SalvageCrateCollide", 32, 32 },
	{ "ShroudCrateCollide", 32, 32 },
	{ "SlavedUpdate", 64, 32 },
	{ "SlowDeathBehavior", 4096, 32 },
	{ "SpyVisionUpdate", 16, 16 },
	{ "DefaultProductionExitUpdate", 32, 32 },
	{ "SpawnPointProductionExitUpdate", 32, 32 },
	{ "SpawnBehavior", 32, 32 },
	{ "SpecialPowerCompletionDie", 32, 32 },
	{ "SpecialPowerCreate", 32, 32 },
	{ "PreorderCreate", 32, 32 },
	{ "SpecialAbility", 512, 32 },
	{ "SpecialAbilityUpdate", 512, 32 },
	{ "MissileLauncherBuildingUpdate", 32, 32 },
	{ "SquishCollide", 512, 32 },
	{ "StructureBody", 512, 64 },
	{ "HiveStructureBody", 64, 32 }, //Stinger sites
	{ "StructureCollapseUpdate", 32, 32 },
	{ "StructureToppleUpdate", 32, 32 },
	{ "SupplyCenterCreate", 32, 32 },
	{ "SupplyCenterDockUpdate", 32, 32 },
	{ "SupplyCenterProductionExitUpdate", 32, 32 },
	{ "SupplyTruckStateMachine", 256, 32 },
	{ "SupplyTruckAIUpdate", 32, 32 },
	{ "SupplyWarehouseCreate", 32, 32 },
	{ "SupplyWarehouseDockUpdate", 32, 32 },
	{ "EnemyNearUpdate", 1024, 32 },
	{ "TechBuildingBehavior", 32, 32 },
	{ "ToppleUpdate", 2048, 32 },
	{ "TransitionDamageFX", 256, 32 },
	{ "TransportAIUpdate", 64, 32 },
	{ "TransportContain", 128, 32 },
	{ "TunnelContain", 16, 16 },
	{ "TunnelContainDie", 32, 32 },
	{ "TunnelCreate", 32, 32 },
	{ "TurretAI", 256, 32 },
	{ "TurretStateMachine", 128, 32 },
	{ "TurretSwapUpgrade", 512, 128 },
	{ "UnitCrateCollide", 32, 32 },
	{ "UnpauseSpecialPowerUpgrade", 32, 32 },
	{ "VeterancyCrateCollide", 32, 32 },
	{ "VeterancyGainCreate", 512, 128 },
	{ "ConvertToCarBombCrateCollide", 32, 32 },
	{ "ConvertToHijackedVehicleCrateCollide", 32, 32 },
	{ "JetAIUpdate", 64, 32 },
	{ "ChinookAIUpdate", 32, 32 },
	{ "WanderAIUpdate", 32, 32 },
	{ "WaveGuideUpdate", 16, 16 },
	{ "WeaponBonusUpgrade", 512, 128 },
	{ "WeaponSetUpgrade", 512, 128 },
	{ "ArmorUpgrade", 512, 128 },
	{ "WorkerAIUpdate", 128, 128 },
	{ "WorkerStateMachine", 128, 128 },
	{ "ChinookAIStateMachine", 32, 32 },
	{ "DeployStyleAIUpdate", 32, 32 },
	{ "AssaultTransportAIUpdate", 64, 32 },
	{ "StreamingArchiveFile", 8, 8 },

	{ "DozerActionStateMachine", 256, 32 },
	{ "DozerPrimaryStateMachine", 256, 32 },
	{ "W3DDisplayString", 1024, 128 },
	{ "W3DDefaultDraw", 1024, 128 },
	{ "W3DDebrisDraw", 1024, 128 },
	{ "W3DDependencyModelDraw", 64, 64 },
	{ "W3DLaserDraw", 32, 32 },
	{ "W3DModelDraw", 4096, 128 },
	{ "W3DOverlordTankDraw", 64, 64 },
	{ "W3DPoliceCarDraw", 32, 32 },
	{ "W3DProjectileStreamDraw", 32, 32 },
	{ "W3DRopeDraw", 32, 32 },
	{ "W3DScienceModelDraw", 32, 32 },
	{ "W3DSupplyDraw", 32, 32 },
	{ "W3DTankDraw", 256, 32 },
	{ "W3DTracerDraw", 64, 32 },
	{ "W3DTruckDraw", 128, 32 },
	{ "W3DTankTruckDraw", 32, 16 },
	{ "DefaultSpecialPower", 32, 32 },
	{ "OCLSpecialPower", 32, 32 },
#ifdef ALLOW_DEMORALIZE
	{ "DemoralizeSpecialPower", 16, 16, },
#endif
	{ "CashHackSpecialPower", 32, 32 },
	{ "CommandSetUpgrade", 32, 32 },
	{ "GrantUpgradeCreate", 256, 32 },
	{ "SpyVisionSpecialPower", 256, 32 },
	{ "StealthDetectorUpdate", 256, 32 },
	{ "StealthUpdate", 256, 32 },
	{ "StealthUpgrade", 256, 32 },
	{ "StatusBitsUpgrade", 128, 128 },
	{ "SubObjectsUpgrade", 128, 128 },
	{ "ExperienceScalarUpgrade", 256, 128 },
	{ "MaxHealthUpgrade", 128, 128 },
	{ "WeaponBonusUpgrade", 128, 64 },
	{ "StickyBombUpdate", 64, 32 },
	{ "FireOCLAfterWeaponCooldownUpdate", 64, 32 },
	{ "HijackerUpdate", 64, 32 },
	{ "ChinaMinesUpgrade", 64, 32 },
	{ "PowerPlantUpdate", 16, 16 },
	{ "PowerPlantUpgrade", 16, 16 },
	{ "DefectorSpecialPower", 16, 16 },
	{ "CheckpointUpdate", 16, 16 },
	{ "MobNexusContain", 128, 32 },
	{ "MobMemberSlavedUpdate", 64, 32 },
	{ "EMPUpdate", 64, 32 },
	{ "Overridable", 32, 32 },

	{ "W3DGameWindow", 1024, 32 },
	{ "SuccessState", 32, 32 },
	{ "FailureState", 32, 32 },
	{ "ContinueState", 32, 32 },
	{ "SleepState", 32, 32 },

	{ "AIDockWaitForClearanceState", 256, 32 },
	{ "AIDockProcessDockState", 256, 32 },
	{ "AIGuardInnerState", 32, 32 },
	{ "AIGuardIdleState", 32, 32 },
	{ "AIGuardOuterState", 32, 32 },
	{ "AIGuardReturnState", 32, 32 },
	{ "AIGuardPickUpCrateState", 32, 32 },
	{ "AIGuardAttackAggressorState", 32, 32 },
	{ "AITNGuardInnerState", 32, 32 },
	{ "AITNGuardIdleState", 32, 32 },
	{ "AITNGuardOuterState", 32, 32 },
	{ "AITNGuardReturnState", 32, 32 },
	{ "AITNGuardPickUpCrateState", 32, 32 },
	{ "AITNGuardAttackAggressorState", 32, 32 },
	{ "AIIdleState", 2400, 32 },
	{ "AIRappelState", 600, 32 },
	{ "AIBusyState", 600, 32 },
	{ "AIWaitState", 600, 32 },
	{ "AIAttackState", 4096, 32 },
	{ "AIAttackSquadState", 600, 32 },
	{ "AIDeadState", 600, 32 },
	{ "AIDockState", 600, 32 },
	{ "AIExitState", 600, 32 },
	{ "AIGuardState", 600, 32 },
	{ "AITunnelNetworkGuardState", 600, 32 },
	{ "AIHuntState", 600, 32 },
	{ "AIAttackAreaState", 600, 32 },
	{ "AIFaceState", 1200, 32 },
	{ "ApproachState", 600, 32 },
	{ "DeliveringState", 600, 32 },
	{ "ConsiderNewApproachState", 600, 32 },
	{ "RecoverFromOffMapState", 600, 32 },
	{ "HeadOffMapState", 600, 32 },
	{ "CleanUpState", 600, 32 },
	{ "HackInternetState", 600, 32 },
	{ "PackingState", 600, 32 },
	{ "UnpackingState", 600, 32 },
	{ "SupplyTruckWantsToPickUpOrDeliverBoxesState", 600, 32 },
	{ "RegroupingState", 600, 32 },
	{ "DockingState", 600, 32 },
	{ "ChinookEvacuateState", 32, 32 },
	{ "ChinookHeadOffMapState", 32, 32 },
	{ "ChinookTakeoffOrLandingState", 32, 32 },
	{ "ChinookCombatDropState", 32, 32 },
	{ "DozerActionPickActionPosState", 256, 32 },
	{ "DozerActionMoveToActionPosState", 256, 32 },
	{ "DozerActionDoActionState", 256, 32 },
	{ "DozerPrimaryIdleState", 256, 32 },
	{ "DozerActionState", 256, 32 },
	{ "DozerPrimaryGoingHomeState", 256, 32 },
	{ "JetAwaitingRunwayState", 64, 32 },
	{ "JetOrHeliCirclingDeadAirfieldState", 64, 32 },
	{ "HeliTakeoffOrLandingState", 64, 32 },
	{ "JetOrHeliParkOrientState", 64, 32 },
	{ "JetOrHeliReloadAmmoState", 64, 32 },
	{ "SupplyTruckBusyState", 600, 32 },
	{ "SupplyTruckIdleState", 600, 32 },
	{ "ActAsDozerState", 600, 32 },
	{ "ActAsSupplyTruckState", 600, 32 },
	{ "AIDockApproachState", 256, 32 },
	{ "AIDockAdvancePositionState", 256, 32 },
	{ "AIDockMoveToEntryState", 256, 32 },
	{ "AIDockMoveToDockState", 256, 32 },
	{ "AIDockMoveToExitState", 256, 32 },
	{ "AIDockMoveToRallyState", 256, 32 },
	{ "AIMoveToState", 600, 32 },
	{ "AIMoveOutOfTheWayState", 600, 32 },
	{ "AIMoveAndTightenState", 600, 32 },
	{ "AIMoveAwayFromRepulsorsState", 600, 32 },
	{ "AIAttackApproachTargetState", 96, 32 },
	{ "AIAttackPursueTargetState", 96, 32 },
	{ "AIAttackAimAtTargetState", 96, 32 },
	{ "AIAttackFireWeaponState", 256, 32 },
	{ "AIPickUpCrateState", 4096, 32 },
	{ "AIFollowWaypointPathState", 1200, 32 },
	{ "AIFollowWaypointPathExactState", 1200, 32 },
	{ "AIWanderInPlaceState", 600, 32 },
	{ "AIFollowPathState", 1200, 32 },
	{ "AIMoveAndEvacuateState", 1200, 32 },
	{ "AIMoveAndDeleteState", 600, 32 },
	{ "AIEnterState", 600, 32 },
	{ "JetOrHeliReturningToDeadAirfieldState", 64, 32 },
	{ "JetOrHeliReturnForLandingState", 64, 32 },
	{ "TurretAIIdleState", 600, 32 },
	{ "TurretAIIdleScanState", 600, 32 },
	{ "TurretAIAimTurretState", 600, 32 },
	{ "TurretAIRecenterTurretState", 600, 32 },
	{ "TurretAIHoldTurretState", 600, 32 },
	{ "JetOrHeliTaxiState", 64, 32 },
	{ "JetTakeoffOrLandingState", 64, 32 },
	{ "JetPauseBeforeTakeoffState", 64, 32 },
	{ "AIAttackMoveToState", 600, 32 },
	{ "AIAttackFollowWaypointPathState", 1200, 32 },
	{ "AIWanderState", 600, 32 },
	{ "AIPanicState", 600, 32 },
	{ "ChinookMoveToBldgState", 32, 32 },
	{ "ScienceInfo", 64, 32 },
	{ "RankInfo", 32, 32 },

	{ "FireWeaponNugget", 32, 32 },
	{ "AttackNugget", 32, 32 },
	{ "DeliverPayloadNugget", 32, 32 },
	{ "ApplyRandomForceNugget", 32, 32 },
	{ "GenericObjectCreationNugget", 512, 32 },
	{ "SoundFXNugget", 256, 32 },
	{ "TracerFXNugget", 32, 32 },
	{ "RayEffectFXNugget", 32, 32 },
	{ "LightPulseFXNugget", 64, 32 },
	{ "ViewShakeFXNugget", 128, 32 },
	{ "TerrainScorchFXNugget", 32, 32 },
	{ "ParticleSystemFXNugget", 600, 32 },
	{ "FXListAtBonePosFXNugget", 32, 32 },
	{ "Squad", 256, 32 },
	{ "BuildListInfo", 256, 32 },

	{ "ScriptGroup", 128, 32 },
	{ "OrCondition", 1024, 256 },
	{ "ScriptAction", 2048, 512 },
	{ "Script", 1024, 256 },
	{ "Parameter", 8192, 1024 },
	{ "Condition", 2048, 256 },
	{ "Template", 32, 32 },
	{ "ScriptList", 32, 32 },
	{ "AttackPriorityInfo", 32, 32 },
	{ "SequentialScript", 32, 32 },
	{ "Win32LocalFile", 1024, 256 },
	{ "RAMFile", 32, 32 },
	{ "BattlePlanBonuses", 32, 32 },
	{ "KindOfPercentProductionChange", 32, 32 },
	{ "UserParser", 4096, 256 },
	{ "XferBlockData", 32, 32 },
	{ "EvaCheckInfo", 32, 32 },
	{ "SuperweaponInfo", 32, 32 },
	{ "NamedTimerInfo", 32, 32 },
	{ "PopupMessageData", 32, 32 },
	{ "FloatingTextData", 32, 32 },
	{ "MapObject", 4096, 32 },
	{ "Waypoint", 1024, 32 },
	{ "PolygonTrigger", 128, 32 },
	{ "Bridge", 32, 32 },
	{ "Mapping", 128, 32 },
	{ "OutputChunk", 32, 32 },
	{ "InputChunk", 32, 32 },
	{ "AnimateWindow", 32, 32 },
	{ "GameFont", 32, 32 },
	{ "NetCommandRef", 256, 32 },
	{ "GameMessageArgument", 128, 32 },
	{ "GameMessageParserArgumentType", 32, 32 },
	{ "GameMessageParser", 32, 32 },
	{ "WeaponBonusSet", 32, 32 },
	{ "Campaign", 32, 32 },
	{ "Mission", 32, 32 },
	{ "ModalWindow", 32, 32 },
	{ "NetPacket", 32, 32 },
	{ "AISideInfo", 32, 32 },
	{ "AISideBuildList", 32, 32 },
	{ "MetaMapRec", 256, 32 },
	{ "TransportStatus", 32, 32 },
	{ "Anim2DTemplate", 32, 32 },
	{ "ObjectTypes", 32, 32 },
	{ "NetCommandList", 512, 32 },
	{ "TurretAIData", 256, 32 },
	{ "NetCommandMsg", 32, 32 },
	{ "NetGameCommandMsg", 64, 32 },
	{ "NetAckBothCommandMsg", 32, 32 },
	{ "NetAckStage1CommandMsg", 32, 32 },
	{ "NetAckStage2CommandMsg", 32, 32 },
	{ "NetFrameCommandMsg", 32, 32 },
	{ "NetPlayerLeaveCommandMsg", 32, 32 },
	{ "NetRunAheadMetricsCommandMsg", 32, 32 },
	{ "NetRunAheadCommandMsg", 32, 32 },
	{ "NetDestroyPlayerCommandMsg", 32, 32 },
	{ "NetDisconnectFrameCommandMsg", 32, 32 },
	{ "NetDisconnectScreenOffCommandMsg", 32, 32 },
	{ "NetFrameResendRequestCommandMsg", 32, 32 },
	{ "NetKeepAliveCommandMsg", 32, 32 },
	{ "NetDisconnectKeepAliveCommandMsg", 32, 32 },
	{ "NetDisconnectPlayerCommandMsg", 32, 32 },
	{ "NetPacketRouterQueryCommandMsg", 32, 32 },
	{ "NetPacketRouterAckCommandMsg", 32, 32 },
	{ "NetDisconnectChatCommandMsg", 32, 32 },
	{ "NetChatCommandMsg", 32, 32 },
	{ "NetDisconnectVoteCommandMsg", 32, 32 },
	{ "NetProgressCommandMsg", 32, 32 },
	{ "NetWrapperCommandMsg", 32, 32 },
	{ "NetFileCommandMsg", 32, 32 },
	{ "NetFileAnnounceCommandMsg", 32, 32 },
	{ "NetFileProgressCommandMsg", 32, 32 },
	{ "NetCommandWrapperListNode", 32, 32 },
	{ "NetCommandWrapperList", 32, 32 },
	{ "Connection", 32, 32 },
	{ "User", 32, 32 },
	{ "FrameDataManager", 32, 32 },
	{ "DrawableIconInfo", 32, 32 },
	{ "TintEnvelope", 128, 32 },
	{ "DynamicAudioEventRTS", 1024, 256 },
	{ "DrawableLocoInfo", 128, 32 },
	{ "W3DPrototypeClass", 2048, 32 },
	{ "EnumeratedIP", 32, 32 },
	{ "WaterTransparencySetting", 4, 4 },
	

	// W3D pools!
	{ "BoxPrototypeClass", 512, 32 },
	{ "SpherePrototypeClass", 32, 32 },
	{ "SoundRenderObjPrototypeClass", 32, 32 },
	{ "RingPrototypeClass", 32, 32 },
	{ "PrimitivePrototypeClass", 8192, 32 },
	{ "HModelPrototypeClass", 256, 32 },
	{ "ParticleEmitterPrototypeClass", 32, 32 },
	{ "NullPrototypeClass", 32, 32 },
	{ "HLodPrototypeClass", 512, 32 },
	{ "DistLODPrototypeClass", 32, 32 },
	{ "DazzlePrototypeClass", 32, 32 },
	{ "CollectionPrototypeClass", 32, 32 },
	{ "BoxPrototypeClass", 256, 32 },
	{ "AggregatePrototypeClass", 32, 32 },
	{ "OBBoxRenderObjClass", 16384, 32 },
	{ "AABoxRenderObjClass", 32, 32 },
	{ "VertexMaterialClass", 16384, 32 },
	{ "TextureClass", 1024, 32 },
	{ "CloudMapTerrainTextureClass", 32, 32 },
	{ "ScorchTextureClass", 32, 32 },
	{ "LightMapTerrainTextureClass", 32, 32 },
	{ "AlphaEdgeTextureClass", 32, 32 },
	{ "AlphaTerrainTextureClass", 32, 32 },
	{ "TerrainTextureClass", 32, 32 },
	{ "MeshClass", 16384, 1024 },
	{ "HTreeClass", 8192, 32 },
	{ "HLodDefClass", 512, 32 },
	{ "HLodClass", 4096, 32 },
	{ "MeshModelClass", 8192, 32 },
	{ "ShareBufferClass", 32768, 1024 },
	{ "AABTreeClass", 32, 32 },
	{ "MotionChannelClass", 16384, 32 },
	{ "BitChannelClass", 64, 32 },
	{ "TimeCodedMotionChannelClass", 32, 32 },
	{ "AdaptiveDeltaMotionChannelClass", 32, 32 },
	{ "TimeCodedBitChannelClass", 32, 32 },
	{ "UVBufferClass", 8192, 32 },
	{ "TexBufferClass", 512, 32 },
	{ "MatBufferClass", 512, 32 },
	{ "MatrixMapperClass", 32, 32 },
	{ "ScaleTextureMapperClass", 32, 32 },
	{ "LinearOffsetTextureMapperClass", 32, 32 },
	{ "GridTextureMapperClass", 32, 32 },
	{ "RotateTextureMapperClass", 32, 32 },
	{ "SineLinearOffsetTextureMapperClass", 32, 32 },
	{ "StepLinearOffsetTextureMapperClass", 32, 32 },
	{ "ZigZagLinearOffsetTextureMapperClass", 32, 32 },
	{ "ClassicEnvironmentMapperClass", 32, 32 },
	{ "EnvironmentMapperClass", 256, 32 },
	{ "EdgeMapperClass", 32, 32 },
	{ "WSClassicEnvironmentMapperClass", 32, 32 },
	{ "WSEnvironmentMapperClass", 32, 32 },
	{ "GridClassicEnvironmentMapperClass", 32, 32 },
	{ "GridEnvironmentMapperClass", 32, 32 },
	{ "ScreenMapperClass", 32, 32 },
	{ "RandomTextureMapperClass", 32, 32 },
	{ "BumpEnvTextureMapperClass", 32, 32 },
	{ "MeshLoadContextClass", 32, 32 },
	{ "MaterialInfoClass", 8192, 32 },
	{ "MeshMatDescClass", 8192, 32 },
	{ "TextureLoadTaskClass", 256, 32 },
	{ "SortingNodeStruct", 256, 32 },
	{ "ProxyArrayClass", 32, 32 },
	{ "Line3DClass", 128, 32 },
	{ "Render2DClass", 64, 32 },
	{ "SurfaceClass", 128, 32 },
	{ "FontCharsClassCharDataStruct", 1024, 32 },
	{ "FontCharsBuffer", 16, 4 },
	{ "FVFInfoClass", 128, 32 },
	{ "TerrainTracksRenderObjClass", 128, 32 },
	{ "DynamicIBAccessClass", 32, 32 },
	{ "DX8IndexBufferClass", 128, 32 },
	{ "SortingIndexBufferClass", 32, 32 },
	{ "DX8VertexBufferClass", 128, 32 },
	{ "SortingVertexBufferClass", 32, 32 },
	{ "DynD3DMATERIAL8", 8192, 32 }, 
	{ "DynamicMatrix3D", 512, 32 },
	{ "MeshGeometryClass", 32, 32 },
	{ "DynamicMeshModel", 32, 32 },
	{ "GapFillerClass", 32, 32 },
	{ "FontCharsClass", 64, 32 },

	{ 0, 0, 0 }
};

//-----------------------------------------------------------------------------
void userMemoryAdjustPoolSize(const char *poolName, Int& initialAllocationCount, Int& overflowAllocationCount)
{
	if (initialAllocationCount > 0)
		return;

	for (const PoolSizeRec* p = sizes; p->name != NULL; ++p)
	{
		if (strcmp(p->name, poolName) == 0)
		{
			initialAllocationCount = p->initial;
			overflowAllocationCount = p->overflow;
			return;
		}
	}

	DEBUG_CRASH(("Initial size for pool %s not found -- you should add it to MemoryInit.cpp\n",poolName));
}

//-----------------------------------------------------------------------------
static Int roundUpMemBound(Int i)
{
	const int MEM_BOUND_ALIGNMENT = 4;

	if (i < MEM_BOUND_ALIGNMENT)
		return MEM_BOUND_ALIGNMENT;
	else
		return (i + (MEM_BOUND_ALIGNMENT-1)) & ~(MEM_BOUND_ALIGNMENT-1);
}

//-----------------------------------------------------------------------------
void userMemoryManagerInitPools()
{
	// note that we MUST use stdio stuff here, and not the normal game file system
	// (with bigfile support, etc), because that relies on memory pools, which
	// aren't yet initialized properly! so rely ONLY on straight stdio stuff here.
	// (not even AsciiString. thanks.)
	
	// since we're called prior to main, the cur dir might not be what
	// we expect. so do it the hard way.
	char buf[_MAX_PATH];
	::GetModuleFileName(NULL, buf, sizeof(buf));
	char* pEnd = buf + strlen(buf);
	while (pEnd != buf) 
	{
		if (*pEnd == '\\') 
		{
			*pEnd = 0;
			break;
		}
		--pEnd;
	}
	strcat(buf, "\\Data\\INI\\MemoryPools.ini");

	FILE* fp = fopen(buf, "r");
	if (fp)
	{
		char poolName[256];
		int initial, overflow;
		while (fgets(buf, _MAX_PATH, fp))
		{
			if (buf[0] == ';')
				continue;
			if (sscanf(buf, "%s %d %d", poolName, &initial, &overflow ) == 3)
			{
				for (PoolSizeRec* p = sizes; p->name != NULL; ++p)
				{
					if (strcasecmp(p->name, poolName) == 0)
					{
						// currently, these must be multiples of 4. so round up.
						p->initial = roundUpMemBound(initial);
						p->overflow = roundUpMemBound(overflow);
						break;	// from for-p
					}
				}
			}
		}
		fclose(fp);
	}
}

