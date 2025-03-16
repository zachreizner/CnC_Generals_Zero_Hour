cmake_minimum_required(VERSION 3.25)
project(generals LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -Wall \
    -Wno-char-subscripts \
    -Wno-conversion-null \
    -Wno-deprecated \
    -Wno-deprecated-copy \
    -Wno-endif-labels \
    -Wno-enum-compare \
    -Wno-ignored-qualifiers \
    -Wno-implicit-exception-spec-mismatch \
    -Wno-implicit-fallthrough \
    -Wno-int-to-void-pointer-cast \
    -Wno-invalid-offsetof \
    -Wno-logical-not-parentheses \
    -Wno-logical-not-parentheses \
    -Wno-narrowing \
    -Wno-overloaded-virtual \
    -Wno-parentheses \
    -Wno-pointer-bool-conversion \
    -Wno-reorder \
    -Wno-sign-compare \
    -Wno-sometimes-uninitialized \
    -Wno-switch \
    -Wno-tautological-constant-out-of-range-compare \
    -Wno-type-limits \
    -Wno-undefined-bool-conversion \
    -Wno-undefined-var-template \
    -Wno-unknown-pragmas \
    -Wno-unused-but-set-variable \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-unused-private-field \
    -Wno-unused-value \
    -Wno-unused-variable \
    -Wno-write-strings \
")

add_library(generals_lib STATIC)

set(GENERALS_SRC
    Code/GameEngine/Source/Common/Audio/AudioEventRTS.cpp
    Code/GameEngine/Source/Common/Audio/AudioRequest.cpp
    Code/GameEngine/Source/Common/Audio/GameAudio.cpp
    Code/GameEngine/Source/Common/Audio/GameMusic.cpp
    # Code/GameEngine/Source/Common/Audio/GameSounds.cpp
    Code/GameEngine/Source/Common/INI/INI.cpp
    Code/GameEngine/Source/Common/INI/INIAiData.cpp
    Code/GameEngine/Source/Common/INI/INIAnimation.cpp
    Code/GameEngine/Source/Common/INI/INIAudioEventInfo.cpp
    Code/GameEngine/Source/Common/INI/INICommandButton.cpp
    Code/GameEngine/Source/Common/INI/INICommandSet.cpp
    Code/GameEngine/Source/Common/INI/INIControlBarScheme.cpp
    Code/GameEngine/Source/Common/INI/INICrate.cpp
    Code/GameEngine/Source/Common/INI/INIDamageFX.cpp
    Code/GameEngine/Source/Common/INI/INIDrawGroupInfo.cpp
    Code/GameEngine/Source/Common/INI/INIGameData.cpp
    Code/GameEngine/Source/Common/INI/INIMapCache.cpp
    Code/GameEngine/Source/Common/INI/INIMapData.cpp
    Code/GameEngine/Source/Common/INI/INIMappedImage.cpp
    Code/GameEngine/Source/Common/INI/INIMiscAudio.cpp
    Code/GameEngine/Source/Common/INI/INIModel.cpp
    Code/GameEngine/Source/Common/INI/INIMultiplayer.cpp
    Code/GameEngine/Source/Common/INI/INIObject.cpp
    Code/GameEngine/Source/Common/INI/INIParticleSys.cpp
    Code/GameEngine/Source/Common/INI/INISpecialPower.cpp
    Code/GameEngine/Source/Common/INI/INITerrain.cpp
    Code/GameEngine/Source/Common/INI/INITerrainBridge.cpp
    Code/GameEngine/Source/Common/INI/INITerrainRoad.cpp
    Code/GameEngine/Source/Common/INI/INIUpgrade.cpp
    Code/GameEngine/Source/Common/INI/INIVideo.cpp
    Code/GameEngine/Source/Common/INI/INIWater.cpp
    Code/GameEngine/Source/Common/INI/INIWeapon.cpp
    Code/GameEngine/Source/Common/INI/INIWebpageURL.cpp
    Code/GameEngine/Source/Common/RTS/ActionManager.cpp
    Code/GameEngine/Source/Common/RTS/Energy.cpp
    Code/GameEngine/Source/Common/RTS/Handicap.cpp
    Code/GameEngine/Source/Common/RTS/MissionStats.cpp
    Code/GameEngine/Source/Common/RTS/Money.cpp
    Code/GameEngine/Source/Common/RTS/Player.cpp
    Code/GameEngine/Source/Common/RTS/PlayerList.cpp
    Code/GameEngine/Source/Common/RTS/PlayerTemplate.cpp
    Code/GameEngine/Source/Common/RTS/ProductionPrerequisite.cpp
    Code/GameEngine/Source/Common/RTS/ResourceGatheringManager.cpp
    Code/GameEngine/Source/Common/RTS/Science.cpp
    Code/GameEngine/Source/Common/RTS/ScoreKeeper.cpp
    Code/GameEngine/Source/Common/RTS/SpecialPower.cpp
    Code/GameEngine/Source/Common/RTS/Team.cpp
    Code/GameEngine/Source/Common/RTS/TunnelTracker.cpp
    Code/GameEngine/Source/Common/Thing/DrawModule.cpp
    Code/GameEngine/Source/Common/Thing/Module.cpp
    Code/GameEngine/Source/Common/Thing/ModuleFactory.cpp
    Code/GameEngine/Source/Common/Thing/Thing.cpp
    Code/GameEngine/Source/Common/Thing/ThingFactory.cpp
    Code/GameEngine/Source/Common/Thing/ThingTemplate.cpp
    Code/GameEngine/Source/Common/System/ArchiveFile.cpp
    Code/GameEngine/Source/Common/System/ArchiveFileSystem.cpp
    Code/GameEngine/Source/Common/System/AsciiString.cpp
    Code/GameEngine/Source/Common/System/BuildAssistant.cpp
    Code/GameEngine/Source/Common/System/CDManager.cpp
    Code/GameEngine/Source/Common/System/Compression.cpp
    Code/GameEngine/Source/Common/System/CopyProtection.cpp
    Code/GameEngine/Source/Common/System/CriticalSection.cpp
    Code/GameEngine/Source/Common/System/DataChunk.cpp
    Code/GameEngine/Source/Common/System/Directory.cpp
    Code/GameEngine/Source/Common/System/DisabledTypes.cpp
    Code/GameEngine/Source/Common/System/File.cpp
    Code/GameEngine/Source/Common/System/FileSystem.cpp
    Code/GameEngine/Source/Common/System/FunctionLexicon.cpp
    Code/GameEngine/Source/Common/System/GameCommon.cpp
    Code/GameEngine/Source/Common/System/GameMemory.cpp
    Code/GameEngine/Source/Common/System/GameType.cpp
    Code/GameEngine/Source/Common/System/Geometry.cpp
    Code/GameEngine/Source/Common/System/KindOf.cpp
    Code/GameEngine/Source/Common/System/List.cpp
    Code/GameEngine/Source/Common/System/LocalFileSystem.cpp
    Code/GameEngine/Source/Common/System/MemoryInit.cpp
    Code/GameEngine/Source/Common/System/QuickTrig.cpp
    Code/GameEngine/Source/Common/System/QuotedPrintable.cpp
    Code/GameEngine/Source/Common/System/RAMFile.cpp
    Code/GameEngine/Source/Common/System/Radar.cpp
    Code/GameEngine/Source/Common/System/Snapshot.cpp
    Code/GameEngine/Source/Common/System/StackDump.cpp
    Code/GameEngine/Source/Common/System/StreamingArchiveFile.cpp
    Code/GameEngine/Source/Common/System/String.cpp
    Code/GameEngine/Source/Common/System/SubsystemInterface.cpp
    Code/GameEngine/Source/Common/System/Trig.cpp
    Code/GameEngine/Source/Common/System/UnicodeString.cpp
    Code/GameEngine/Source/Common/System/Upgrade.cpp
    Code/GameEngine/Source/Common/System/Xfer.cpp
    Code/GameEngine/Source/Common/System/XferCRC.cpp
    Code/GameEngine/Source/Common/System/XferLoad.cpp
    Code/GameEngine/Source/Common/System/XferSave.cpp
    Code/GameEngine/Source/Common/System/encrypt.cpp
    Code/GameEngine/Source/Common/BitFlags.cpp
    Code/GameEngine/Source/Common/CRCDebug.cpp
    Code/GameEngine/Source/Common/CommandLine.cpp
    Code/GameEngine/Source/Common/DamageFX.cpp
    Code/GameEngine/Source/Common/Dict.cpp
    Code/GameEngine/Source/Common/DiscreteCircle.cpp
    Code/GameEngine/Source/Common/GameEngine.cpp
    Code/GameEngine/Source/Common/GameLOD.cpp
    Code/GameEngine/Source/Common/GameMain.cpp
    Code/GameEngine/Source/Common/GlobalData.cpp
    Code/GameEngine/Source/Common/Language.cpp
    Code/GameEngine/Source/Common/MessageStream.cpp
    Code/GameEngine/Source/Common/MiniLog.cpp
    Code/GameEngine/Source/Common/MultiplayerSettings.cpp
    Code/GameEngine/Source/Common/NameKeyGenerator.cpp
    Code/GameEngine/Source/Common/PartitionSolver.cpp
    Code/GameEngine/Source/Common/PerfTimer.cpp
    Code/GameEngine/Source/Common/RandomValue.cpp
    Code/GameEngine/Source/Common/Recorder.cpp
    Code/GameEngine/Source/Common/SkirmishBattleHonors.cpp
    Code/GameEngine/Source/Common/StateMachine.cpp
    Code/GameEngine/Source/Common/StatsCollector.cpp
    Code/GameEngine/Source/Common/TerrainTypes.cpp
    Code/GameEngine/Source/Common/UserPreferences.cpp
    Code/GameEngine/Source/Common/crc.cpp
    Code/GameEngine/Source/Common/version.cpp
    Code/GameEngine/Source/GameLogic/Object/Armor.cpp
    Code/GameEngine/Source/GameLogic/Object/ExperienceTracker.cpp
    Code/GameEngine/Source/GameLogic/Object/FiringTracker.cpp
    Code/GameEngine/Source/GameLogic/Object/GhostObject.cpp
    Code/GameEngine/Source/GameLogic/Object/Locomotor.cpp
    Code/GameEngine/Source/GameLogic/Object/Object.cpp
    Code/GameEngine/Source/GameLogic/Object/ObjectCreationList.cpp
    Code/GameEngine/Source/GameLogic/Object/ObjectTypes.cpp
    Code/GameEngine/Source/GameLogic/Object/PartitionManager.cpp
    Code/GameEngine/Source/GameLogic/Object/SimpleObjectIterator.cpp
    Code/GameEngine/Source/GameLogic/Object/Weapon.cpp
    Code/GameEngine/Source/GameLogic/Object/WeaponSet.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/AutoHealBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/BehaviorModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/BridgeBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/BridgeScaffoldBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/BridgeTowerBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/DumbProjectileBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/FireWeaponWhenDamagedBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/FireWeaponWhenDeadBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/GenerateMinefieldBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/InstantDeathBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/JetSlowDeathBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/MinefieldBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/OverchargeBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/POWTruckBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/ParkingPlaceBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/PoisonedBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/PrisonBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/PropagandaCenterBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/PropagandaTowerBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/RebuildHoleBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/SlowDeathBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/SpawnBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/SupplyWarehouseCripplingBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Behavior/TechBuildingBehavior.cpp
    Code/GameEngine/Source/GameLogic/Object/Body/ActiveBody.cpp
    Code/GameEngine/Source/GameLogic/Object/Body/BodyModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Body/HighlanderBody.cpp
    Code/GameEngine/Source/GameLogic/Object/Body/HiveStructureBody.cpp
    Code/GameEngine/Source/GameLogic/Object/Body/ImmortalBody.cpp
    Code/GameEngine/Source/GameLogic/Object/Body/InactiveBody.cpp
    Code/GameEngine/Source/GameLogic/Object/Body/StructureBody.cpp
    Code/GameEngine/Source/GameLogic/Object/Collide/CollideModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Collide/FireWeaponCollide.cpp
    Code/GameEngine/Source/GameLogic/Object/Collide/SquishCollide.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/CaveContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/GarrisonContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/HealContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/MobNexusContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/OpenContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/OverlordContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/ParachuteContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/RailedTransportContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/TransportContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Contain/TunnelContain.cpp
    Code/GameEngine/Source/GameLogic/Object/Create/CreateModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Create/GrantUpgradeCreate.cpp
    Code/GameEngine/Source/GameLogic/Object/Create/PreorderCreate.cpp
    Code/GameEngine/Source/GameLogic/Object/Create/SpecialPowerCreate.cpp
    Code/GameEngine/Source/GameLogic/Object/Create/SupplyCenterCreate.cpp
    Code/GameEngine/Source/GameLogic/Object/Create/SupplyWarehouseCreate.cpp
    Code/GameEngine/Source/GameLogic/Object/Create/VeterancyGainCreate.cpp
    Code/GameEngine/Source/GameLogic/Object/Damage/BoneFXDamage.cpp
    Code/GameEngine/Source/GameLogic/Object/Damage/DamageModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Damage/TransitionDamageFX.cpp
    Code/GameEngine/Source/GameLogic/Object/Destroy/DestroyModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/CreateCrateDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/CreateObjectDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/CrushDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/DamDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/DestroyDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/DieModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/EjectPilotDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/FXListDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/KeepObjectDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/RebuildHoleExposeDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/SpecialPowerCompletionDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Die/UpgradeDie.cpp
    Code/GameEngine/Source/GameLogic/Object/Helper/ObjectDefectionHelper.cpp
    Code/GameEngine/Source/GameLogic/Object/Helper/ObjectHelper.cpp
    Code/GameEngine/Source/GameLogic/Object/Helper/ObjectRepulsorHelper.cpp
    Code/GameEngine/Source/GameLogic/Object/Helper/ObjectSMCHelper.cpp
    Code/GameEngine/Source/GameLogic/Object/Helper/ObjectWeaponStatusHelper.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/BaikonurLaunchPower.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/CashBountyPower.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/CashHackSpecialPower.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/CleanupAreaPower.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/DefectorSpecialPower.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/DemoralizeSpecialPower.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/OCLSpecialPower.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/SpecialAbility.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/SpecialPowerModule.cpp
    Code/GameEngine/Source/GameLogic/Object/SpecialPower/SpyVisionSpecialPower.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/AIUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/AssistedTargetingUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/AutoDepositUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/AutoFindHealingUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/BaseRenerateUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/BattlePlanUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/BoneFXUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/CheckpointUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/CleanupHazardUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/CommandButtonHuntUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/DelayedWeaponSetUpgradeUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/DeletionUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/DemoTrapUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/DynamicGeometryInfoUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/DynamicShroudClearingRangeUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/EMPUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/EnemyNearUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/FireOCLAfterWeaponCooldownUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/FireSpreadUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/FireWeaponUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/FirestormDynamicGeometryInfoUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/FlammableUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/FloatUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/HeightDieUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/HelicopterSlowDeathUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/HijackerUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/HordeUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/LaserUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/LifetimeUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/MissileLauncherBuildingUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/MobMemberSlavedUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/NeutronMissileSlowDeathUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/NeutronMissileUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/OCLUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/ParticleUplinkCannonUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/PhysicsUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/PilotFindVehicleUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/PointDefenseLaserUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/PowerPlantUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/ProductionUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/ProjectileStreamUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/ProneUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/RadarUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/RadiusDecalUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/SlavedUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/SpecialAbilityUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/SpyVisionUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/StealthDetectorUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/StealthUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/StickyBombUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/StructureCollapseUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/StructureToppleUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/TensileFormationUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/ToppleUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/UpdateModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Update/WaveGuideUpdate.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/ActiveShroudUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/ArmorUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/CommandSetUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/CostModifierUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/DelayedUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/ExperienceScalarUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/LocomotorSetUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/MaxHealthUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/ObjectCreationUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/PowerPlantUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/RadarUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/StatusBitsUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/StealthUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/SubObjectsUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/UnpauseSpecialPowerUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/UpgradeModule.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/WeaponBonusUpgrade.cpp
    Code/GameEngine/Source/GameLogic/Object/Upgrade/WeaponSetUpgrade.cpp
)

target_sources(generals_lib PRIVATE ${GENERALS_SRC})
target_include_directories(generals_lib PRIVATE
    include
    Code/GameEngine/Include
    Code/GameEngine/Include/Precompiled
    Code/Libraries/Include
    Code/Libraries/Source/WWVegas
    Code/Libraries/Source/WWVegas/WWLib
    Code/Libraries/Source/Compression
)
