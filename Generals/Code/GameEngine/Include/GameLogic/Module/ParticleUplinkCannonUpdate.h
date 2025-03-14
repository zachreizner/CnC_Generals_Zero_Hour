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

// FILE: ParticleUplinkCannonUpdate.h //////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, September 2002
// Desc:   Update module to handle building states and weapon firing of the particle uplink cannon.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __PARTICLE_UPLINK_CANNON_UPDATE_H_
#define __PARTICLE_UPLINK_CANNON_UPDATE_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/KindOf.h"
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class SpecialPowerModule;
class ParticleSystem;
class FXList;
class AudioEventRTS;
enum ParticleSystemID : int32_t;

#define MAX_OUTER_NODES 16

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class ParticleUplinkCannonUpdateModuleData : public ModuleData
{
public:
	UnsignedInt			m_beginChargeFrames;
	UnsignedInt			m_raiseAntennaFrames;
	UnsignedInt			m_readyDelayFrames;
	UnsignedInt			m_widthGrowFrames;
	UnsignedInt			m_beamTravelFrames;
	UnsignedInt			m_totalFiringFrames;
	UnsignedInt			m_framesBetweenLaunchFXRefresh;

	AsciiString			m_outerEffectBaseBoneName;
	UnsignedInt			m_outerEffectNumBones;
	AsciiString			m_outerNodesLightFlareParticleSystemName;
	AsciiString			m_outerNodesMediumFlareParticleSystemName;
	AsciiString			m_outerNodesIntenseFlareParticleSystemName;

	AsciiString			m_connectorBoneName;
	AsciiString			m_connectorMediumLaserNameName;
	AsciiString			m_connectorIntenseLaserNameName;
	AsciiString			m_connectorMediumFlareParticleSystemName;
	AsciiString			m_connectorIntenseFlareParticleSystemName;

	AsciiString			m_laserBaseLightFlareParticleSystemName;
	AsciiString			m_laserBaseMediumFlareParticleSystemName;
	AsciiString			m_laserBaseIntenseFlareParticleSystemName;
	
	AsciiString			m_fireBoneName;
	AsciiString			m_particleBeamLaserName;

	FXList					*m_groundHitFX;
	FXList					*m_beamLaunchFX;

	Real						m_swathOfDeathDistance;
	Real						m_swathOfDeathAmplitude;
	UnsignedInt			m_totalScorchMarks;
	Real						m_scorchMarkScalar;

	UnsignedInt			m_totalDamagePulses;
	Real						m_damagePerSecond;
	DamageType			m_damageType;
	DeathType				m_deathType;
	Real						m_damageRadiusScalar;
	Real						m_revealRange;

	SpecialPowerTemplate *m_specialPowerTemplate;


	AsciiString		m_powerupSoundName;
	AsciiString		m_unpackToReadySoundName;
	AsciiString		m_firingToIdleSoundName;
	AsciiString		m_annihilationSoundName;
	AsciiString		m_damagePulseRemnantObjectName;

  Real					m_manualDrivingSpeed;
  Real					m_manualFastDrivingSpeed;
  UnsignedInt		m_doubleClickToFastDriveDelay;

	ParticleUplinkCannonUpdateModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);

private: 

};

enum PUCStatus
{
	STATUS_IDLE,
	STATUS_CHARGING,
	STATUS_PREPARING,
	STATUS_ALMOST_READY,
	STATUS_READY_TO_FIRE,
	STATUS_PREFIRE,
	STATUS_FIRING,
	STATUS_POSTFIRE,
	STATUS_PACKING,
};

enum LaserStatus
{
	LASERSTATUS_NONE,
	LASERSTATUS_BORN,
	LASERSTATUS_DECAYING,
	LASERSTATUS_DEAD,
};

enum IntensityTypes
{
	IT_LIGHT,
	IT_MEDIUM,
	IT_INTENSE,
	IT_FINISH,
};

//-------------------------------------------------------------------------------------------------
/** The default	update module */
//-------------------------------------------------------------------------------------------------
class ParticleUplinkCannonUpdate : public UpdateModule, public SpecialPowerUpdateInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ParticleUplinkCannonUpdate, "ParticleUplinkCannonUpdate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ParticleUplinkCannonUpdate, ParticleUplinkCannonUpdateModuleData );

public:

	ParticleUplinkCannonUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// SpecialPowerUpdateInterface
	virtual void initiateIntentToDoSpecialPower(const SpecialPowerTemplate *specialPowerTemplate, const Object *targetObj, const Coord3D *targetPos, UnsignedInt commandOptions, Int locationCount );
	virtual Bool isSpecialAbility() const { return false; }
	virtual Bool isSpecialPower() const { return true; }
	virtual Bool isActive() const {return m_status != STATUS_IDLE;}
	virtual SpecialPowerUpdateInterface* getSpecialPowerUpdateInterface() { return this; }
	virtual CommandOption getCommandOption() const { return (CommandOption)0; }
	virtual Bool isPowerCurrentlyInUse( const CommandButton *command = NULL ) const;

	virtual void onObjectCreated();
	virtual UpdateSleepTime update();

	void removeAllEffects();

	void createOuterNodeParticleSystems( IntensityTypes intensity );
	void createConnectorLasers( IntensityTypes intensity );
	void createConnectorFlare( IntensityTypes intensity );
	void createLaserBaseFlare( IntensityTypes intensity );
	void createGroundToOrbitLaser( UnsignedInt growthFrames );
	void createOrbitToTargetLaser( UnsignedInt growthFrames );
	void createGroundHitParticleSystem( IntensityTypes intensity );

	Bool calculateDefaultInformation();
	Bool calculateUpBonePositions();

	virtual Bool doesSpecialPowerHaveOverridableDestinationActive() const;
	virtual void setSpecialPowerOverridableDestination( const Coord3D *loc );

	// Disabled conditions to process (termination conditions!)
	virtual DisabledMaskType getDisabledTypesToProcess() const { return MAKE_DISABLED_MASK3( DISABLED_UNDERPOWERED, DISABLED_EMP, DISABLED_HACKED ); }

protected:

	void setLogicalStatus( PUCStatus status );
	void setClientStatus( PUCStatus status, Bool revealThisFrame );
	void killEverything();

	SpecialPowerModuleInterface* m_specialPowerModule;

	AudioEventRTS		m_powerupSound;
	AudioEventRTS		m_unpackToReadySound;
	AudioEventRTS		m_firingToIdleSound;
	AudioEventRTS		m_annihilationSound;

	Matrix3D				m_outerNodeOrientations[ MAX_OUTER_NODES ];

	Coord3D					m_outerNodePositions[ MAX_OUTER_NODES ];
	Coord3D					m_connectorNodePosition;
	Coord3D					m_laserOriginPosition;
	Coord3D					m_initialTargetPosition;
	Coord3D					m_currentTargetPosition;
	Coord3D					m_overrideTargetDestination;

	PUCStatus					m_status;
	LaserStatus				m_laserStatus;
	UnsignedInt				m_frames;
	ParticleSystemID	m_outerSystemIDs[ MAX_OUTER_NODES ];
	DrawableID				m_laserBeamIDs[ MAX_OUTER_NODES ];
	DrawableID				m_groundToOrbitBeamID;
	DrawableID				m_orbitToTargetBeamID;
	ParticleSystemID	m_connectorSystemID;
	ParticleSystemID	m_laserBaseSystemID;

	UnsignedInt			m_scorchMarksMade;
	UnsignedInt			m_nextScorchMarkFrame;
	UnsignedInt			m_nextLaunchFXFrame;
	UnsignedInt			m_damagePulsesMade;
	UnsignedInt			m_nextDamagePulseFrame;
	UnsignedInt			m_startAttackFrame;
	UnsignedInt			m_startDecayFrame;
	UnsignedInt			m_lastDrivingClickFrame;
	UnsignedInt			m_2ndLastDrivingClickFrame;

	Bool						m_upBonesCached;
	Bool						m_defaultInfoCached;
	Bool						m_invalidSettings;
	Bool						m_manualTargetMode;
	Bool						m_clientShroudedLastFrame;
};


#endif

