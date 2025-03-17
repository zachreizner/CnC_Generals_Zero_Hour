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

// ParticleSys.cpp ////////////////////////////////////////////////////////////////////////////////
// Particle System implementation
// Author: Michael S. Booth, November 2001
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_PARTICLE_SYSTEM_NAMES

#include "Common/GameState.h"
#include "Common/INI.h"
#include "Common/PerfTimer.h"
#include "Common/ThingFactory.h"
#include "Common/GameLOD.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"
#include "GameClient/DebugDisplay.h"
#include "GameClient/Display.h"
#include "GameClient/GameClient.h"
#include "GameClient/InGameUI.h"
#include "GameClient/ParticleSys.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/TerrainLogic.h"

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

//------------------------------------------------------------------------------ Performance Timers 
//#include "Common/PerfMetrics.h"
//#include "Common/PerfTimer.h"

//static PerfTimer s_particleSys("ParticleSys::update", false, PERFMETRICS_LOGIC_STARTFRAME, PERFMETRICS_LOGIC_STOPFRAME);
//-------------------------------------------------------------------------------------------------

// the singleton
ParticleSystemManager *TheParticleSystemManager = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ParticleInfo::ParticleInfo( void )
{
	//Added By Sadullah Nader
	//Initializations inserted
	m_angleX = m_angleY = m_angleZ = 0.0f;
	m_angularDamping = 0.0f;
	m_angularRateX = m_angularRateY = m_angularRateZ = 0.0f;
	m_colorScale =0.0f;
  m_size = 0.0f;
	m_sizeRate = 0.0f;
	m_sizeRateDamping = 0.0f;
	m_velDamping = 0.0f;
	m_windRandomness = 0.0f;

	m_emitterPos.zero();
	m_pos.zero();
	m_vel.zero();

	m_lifetime = 0;
	m_particleUpTowardsEmitter = FALSE;
	
	//
}  // end ParticleInfo

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ParticleInfo::crc( Xfer *xfer )
{

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ParticleInfo::xfer( Xfer *xfer )
{
	Int i;

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// velocity
	xfer->xferCoord3D( &m_vel );

	// position
	xfer->xferCoord3D( &m_pos );

	// emitter position
	xfer->xferCoord3D( &m_emitterPos );

	// velocity damping
	xfer->xferReal( &m_velDamping );

	// angle
	xfer->xferReal( &m_angleX );
	xfer->xferReal( &m_angleY );
	xfer->xferReal( &m_angleZ );

	// angular rate
	xfer->xferReal( &m_angularRateX );
	xfer->xferReal( &m_angularRateY );
	xfer->xferReal( &m_angularRateZ );

	// lifetime
	xfer->xferUnsignedInt( &m_lifetime );

	// size
	xfer->xferReal( &m_size );

	// size rate
	xfer->xferReal( &m_sizeRate );

	// size rate damping
	xfer->xferReal( &m_sizeRateDamping );

	// alpha keys
	for( i = 0; i < MAX_KEYFRAMES; ++i )
	{

		xfer->xferReal( &m_alphaKey[ i ].value );
		xfer->xferUnsignedInt( &m_alphaKey[ i ].frame );

	}  // end for, i

	// color keys
	for( i = 0; i < MAX_KEYFRAMES; ++i )
	{

		xfer->xferRGBColor( &m_colorKey[ i ].color );
		xfer->xferUnsignedInt( &m_colorKey[ i ].frame );

	}  // end for, i

	// color scale
	xfer->xferReal( &m_colorScale );

	// particle up towards emitter
	xfer->xferBool( &m_particleUpTowardsEmitter );

	// wind randomness
	xfer->xferReal( &m_windRandomness );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ParticleInfo::loadPostProcess( void )
{

}  // end loadPostProcess

/** Load post process */
// ------------------------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
enum
{
	MAX_SIZE_BONUS = 50
};


//todo move this somewhere more useful.
// ------------------------------------------------------------------------------------------------
#if 0
static Real angleBetween(const Coord2D &vecA, const Coord2D &vecB)
{
	Real lengthA = vecA->length();
	if (lengthA < FLT_EPSILON) {
		return 0.0f;
	}

	Real lengthB = vecB->length();
	if (lengthB < FLT_EPSILON) {
		return 0.0f;
	}

	Real dotProduct = (vecA->x * vecB->x + vecA->y * vecB->y);

	// If the dotproduct is 0.0, then they are orthogonal
	if (dotProduct < FLT_EPSILON && dotProduct > -FLT_EPSILON) {
		return vecB->x > 0.0f ? PI : 0.0f;
	}

	Real cosTheta = dotProduct / (lengthA * lengthB);
	Real theta = ACos(cosTheta);
	return vecB->x > 0.0f ? theta : -theta;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// Particle ///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
/** Compute alpha rate to get to next key on given frame */
// ------------------------------------------------------------------------------------------------
void Particle::computeAlphaRate( void )
{
	if (m_alphaKey[ m_alphaTargetKey ].frame == 0)
	{
		m_alphaRate = 0.0f;
		return;
	}

	Real delta = m_alphaKey[ m_alphaTargetKey ].value - m_alphaKey[ m_alphaTargetKey-1 ].value;
	UnsignedInt time = m_alphaKey[ m_alphaTargetKey ].frame - m_alphaKey[ m_alphaTargetKey-1 ].frame;

	m_alphaRate = delta/time;
}

// ------------------------------------------------------------------------------------------------
/** Compute color rate to get to next key on given frame */
// ------------------------------------------------------------------------------------------------
void Particle::computeColorRate( void )
{
	if (m_colorKey[ m_colorTargetKey ].frame == 0)
	{
		m_colorRate.red = 0.0f;
		m_colorRate.green = 0.0f;
		m_colorRate.blue = 0.0f;
		return;
	}

	UnsignedInt time = m_colorKey[ m_colorTargetKey ].frame - m_colorKey[ m_colorTargetKey-1 ].frame;
	Real delta = m_colorKey[ m_colorTargetKey ].color.red - m_colorKey[ m_colorTargetKey-1 ].color.red;
	m_colorRate.red = delta/time;

	delta = m_colorKey[ m_colorTargetKey ].color.green - m_colorKey[ m_colorTargetKey-1 ].color.green;
	m_colorRate.green = delta/time;

	delta = m_colorKey[ m_colorTargetKey ].color.blue - m_colorKey[ m_colorTargetKey-1 ].color.blue;
	m_colorRate.blue = delta/time;
}

// ------------------------------------------------------------------------------------------------
/** Construct a particle from a particle template */
// ------------------------------------------------------------------------------------------------
Particle::Particle( ParticleSystem *system, const ParticleInfo *info )
{
	m_system = system;

	m_isCulled = FALSE;
	m_accel.x = 0.0f;
	m_accel.y = 0.0f;
	m_accel.z = 0.0f;

	m_vel = info->m_vel;
	m_pos = info->m_pos;

	m_angleX = info->m_angleX;
	m_angleY = info->m_angleY;
	m_angleZ = info->m_angleZ;

	//Added By Sadullah Nader
	//Initializations inserted
	m_lastPos.zero();
	//
	m_windRandomness = info->m_windRandomness;	
	m_particleUpTowardsEmitter = info->m_particleUpTowardsEmitter;
	m_emitterPos = info->m_emitterPos;

	
	m_angularRateX = info->m_angularRateX;
	m_angularRateY = info->m_angularRateY;
	m_angularRateZ = info->m_angularRateZ;
	m_angularDamping = info->m_angularDamping;

	m_velDamping = info->m_velDamping;

	m_lifetime = info->m_lifetime;
	m_lifetimeLeft = info->m_lifetime;
	m_createTimestamp = TheGameClient->getFrame();
	m_personality = 0;

	m_size = info->m_size;
	m_sizeRate = info->m_sizeRate;
	m_sizeRateDamping = info->m_sizeRateDamping;

	// set up alpha
	for( int i=0; i<MAX_KEYFRAMES; i++ )
		m_alphaKey[i] = info->m_alphaKey[i];

	m_alpha = m_alphaKey[0].value;
	m_alphaTargetKey = 1;
	computeAlphaRate();

	// set up colors
	for( int i=0; i<MAX_KEYFRAMES; i++ )
		m_colorKey[i] = info->m_colorKey[i];

	m_color = m_colorKey[0].color;
	m_colorTargetKey = 1;
	computeColorRate();

	m_colorScale = info->m_colorScale;

	if (system->isUsingDrawables())
	{
		const ThingTemplate *tmpl = TheThingFactory->findTemplate(system->getParticleTypeName());
		DEBUG_ASSERTCRASH(tmpl, ("Drawable %s not found\n",system->getParticleTypeName().str()));
		if (tmpl)
		{
			m_drawable = TheThingFactory->newDrawable(tmpl);
			if (m_drawable)
				m_drawable->attachToParticleSystem( this );
		}
	}

	m_inSystemList = m_inOverallList = FALSE;
	m_systemPrev = m_systemNext = m_overallPrev = m_overallNext = NULL;

	// add this particle to the global list, retaining particle creation order
	TheParticleSystemManager->addParticle(this, system->getPriority() );

	// add this particle to the Particle System list, retaining local creation order
	m_system->addParticle(this);

	//DEBUG_ASSERTLOG(!(totalParticleCount % 100 == 0), ( "TotalParticleCount = %d\n", m_totalParticleCount ));
}

// ------------------------------------------------------------------------------------------------
/** Destructor */
// ------------------------------------------------------------------------------------------------
Particle::~Particle()
{
	// tell the particle system that this particle is gone
	m_system->removeParticle( this );

	if (m_drawable)
		TheGameClient->destroyDrawable( m_drawable );
	m_drawable = NULL;

	// if this particle was controlling another particle system, destroy that system
	if (m_systemUnderControl)
	{
		m_systemUnderControl->detachControlParticle( this );
		m_systemUnderControl->destroy();
	}
	m_systemUnderControl = NULL;

	// remove from the global list
	TheParticleSystemManager->removeParticle(this);

	//DEBUG_ASSERTLOG(!(totalParticleCount % 100 == 0), ( "TotalParticleCount = %d\n", m_totalParticleCount ));
}

// ------------------------------------------------------------------------------------------------
/** Add the given acceleration */
// ------------------------------------------------------------------------------------------------
void Particle::applyForce( const Coord3D *force )
{
	m_accel.x += force->x;
	m_accel.y += force->y;
	m_accel.z += force->z;
}

// ------------------------------------------------------------------------------------------------
/** Update the behavior of an individual particle */
// ------------------------------------------------------------------------------------------------
Bool Particle::update( void )
{
	// integrate acceleration into velocity
	m_vel.x += m_accel.x;
	m_vel.y += m_accel.y;
	m_vel.z += m_accel.z;

	m_vel.x *= m_velDamping;
	m_vel.y *= m_velDamping;
	m_vel.z *= m_velDamping;

	// integrate velocity into position
	const Coord3D *driftVel = m_system->getDriftVelocity();
	m_pos.x += m_vel.x + driftVel->x;
	m_pos.y += m_vel.y + driftVel->y;
	m_pos.z += m_vel.z + driftVel->z;

	// integrate the wind (if specified) into position
	doWindMotion();

	// update orientation
	m_angleX += m_angularRateX;
	m_angleY += m_angularRateY;
	m_angleZ += m_angularRateZ;
	m_angularRateX *= m_angularDamping;
	m_angularRateY *= m_angularDamping;
	m_angularRateZ *= m_angularDamping;

	if (m_particleUpTowardsEmitter) {
		// adjust the up position back towards the particle
		Coord2D emitterDir;
		emitterDir.x = m_pos.x - m_emitterPos.x;
		emitterDir.y = m_pos.y - m_emitterPos.y;
#if 0
		static const Coord2D upVec = { 0.0f, 1.0f };
		m_angleZ = angleBetween(upVec, emitterDir) + PI;
#else
		if (emitterDir.y < FLT_EPSILON && emitterDir.y > -FLT_EPSILON) {
			m_angleZ = emitterDir.x > 0.0f ? PI + PI : PI;
		} else {
			Real emitterDirLength = emitterDir.length();
			if (emitterDirLength < FLT_EPSILON) {
				m_angleZ = PI;
			} else {
				Real theta = ACos(emitterDir.y/emitterDirLength);
				m_angleZ = emitterDir.x > 0.0f ? PI + theta : PI - theta;
			}
		}
#endif
	}

	// update size
	m_size += m_sizeRate;
	m_sizeRate *= m_sizeRateDamping;

	//
	// Update alpha
	//
	m_alpha += m_alphaRate;

	if (m_alphaTargetKey < MAX_KEYFRAMES && m_alphaKey[ m_alphaTargetKey ].frame)
	{
		if (TheGameClient->getFrame() - m_createTimestamp >= m_alphaKey[ m_alphaTargetKey ].frame)
		{
			m_alpha = m_alphaKey[ m_alphaTargetKey ].value;
			m_alphaTargetKey++;
			computeAlphaRate();
		}
	}
	else
		m_alphaRate = 0.0f;

	if (m_alpha < 0.0f)
		m_alpha = 0.0f;
	else if (m_alpha > 1.0f)
		m_alpha = 1.0f;


	//
	// Update color
	//
	m_color.red += m_colorRate.red;
	m_color.green += m_colorRate.green;
	m_color.blue += m_colorRate.blue;

	if (m_colorTargetKey < MAX_KEYFRAMES && m_colorKey[ m_colorTargetKey ].frame)
	{
		if (TheGameClient->getFrame() - m_createTimestamp >= m_colorKey[ m_colorTargetKey ].frame)
		{
			// can't set, because of colorscale
			// m_color = m_colorKey[ m_colorTargetKey ].color;
			m_colorTargetKey++;
			computeColorRate();
		}
	}
	else
	{
		m_colorRate.red = 0.0f;
		m_colorRate.green = 0.0f;
		m_colorRate.blue = 0.0f;
	}

	/// @todo Rethink this - at least its name
	m_color.red += m_colorScale;
	m_color.green += m_colorScale;
	m_color.blue += m_colorScale;

	if (m_color.red < 0.0f)
		m_color.red = 0.0f;
	else if (m_color.red > 1.0f)
		m_color.red = 1.0f;

	if (m_color.red < 0.0f)
		m_color.green = 0.0f;
	else if (m_color.green > 1.0f)
		m_color.green = 1.0f;

	if (m_color.blue < 0.0f)
		m_color.blue = 0.0f;
	else if (m_color.blue > 1.0f)
		m_color.blue = 1.0f;


	// reset the acceleration for accumulation next frame
	m_accel.x = 0.0f;
	m_accel.y = 0.0f;
	m_accel.z = 0.0f;

	if (m_drawable)
	{
		Matrix3D rot;
		rot.Make_Identity();
		rot.Rotate_X( m_angleX );
		rot.Rotate_Y( m_angleY );
		rot.Rotate_Z( m_angleZ );
		m_drawable->setInstanceMatrix( &rot );
		m_drawable->setPosition( &m_pos );
	}

	// monitor lifetime
	if (m_lifetimeLeft && --m_lifetimeLeft == 0)
		return false;

	DEBUG_ASSERTCRASH( m_lifetimeLeft, ( "A particle has an infinite lifetime..." ));

	// if we've gone totally invisible, destroy ourselves
	if (isInvisible())
		return false;
	return true;
}

// ------------------------------------------------------------------------------------------------
/** Do wind motion as specified by the particle system template, if present */
// ------------------------------------------------------------------------------------------------
void Particle::doWindMotion( void )
{
	ParticleSystemInfo::WindMotion windMotion = m_system->getWindMotion();

	// see if we should even do anything
	if( windMotion == ParticleSystemInfo::WIND_MOTION_NOT_USED )
		return;

	// get the angle of the wind
	Real windAngle = m_system->getWindAngle();

	// get the system position
	Coord3D systemPos;
	m_system->getPosition( &systemPos );

	// when we're attached objects and drawables we offset by that position as well
	if( ObjectID attachedObj = m_system->getAttachedObject() )
	{
		Object *obj = TheGameLogic->findObjectByID( attachedObj );

		if( obj )
		{
			const Coord3D *objPos = obj->getPosition();

			systemPos.x += objPos->x;
			systemPos.y += objPos->y;
			systemPos.z += objPos->z;

		}  // end if

	}  // end if
	else if( DrawableID attachedDraw = m_system->getAttachedDrawable() )
	{
		Drawable *draw = TheGameClient->findDrawableByID( attachedDraw );

		if( draw )
		{
			const Coord3D *drawPos = draw->getPosition();

			systemPos.x += drawPos->x;
			systemPos.y += drawPos->y;
			systemPos.z += drawPos->z;

		}  // end if

	}  // end else if

	//
	// compute a vector from the system position in the world to the particle ... we will use
	// this to compute how much force we apply
	//
	Coord3D v;
	v.x = m_pos.x - systemPos.x;
	v.y = m_pos.y - systemPos.y;
	v.z = m_pos.z - systemPos.z;

	// distance amounts for full force from wind and no force at all
	Real fullForceDistance = 75.0f;
	Real noForceDistance = 200.0f;

	//
	// given the distance from the wind position to the particle ... figure out how much
	// force we're going to apply to it.  When it's further away (outside of the full force
	// distance) we will apply only a fraction of the force
	//

	Real distFromWind = v.length();
	if( distFromWind < noForceDistance )
	{
		Real windForceStrength = 2.0f * m_windRandomness;

		// only apply force if still within the circle of influence
		if( distFromWind > fullForceDistance )
			windForceStrength *= (1.0f - ((distFromWind - fullForceDistance) / 
																		(noForceDistance - fullForceDistance)));

		// integate the wind motion into the position
		m_pos.x += (Cos( windAngle ) * windForceStrength);
		m_pos.y += (Sin( windAngle ) * windForceStrength);

	}  // end if

}  // doWindMotion

// ------------------------------------------------------------------------------------------------
/** Get priority of a particle ... which is the priority of it's attached system */
// ------------------------------------------------------------------------------------------------
ParticlePriorityType Particle::getPriority( void )
{ 
	return m_system->getPriority();
}

// ------------------------------------------------------------------------------------------------
/** Return true if this particle is invisible */
// ------------------------------------------------------------------------------------------------
Bool Particle::isInvisible( void )
{
	// Drawables are never invisible (yet)
	/// @todo Allow Drawables to fade via alpha (MSB)
	if (m_drawable)
		return false;

	switch (m_system->getShaderType())
	{
		case ParticleSystemInfo::ADDITIVE:
			// if color is black, this particle is invisible
			
			// check that we're not in the process of going to another color
			if (m_colorKey[ m_colorTargetKey ].frame == 0)
			{
				if (m_color.red < 0.01f && m_color.green < 0.01f && m_color.blue < 0.01f)
					return true;
			}
			return false;

		case ParticleSystemInfo::ALPHA:
			// if alpha is zero, this particle is invisible
			if (m_alpha < 0.01f)
				return true;
			return false;

		case ParticleSystemInfo::ALPHA_TEST:
			// hmm... assume these particles are never invisible
			return false;

		case ParticleSystemInfo::MULTIPLY:
			// if color is white, this particle is invisible

			// check that we're not in the process of going to another color
			if (m_colorKey[ m_colorTargetKey ].frame == 0)
			{
				if (m_color.red > 0.99f && m_color.green > 0.99f && m_color.blue > 0.99f)
					return true;
			}
			return false;
	}

	// should never get here - if we do, data is incorrect
	return true;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void Particle::crc( Xfer *xfer )
{

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void Particle::xfer( Xfer *xfer )
{

	// version 
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// base class particle info
	ParticleInfo::xfer( xfer );

	// personality
	xfer->xferUnsignedInt( &m_personality );

	// acceleration
	xfer->xferCoord3D( &m_accel );

	// last position
	xfer->xferCoord3D( &m_lastPos );

	// lifetime left
	xfer->xferUnsignedInt( &m_lifetimeLeft );

	// creation timestamp
	xfer->xferUnsignedInt( &m_createTimestamp );

	// alpha
	xfer->xferReal( &m_alpha );

	// alpha rate
	xfer->xferReal( &m_alphaRate );

	// alpha target key
	xfer->xferInt( &m_alphaTargetKey );

	// color
	xfer->xferRGBColor( &m_color );

	// color rate
	xfer->xferRGBColor( &m_colorRate );

	// color target key
	xfer->xferInt( &m_colorTargetKey );

	// drawable
	DrawableID drawableID = m_drawable ? m_drawable->getID() : INVALID_DRAWABLE_ID;
	xfer->xferDrawableID( &drawableID );
	if( xfer->getXferMode() == XFER_LOAD && drawableID != INVALID_DRAWABLE_ID )
	{

		//
		// save drawable pointer, note that this xfer block is after all the drawable stuff
		// so we don't need to post process anything all the correct drawables are loaded
		// and available to us
		//
		m_drawable = TheGameClient->findDrawableByID( drawableID );

		// sanity
		if( m_drawable == NULL )
		{

			DEBUG_CRASH(( "Particle::xfer - Unable to find matching drawable id for particle\n" ));
			throw SC_INVALID_DATA;

		}  // end if

		// set the particle pointer in the drawable
		m_drawable->friend_setParticle( this );

	}  // end if

	// system under control as an id
	ParticleSystemID systemUnderControlID = m_systemUnderControl ? m_systemUnderControl->getSystemID() : INVALID_PARTICLE_SYSTEM_ID;
	xfer->xferUser( &systemUnderControlID, sizeof( ParticleSystemID ) );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void Particle::loadPostProcess( void )
{

	// call base class post process
	ParticleInfo::loadPostProcess();

	// tidy up the m_systemUnderControl pointer
	if( m_systemUnderControlID != INVALID_PARTICLE_SYSTEM_ID )
	{
		ParticleSystem *system;
		
		// find system
		system = TheParticleSystemManager->findParticleSystem( m_systemUnderControlID );

		// set us as the control particle for this system
		system->setControlParticle( this );
		controlParticleSystem( system );

		// sanity
		if( m_systemUnderControlID == INVALID_PARTICLE_SYSTEM_ID )
		{

			DEBUG_CRASH(( "Particle::loadPostProcess - Unable to find system under control pointer\n" ));
			throw SC_INVALID_DATA;

		}  // end if

	}  // end if

}  // end loadPostProcess

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ParticleSystemInfo::ParticleSystemInfo()
{
	m_priority = PARTICLE_PRIORITY_LOWEST;
	m_isGroundAligned = false;
	m_isEmitAboveGroundOnly = false;
	m_isParticleUpTowardsEmitter = false;
	
	//Added By Sadullah Nader
	//Initializations inserted
	m_driftVelocity.zero();
	m_gravity = 0.0f;
	m_isEmissionVolumeHollow = FALSE;
	m_isOneShot = FALSE;
	m_slavePosOffset.zero();
	m_systemLifetime = 0;
	
	//
	// some default values for the wind motion values
	m_windMotion = WIND_MOTION_NOT_USED;
	m_windAngle = 0.0f;
	m_windAngleChange = 0.15f;  // higher is ping pong faster
	m_windAngleChangeMin = 0.15f;
	m_windAngleChangeMax = 0.45f;
	m_windMotionStartAngleMin = 0.0f;
	m_windMotionStartAngleMax = PI / 4.0f;
	m_windMotionStartAngle = m_windMotionStartAngleMin;
	m_windMotionEndAngleMin = TWO_PI - (PI / 4.0f);
	m_windMotionEndAngleMax = TWO_PI;
	m_windMotionEndAngle = m_windMotionEndAngleMin;
	m_windMotionMovingToEndAngle = TRUE;
	m_volumeParticleDepth = DEFAULT_VOLUME_PARTICLE_DEPTH;

}


void ParticleSystemInfo::tintAllColors( Color tintColor )
{
	RGBColor rgb;
	rgb.setFromInt(tintColor);

	//This tints all but the first colorKey!!!
	for (int key = 1; key < MAX_KEYFRAMES; ++key )
	{
		m_colorKey[ key ].color.red   *= (Real)(rgb.red  ) / 255.0f;
		m_colorKey[ key ].color.green *= (Real)(rgb.green) / 255.0f;
		m_colorKey[ key ].color.blue  *= (Real)(rgb.blue ) / 255.0f;
	}

}  // end loadPostProcess


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ParticleSystemInfo::crc( Xfer *xfer )
{

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ParticleSystemInfo::xfer( Xfer *xfer )
{
	Int i;

	// version 
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// is one shot
	xfer->xferBool( &m_isOneShot );

	// shader type
	xfer->xferUser( &m_shaderType, sizeof( ParticleShaderType ) );

	// particle type
	xfer->xferUser( &m_particleType, sizeof( ParticleType ) );

	// particle type name
	xfer->xferAsciiString( &m_particleTypeName );

	// angles
	xfer->xferUser( &m_angleX, sizeof( GameClientRandomVariable ) );
	xfer->xferUser( &m_angleY, sizeof( GameClientRandomVariable ) );
	xfer->xferUser( &m_angleZ, sizeof( GameClientRandomVariable ) );

	// angular rate
	xfer->xferUser( &m_angularRateX, sizeof( GameClientRandomVariable ) );
	xfer->xferUser( &m_angularRateY, sizeof( GameClientRandomVariable ) );
	xfer->xferUser( &m_angularRateZ, sizeof( GameClientRandomVariable ) );

	// angular damping
	xfer->xferUser( &m_angularDamping, sizeof( GameClientRandomVariable ) );

	// velocity damping
	xfer->xferUser( &m_velDamping, sizeof( GameClientRandomVariable ) );

	// lifetime
	xfer->xferUser( &m_lifetime, sizeof( GameClientRandomVariable ) );

	// system lifetime
	xfer->xferUnsignedInt( &m_systemLifetime );

	// start size
	xfer->xferUser( &m_startSize, sizeof( GameClientRandomVariable ) );

	// start size rate
	xfer->xferUser( &m_startSizeRate, sizeof( GameClientRandomVariable ) );

	// size rate
	xfer->xferUser( &m_sizeRate, sizeof( GameClientRandomVariable ) );

	// size rate damping
	xfer->xferUser( &m_sizeRateDamping, sizeof( GameClientRandomVariable ) );

	// alpha keys
	for( i = 0; i < MAX_KEYFRAMES; ++i )
	{

		xfer->xferUser( &m_alphaKey[ i ].var, sizeof( GameClientRandomVariable ) );
		xfer->xferUnsignedInt( &m_alphaKey[ i ].frame );

	}  // end for, i

	// color keys
	for( i = 0; i < MAX_KEYFRAMES; ++i )
	{

		xfer->xferRGBColor( &m_colorKey[ i ].color );
		xfer->xferUnsignedInt( &m_colorKey[ i ].frame );

	}  // end for, i

	// color scale
	xfer->xferUser( &m_colorScale, sizeof( GameClientRandomVariable ) );
	
	// burst delay
	xfer->xferUser( &m_burstDelay, sizeof( GameClientRandomVariable ) );
	
	// burst count
	xfer->xferUser( &m_burstCount, sizeof( GameClientRandomVariable ) );
	
	// initial delay
	xfer->xferUser( &m_initialDelay, sizeof( GameClientRandomVariable ) );
	
	// drift velocity
	xfer->xferCoord3D( &m_driftVelocity );
	
	// gravity
	xfer->xferReal( &m_gravity );
		
	// slave system name
	xfer->xferAsciiString( &m_slaveSystemName );

	// slave position offset
	xfer->xferCoord3D( &m_slavePosOffset );

	// attached system name
	xfer->xferAsciiString( &m_attachedSystemName );

	// emission velocity type, this must come before m_emissionVelocity
	xfer->xferUser( &m_emissionVelocityType, sizeof( EmissionVelocityType ) );

	// particle priority
	xfer->xferUser( &m_priority, sizeof( ParticlePriorityType ) );

	// emission velocity
	switch( m_emissionVelocityType )
	{

		// --------------------------------------------------------------------------------------------
		case ORTHO:
			xfer->xferUser( &m_emissionVelocity.ortho.x, sizeof( GameClientRandomVariable ) );
			xfer->xferUser( &m_emissionVelocity.ortho.y, sizeof( GameClientRandomVariable ) );
			xfer->xferUser( &m_emissionVelocity.ortho.z, sizeof( GameClientRandomVariable ) );
			break;

		// --------------------------------------------------------------------------------------------
		case SPHERICAL:		
			xfer->xferUser( &m_emissionVelocity.spherical.speed, sizeof( GameClientRandomVariable ) );			
			break;

		// --------------------------------------------------------------------------------------------
		case HEMISPHERICAL:
			xfer->xferUser( &m_emissionVelocity.hemispherical.speed, sizeof( GameClientRandomVariable ) );			
			break;

		// --------------------------------------------------------------------------------------------
		case CYLINDRICAL:
			xfer->xferUser( &m_emissionVelocity.cylindrical.radial, sizeof( GameClientRandomVariable ) );						
			xfer->xferUser( &m_emissionVelocity.cylindrical.normal, sizeof( GameClientRandomVariable ) );
			break;

		// --------------------------------------------------------------------------------------------
		case OUTWARD:
			xfer->xferUser( &m_emissionVelocity.outward.speed, sizeof( GameClientRandomVariable ) );
			xfer->xferUser( &m_emissionVelocity.outward.otherSpeed, sizeof( GameClientRandomVariable ) );
			break;

	}  // end switch, m_emissionVelocityType

	// emission volume type
	xfer->xferUser( &m_emissionVolumeType, sizeof( EmissionVolumeType ) );

	// emission volume
	switch( m_emissionVolumeType )
	{

		// --------------------------------------------------------------------------------------------
		case POINT:
			// point has no data, it uses the systems position			
			break;

		// --------------------------------------------------------------------------------------------
		case LINE:
			xfer->xferCoord3D( &m_emissionVolume.line.start );
			xfer->xferCoord3D( &m_emissionVolume.line.end );
			break;

		// --------------------------------------------------------------------------------------------
		case BOX:
			xfer->xferCoord3D( &m_emissionVolume.box.halfSize );
			break;

		// --------------------------------------------------------------------------------------------
		case SPHERE:
			xfer->xferReal( &m_emissionVolume.sphere.radius );
			break;

		// --------------------------------------------------------------------------------------------
		case CYLINDER:
			xfer->xferReal( &m_emissionVolume.cylinder.radius );
			xfer->xferReal( &m_emissionVolume.cylinder.length );
			break;

	}  // end switch, m_emissionVolumeType

	// is emission volume hollow
	xfer->xferBool( &m_isEmissionVolumeHollow );

	// is ground aligned
	xfer->xferBool( &m_isGroundAligned );

	// emit above ground only
	xfer->xferBool( &m_isEmitAboveGroundOnly );

	// is particle up towards emitter
	xfer->xferBool( &m_isParticleUpTowardsEmitter );

	// wind motion
	xfer->xferUser( &m_windMotion, sizeof( WindMotion ) );

	// wind angle
	xfer->xferReal( &m_windAngle );

	// wind angle change
	xfer->xferReal( &m_windAngleChange );

	// wind angle change min
	xfer->xferReal( &m_windAngleChangeMin );

	// wind angle change max
	xfer->xferReal( &m_windAngleChangeMax );

	// wind motion start angle
	xfer->xferReal( &m_windMotionStartAngle );

	// wind motion start angle min
	xfer->xferReal( &m_windMotionStartAngleMin );

	// wind motion start angle max
	xfer->xferReal( &m_windMotionStartAngleMax );

	// wind motion end angle
	xfer->xferReal( &m_windMotionEndAngle );

	// wind motion end angle min
	xfer->xferReal( &m_windMotionEndAngleMin );

	// wind motion end angle max
	xfer->xferReal( &m_windMotionEndAngleMax );

	// wind motion moving to end angle
	xfer->xferByte( &m_windMotionMovingToEndAngle );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ParticleSystemInfo::loadPostProcess( void )
{

}  // end loadPostProcess

///////////////////////////////////////////////////////////////////////////////////////////////////
// ParticleSystem /////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
/** Read particle system properties from given file */
// ------------------------------------------------------------------------------------------------
ParticleSystem::ParticleSystem( const ParticleSystemTemplate *sysTemplate, 
																ParticleSystemID id, 
																Bool createSlaves )
{
	m_systemParticlesHead = m_systemParticlesTail = NULL;

	m_isFirstPos = true;
	m_template = sysTemplate;
	m_systemID = id;

	//Added By Sadullah Nader
	//Initializations inserted
	m_lastPos.zero();
	m_pos.zero();
	m_velCoeff.zero();
	//

	m_attachedToDrawableID = INVALID_DRAWABLE_ID;
	m_attachedToObjectID = INVALID_ID;

	m_isLocalIdentity = true;
	m_localTransform.Make_Identity();

	m_isIdentity = true;
	m_transform.Make_Identity();

	m_isStopped = false;
	m_isDestroyed = false;
	m_isSaveable = true;

	m_slavePosOffset = sysTemplate->m_slavePosOffset;


	///@todo: further formalize this parameter with an UnsignedInt field in the editor
	m_volumeParticleDepth = DEFAULT_VOLUME_PARTICLE_DEPTH;


	m_driftVelocity = sysTemplate->m_driftVelocity;

	m_velCoeff.x = 1.0f;
	m_velCoeff.y = 1.0f;
	m_velCoeff.z = 1.0f;
	m_countCoeff = 1.0f;
	m_delayCoeff = 1.0f;
	m_sizeCoeff = 1.0f;

	m_gravity = sysTemplate->m_gravity;

	m_lifetime = sysTemplate->m_lifetime;
	m_startSize = sysTemplate->m_startSize;
	m_startSizeRate = sysTemplate->m_startSizeRate;
	m_sizeRate = sysTemplate->m_sizeRate;
	m_sizeRateDamping = sysTemplate->m_sizeRateDamping;

	for( int i=0; i<MAX_KEYFRAMES; i++ )
		m_alphaKey[i] = sysTemplate->m_alphaKey[i];

	for( int i=0; i<MAX_KEYFRAMES; i++ )
		m_colorKey[i] = sysTemplate->m_colorKey[i];

	/// @todo It is confusing to do this conversion here...
	Real low = sysTemplate->m_colorScale.getMinimumValue();
	Real hi = sysTemplate->m_colorScale.getMaximumValue();
	m_colorScale.setRange( low / 255.0f, hi / 255.0f );

	m_burstDelay = sysTemplate->m_burstDelay;
	m_burstDelayLeft = 0;

	m_burstCount = sysTemplate->m_burstCount;

	m_isOneShot = sysTemplate->m_isOneShot;

	m_delayLeft = (UnsignedInt)sysTemplate->m_initialDelay.getValue();

	m_startTimestamp = TheGameClient->getFrame();
	m_systemLifetimeLeft = sysTemplate->m_systemLifetime;
	if (sysTemplate->m_systemLifetime)
		m_isForever = false;
	else
		m_isForever = true;

	m_accumulatedSizeBonus = 0;

	m_velDamping = sysTemplate->m_velDamping;

	m_angleX = sysTemplate->m_angleX;
	m_angleY = sysTemplate->m_angleY;
	m_angleZ = sysTemplate->m_angleZ;
	m_angularRateX = sysTemplate->m_angularRateX;
	m_angularRateY = sysTemplate->m_angularRateY;
	m_angularRateZ = sysTemplate->m_angularRateZ;
	m_angularDamping = sysTemplate->m_angularDamping;

	m_priority = sysTemplate->m_priority;

	m_emissionVelocityType = sysTemplate->m_emissionVelocityType;
	m_emissionVelocity = sysTemplate->m_emissionVelocity;

	m_emissionVolumeType = sysTemplate->m_emissionVolumeType;
	m_emissionVolume = sysTemplate->m_emissionVolume;

	m_isEmissionVolumeHollow = sysTemplate->m_isEmissionVolumeHollow;
	m_isGroundAligned = sysTemplate->m_isGroundAligned;
	m_isEmitAboveGroundOnly = sysTemplate->m_isEmitAboveGroundOnly;
	m_isParticleUpTowardsEmitter = sysTemplate->m_isParticleUpTowardsEmitter;

	m_windMotion = sysTemplate->m_windMotion;
	m_windAngleChange = sysTemplate->m_windAngleChange;
	m_windAngleChangeMin = sysTemplate->m_windAngleChangeMin;
	m_windAngleChangeMax = sysTemplate->m_windAngleChangeMax;
	m_windMotionStartAngleMin = sysTemplate->m_windMotionStartAngleMin;
	m_windMotionStartAngleMax = sysTemplate->m_windMotionStartAngleMax;
	m_windMotionEndAngleMin = sysTemplate->m_windMotionEndAngleMin;
	m_windMotionEndAngleMax = sysTemplate->m_windMotionEndAngleMax;
	m_windMotionMovingToEndAngle = sysTemplate->m_windMotionMovingToEndAngle;
	m_windMotionStartAngle = GameClientRandomValueReal( m_windMotionStartAngleMin, m_windMotionStartAngleMax );
	m_windMotionEndAngle = GameClientRandomValueReal( m_windMotionEndAngleMin, m_windMotionEndAngleMax );
	m_windAngle = GameClientRandomValueReal( m_windMotionStartAngle, m_windMotionEndAngle );
			
	m_shaderType = sysTemplate->m_shaderType;

	m_particleType = sysTemplate->m_particleType;
	m_particleTypeName = sysTemplate->m_particleTypeName;

	m_isStopped = false;

	// set up slave particle system, if any
	m_masterSystemID = INVALID_PARTICLE_SYSTEM_ID;
	m_slaveSystemID = INVALID_PARTICLE_SYSTEM_ID;
	m_masterSystem = NULL;
	m_slaveSystem = NULL;
	if( createSlaves )
	{
		ParticleSystem *slaveSystem = sysTemplate->createSlaveSystem();

		if( slaveSystem )
		{

			setSlave( slaveSystem );
			m_slaveSystem->setMaster( this );

		}  // end if

	}  // end if

	m_attachedSystemName = sysTemplate->m_attachedSystemName;
	m_particleCount = 0;
	m_personalityStore = 0;
	m_controlParticle = NULL;

	TheParticleSystemManager->friend_addParticleSystem(this);

	//DEBUG_ASSERTLOG(!(m_totalParticleSystemCount % 10 == 0), ( "TotalParticleSystemCount = %d\n", m_totalParticleSystemCount ));
}

// ------------------------------------------------------------------------------------------------
/** Destroy particle system and all of its particles */
// ------------------------------------------------------------------------------------------------
ParticleSystem::~ParticleSystem()
{

	// tell any of our slave systems that we are going away
	if( m_slaveSystem )
	{

		DEBUG_ASSERTCRASH( m_slaveSystem->getMaster() == this, ("~ParticleSystem: Our slave doesn't have us as a master!\n") );
		m_slaveSystem->setMaster( NULL );
		setSlave( NULL );

	}  // end if

	// tell any master system that *we* are going away
	if( m_masterSystem )
	{

		DEBUG_ASSERTCRASH( m_masterSystem->getSlave() == this, ("~ParticleSystem: Our master doesn't have us as a slave!\n") );
		m_masterSystem->setSlave( NULL );
		setMaster( NULL );

	}  // end if
	

	// destroy all particles "in the air"
	while (m_systemParticlesHead)
		m_systemParticlesHead->deleteInstance();

	m_attachedToDrawableID = INVALID_DRAWABLE_ID;
	m_attachedToObjectID = INVALID_ID;

	// if this system was controlled by a particle, detach
	if (m_controlParticle)
		m_controlParticle->detachControlledParticleSystem();

	m_controlParticle = NULL;
	
	TheParticleSystemManager->friend_removeParticleSystem(this);
	//DEBUG_ASSERTLOG(!(m_totalParticleSystemCount % 10 == 0), ( "TotalParticleSystemCount = %d\n", m_totalParticleSystemCount ));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ParticleSystem::setMaster( ParticleSystem *master )
{

	m_masterSystem = master;
	m_masterSystemID = master ? master->getSystemID() : INVALID_PARTICLE_SYSTEM_ID;

}  // end set Master

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ParticleSystem::setSlave( ParticleSystem *slave )
{

	m_slaveSystem = slave;
	m_slaveSystemID = slave ? slave->getSystemID() : INVALID_PARTICLE_SYSTEM_ID;

}  // end setSlave

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ParticleSystem::setSaveable(Bool b)
{
	m_isSaveable = b;
	if (m_slaveSystem)
		m_slaveSystem->setSaveable(b);
}

// ------------------------------------------------------------------------------------------------
/** (Re)start a stopped particle system */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::start( void )
{
	m_isStopped = false;
}

// ------------------------------------------------------------------------------------------------
/** Stop a particle system from emitting */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::stop( void )
{
	m_isStopped = true;
}

// ------------------------------------------------------------------------------------------------
/** Stop emitting, wait for all of our particles to die, then destroy self. */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::destroy( void )
{
	m_isDestroyed = true;
	if( m_slaveSystem )
	{
		m_slaveSystem->destroy();  // If we don't it will leak forever.  We are solely responsible for it.
	}
}

// ------------------------------------------------------------------------------------------------
/** Get the position of the particle system */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::getPosition( Coord3D *pos )
{
	Vector3 vec;
	m_localTransform.Get_Translation(&vec);
	if (pos)
	{	pos->x=vec.X;
		pos->y=vec.Y;
		pos->z=vec.Z;
	}
}

// ------------------------------------------------------------------------------------------------
/** Set the position of the particle system */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::setPosition( const Coord3D *pos )
{
	m_localTransform.Set_X_Translation( pos->x );
	m_localTransform.Set_Y_Translation( pos->y );
	m_localTransform.Set_Z_Translation( pos->z );
	m_isLocalIdentity = false;
}

// ------------------------------------------------------------------------------------------------
/** Set the system's local transform */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::setLocalTransform( const Matrix3D *matrix )
{
	m_localTransform = *matrix;
	m_isLocalIdentity = false;
}

// ------------------------------------------------------------------------------------------------
/** Rotate local transform matrix */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::rotateLocalTransformX( Real x )
{
	m_localTransform.Rotate_X( x );
	m_isLocalIdentity = false;
}

// ------------------------------------------------------------------------------------------------
/** Rotate local transform matrix */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::rotateLocalTransformY( Real y )
{
	m_localTransform.Rotate_Y( y );
	m_isLocalIdentity = false;
}

// ------------------------------------------------------------------------------------------------
/** Rotate local transform matrix */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::rotateLocalTransformZ( Real z )
{
	m_localTransform.Rotate_Z( z );
	m_isLocalIdentity = false;
}

// ------------------------------------------------------------------------------------------------
/** Attach this particle system to a Drawable */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::attachToDrawable( const Drawable *draw )
{
	if (draw)
		m_attachedToDrawableID = draw->getID();
	else
		m_attachedToDrawableID = INVALID_DRAWABLE_ID;
}

// ------------------------------------------------------------------------------------------------
/** Attach this particle system to a Drawable */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::attachToObject( const Object *obj )
{
	if (obj)
		m_attachedToObjectID = obj->getID();
	else
		m_attachedToObjectID = INVALID_ID;
}

// ------------------------------------------------------------------------------------------------
/** Compute a random point on a unit sphere
 * @todo The density of random points generated is not uniform within the sphere */
// ------------------------------------------------------------------------------------------------
const Coord3D *ParticleSystem::computePointOnUnitSphere( void )
{
	static Coord3D point;

	do
	{
		point.x = GameClientRandomValueReal( -1.0f, 1.0f );
		point.y = GameClientRandomValueReal( -1.0f, 1.0f );
		point.z = GameClientRandomValueReal( -1.0f, 1.0f );
	}
	while (point.x == 0.0f && point.y == 0.0f && point.z == 0.0f);

	point.normalize();

	return &point;
}

// ------------------------------------------------------------------------------------------------
/** Compute a velocity vector based on emission properties */
// ------------------------------------------------------------------------------------------------
const Coord3D *ParticleSystem::computeParticleVelocity( const Coord3D *pos )
{
	static Coord3D newVel;

	switch( m_emissionVelocityType )
	{
		case ORTHO:
			newVel.x = m_emissionVelocity.ortho.x.getValue();
			newVel.y = m_emissionVelocity.ortho.y.getValue();
			newVel.z = m_emissionVelocity.ortho.z.getValue();
			break;
		
		case CYLINDRICAL:
		{
			Real radialSpeed, angle;
			radialSpeed = m_emissionVelocity.cylindrical.radial.getValue();

			angle = GameClientRandomValueReal( 0, 2.0f*PI );
			newVel.x = radialSpeed * cos( angle );
			newVel.y = radialSpeed * sin( angle );

			newVel.z = m_emissionVelocity.cylindrical.normal.getValue();
			break;
		}

		// "outward" velocity is directed along the surface normal of the emission volume
		case OUTWARD:
		{
			Real speed = m_emissionVelocity.outward.speed.getValue();
			Real otherSpeed = m_emissionVelocity.outward.otherSpeed.getValue();
			Coord3D sysPos;

			/*
			sysPos.x = m_localTransform.Get_X_Translation();
			sysPos.y = m_localTransform.Get_Y_Translation();
			sysPos.z = m_localTransform.Get_Z_Translation();
			*/
			sysPos.x = 0.0f;
			sysPos.y = 0.0f;
			sysPos.z = 0.0f;

			switch( m_emissionVolumeType )
			{
				case CYLINDER:
					Coord2D disk;

					disk.x = pos->x - sysPos.x;
					disk.y = pos->y - sysPos.y;
					disk.normalize();

					newVel.x = speed * disk.x;
					newVel.y = speed * disk.y;
					newVel.z = otherSpeed;
					break;

				case BOX:				///< @todo Implement BOX OUTWARD velocity
				case SPHERE:
				{
					newVel.x = pos->x - sysPos.x;
					newVel.y = pos->y - sysPos.y;
					newVel.z = pos->z - sysPos.z;
					newVel.normalize();

					newVel.x *= speed;
					newVel.y *= speed;
					newVel.z *= speed;
					break;
				}

				case LINE:
				{
					Coord3D along;			// unit vector along line direction

					along.x = m_emissionVolume.line.end.x - m_emissionVolume.line.start.x;
					along.y = m_emissionVolume.line.end.y - m_emissionVolume.line.start.y;
					along.z = m_emissionVolume.line.end.z - m_emissionVolume.line.start.z;
					along.normalize();

					Coord3D perp;				// unit vector perpendicular to the along/up plane
					Coord3D up;					// unit vector in the up direction (Z)
					up.x = 0.0;
					up.y = 0.0;
					up.z = 1.0;
					perp.crossProduct( &up, &along, &perp );
					up.crossProduct( &along, &perp, &up );

					// "speed" is in 'horizontal' plane, and "otherSpeed" is 'vertical'
					newVel.x = speed * perp.x + otherSpeed * up.x;
					newVel.y = speed * perp.y + otherSpeed * up.y;
					newVel.z = speed * perp.z + otherSpeed * up.z;								
					break;
				}

				case POINT:
				{
					Coord3D vel = *computePointOnUnitSphere();

					newVel.x = speed * vel.x;
					newVel.y = speed * vel.y;
					newVel.z = speed * vel.z;
					break;
				}
			}

			break;
		}

		case SPHERICAL:
		{
			Real speed = m_emissionVelocity.spherical.speed.getValue();
			Coord3D vel = *computePointOnUnitSphere();

			newVel.x = speed * vel.x;
			newVel.y = speed * vel.y;
			newVel.z = speed * vel.z;
			break;
		}

		case HEMISPHERICAL:
		{
			Coord3D vel;
			Real speed = m_emissionVelocity.spherical.speed.getValue();

			do
			{
				vel.x = GameClientRandomValueReal( -1.0f, 1.0f );
				vel.y = GameClientRandomValueReal( -1.0f, 1.0f );
				vel.z = GameClientRandomValueReal( 0.0f, 1.0f );
			}
			while (vel.x == 0.0f && vel.y == 0.0f && vel.z == 0.0f);

			vel.normalize();

			newVel.x = speed * vel.x;
			newVel.y = speed * vel.y;
			newVel.z = speed * vel.z;
			break;
		}

		default:
			newVel.x = 0.0f;
			newVel.y = 0.0f;
			newVel.z = 0.0f;
			break;
	}

	// scale the velocity by the velocity multiplier
	newVel.x *= m_velCoeff.x*(0.5f+TheGlobalData->m_particleScale/2.0f);
	newVel.y *= m_velCoeff.y*(0.5f+TheGlobalData->m_particleScale/2.0f);
	newVel.z *= m_velCoeff.z*(0.5f+TheGlobalData->m_particleScale/2.0f);

	return &newVel;
}

// ------------------------------------------------------------------------------------------------
/** Compute a position based on emission properties */
// ------------------------------------------------------------------------------------------------
const Coord3D *ParticleSystem::computeParticlePosition( void )
{
	static Coord3D newPos;

	switch( m_emissionVolumeType )
	{
		case CYLINDER:
		{
			Real angle = GameClientRandomValueReal( 0, 2.0f*PI );
			Real radius;
			
			if (m_isEmissionVolumeHollow)
				radius = m_emissionVolume.cylinder.radius;
			else
				radius = GameClientRandomValueReal( 0.0f, m_emissionVolume.cylinder.radius );

			newPos.x = radius * cos( angle );
			newPos.y = radius * sin( angle );

			Real halfLength = m_emissionVolume.cylinder.length/2.0f;
			newPos.z = GameClientRandomValueReal( -halfLength, halfLength );

			break;
		}

		case SPHERE:
		{
			Real radius;

			if (m_isEmissionVolumeHollow)
				radius = m_emissionVolume.sphere.radius;
			else
				radius = GameClientRandomValueReal( 0.0f, m_emissionVolume.sphere.radius );

			newPos = *computePointOnUnitSphere();

			newPos.x *= radius;
			newPos.y *= radius;
			newPos.z *= radius;

			break;
		}

		case BOX:
		{			
			if (m_isEmissionVolumeHollow) {
				// determine which side to generate on.
				// 0 is bottom, 3 is top, 
				// 1 is left , 4 is right
				// 2 is front, 5 is right back

				int side = GameClientRandomValue(0, 6);
				if (side % 3 == 0) {
					// generate X, Y
					newPos.x = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.x, m_emissionVolume.box.halfSize.x );
					newPos.y = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.y, m_emissionVolume.box.halfSize.y );
					if (side == 0) {
						newPos.z = -m_emissionVolume.box.halfSize.z;
					} else {
						newPos.z = m_emissionVolume.box.halfSize.z;
					}
															
				} else if (side % 3 == 1) {
					// generate Y, Z
					newPos.y = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.y, m_emissionVolume.box.halfSize.y );
					newPos.z = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.z, m_emissionVolume.box.halfSize.z );
					if (side == 1) {
						newPos.x = -m_emissionVolume.box.halfSize.x;
					} else {
						newPos.x = m_emissionVolume.box.halfSize.y;
					}

				} else if (side % 3 == 2) {
					// generate X, Z
					newPos.x = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.x, m_emissionVolume.box.halfSize.x );
					newPos.z = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.z, m_emissionVolume.box.halfSize.z );
					if (side == 2) {
						newPos.y = -m_emissionVolume.box.halfSize.y;
					} else {
						newPos.y = m_emissionVolume.box.halfSize.y;
					}
				}
			} else {
				newPos.x = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.x, m_emissionVolume.box.halfSize.x );
				newPos.y = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.y, m_emissionVolume.box.halfSize.y );
				newPos.z = GameClientRandomValueReal( -m_emissionVolume.box.halfSize.z, m_emissionVolume.box.halfSize.z );
			break;
			}
		}

		case LINE:
		{
			Coord3D delta, start, end;

			start = m_emissionVolume.line.start;
			end = m_emissionVolume.line.end;

			delta.x = end.x - start.x;
			delta.y = end.y - start.y;
			delta.z = end.z - start.z;

			Real t = GameClientRandomValueReal( 0.0f, 1.0f );

			newPos.x = start.x + t * delta.x;
			newPos.y = start.y + t * delta.y;
			newPos.z = start.z + t * delta.z;
			break;
		}

		case POINT:
		default:
			newPos.x = 0.0f;
			newPos.y = 0.0f;
			newPos.z = 0.0f;
			break;
	}
	newPos.x *= (0.5f+TheGlobalData->m_particleScale/2.0f);
	newPos.y *= (0.5f+TheGlobalData->m_particleScale/2.0f);
	newPos.z *= (0.5f+TheGlobalData->m_particleScale/2.0f);
	return &newPos;
}

// ------------------------------------------------------------------------------------------------
/** Factory method for particles. */
// ------------------------------------------------------------------------------------------------
Particle *ParticleSystem::createParticle( const ParticleInfo *info, 
																					ParticlePriorityType priority,
																					Bool forceCreate )
{

	//
	// if we aren't absolutely forcing this particle to be created (which is needed when
	// loading and creating particle systems from the save games) we need to check a few
	// restrictions before this particle can really be created
	//
	if( forceCreate == FALSE )
	{

		if (TheGlobalData->m_useFX == FALSE)
			return NULL;

		//
		// Enforce particle limit.
		// If we are at the limit, destroy the oldest particle in order
		// to make room for this one.
		//

		//
		// Check if particle is below priorities we allow for this FPS or if it being skipped because 
		// all particesl are being skipped (excluding special fps independent particles at 
		// getMinDynamicParticleSkipPriority()) 
		//
		if( priority < TheGameLODManager->getMinDynamicParticlePriority() ||
				(priority < TheGameLODManager->getMinDynamicParticleSkipPriority() && 
				 TheGameLODManager->isParticleSkipped()) )
			return NULL;

		if ( getParticleCount() > 0 && priority == AREA_EFFECT && m_isGroundAligned && TheParticleSystemManager->getFieldParticleCount() > (UnsignedInt)TheGlobalData->m_maxFieldParticleCount )
			return NULL;
		
		// ALWAYS_RENDER particles are exempt from all count limits, and are always created, regardless of LOD issues.
		if (priority != ALWAYS_RENDER)
		{
			int numInExcess = TheParticleSystemManager->getParticleCount() - (UnsignedInt)TheGlobalData->m_maxParticleCount;
			if ( numInExcess > 0)
			{
				if( TheParticleSystemManager->removeOldestParticles((UnsignedInt) numInExcess, priority) != numInExcess )
					return NULL;  // could not remove enough particles, don't create new stuff
			}

			if (TheGlobalData->m_maxParticleCount == 0)
				return NULL;
		}

	}  // end if

	Particle *p = newInstance(Particle)( this, info );
	return p;

}

// ------------------------------------------------------------------------------------------------
/** Generate a new, random set of ParticleInfo
 * particleNum and particleCount are used to get 'tween frame particles emitted in the correct
 * place. (jkmcd) */
// ------------------------------------------------------------------------------------------------
const ParticleInfo *ParticleSystem::generateParticleInfo( Int particleNum, Int particleCount )
{
	static ParticleInfo info;
	if (particleCount == 0) {
		DEBUG_CRASH(("particleCount must NOT be 0. Set to 1 or greater."));
		return &info;
	}

	// NOTE: position MUST be computed before velocity, in case OUTWARD velocity is
	// specified, which must know where the particle is in space.
	info.m_pos = *computeParticlePosition();
	info.m_vel = *computeParticleVelocity( &info.m_pos );

	// transform the position and velocity, if necessary
	/// @todo Avoid conversion from Coord3D to Vector3 somehow
	if (m_isIdentity == false)
	{
		// transform particle position to world coordinates
		Vector3 p, pr;
		
		Coord3D emissionAdjustment;	// this is the adjustment for inter-frame emission
		// @todo : This should work, if m_lastPos = m_pos is removed from here but it doesn't. 
		// @todo : Investigate why. jkmcd
		if (m_isFirstPos) {
			m_lastPos = m_pos;
			m_isFirstPos = false;
		}
		
		emissionAdjustment.x = (1 - (INT_TO_REAL(particleNum) / particleCount)) * (m_pos.x - m_lastPos.x);
		emissionAdjustment.y = (1 - (INT_TO_REAL(particleNum) / particleCount)) * (m_pos.y - m_lastPos.y);
		emissionAdjustment.z = (1 - (INT_TO_REAL(particleNum) / particleCount)) * (m_pos.z - m_lastPos.z);

		p.X = info.m_pos.x;
		p.Y = info.m_pos.y;
		p.Z = info.m_pos.z;

#ifdef ALLOW_TEMPORARIES
		pr = m_transform * p;
#else
		m_transform.mulVector3(p, pr);
#endif

		info.m_pos.x = pr.X - emissionAdjustment.x;
		info.m_pos.y = pr.Y - emissionAdjustment.y;
		info.m_pos.z = pr.Z - emissionAdjustment.z;

		// transform particle velocity to world coordinates
		Vector3 v, vr;

		v.X = info.m_vel.x;
		v.Y = info.m_vel.y;
		v.Z = info.m_vel.z;

		Matrix3D::Rotate_Vector( m_transform, v, &vr );

		info.m_vel.x = vr.X;
		info.m_vel.y = vr.Y;
		info.m_vel.z = vr.Z;
	}

	info.m_velDamping = m_velDamping.getValue();
	info.m_angularDamping = m_angularDamping.getValue();

	info.m_angleX = m_angleX.getValue();
	info.m_angleY = m_angleY.getValue();
	info.m_angleZ = m_angleZ.getValue();
	info.m_angularRateX = m_angularRateX.getValue();
	info.m_angularRateY = m_angularRateY.getValue();
	info.m_angularRateZ = m_angularRateZ.getValue();

	info.m_lifetime = (UnsignedInt)m_lifetime.getValue();

	info.m_size = m_startSize.getValue()*m_sizeCoeff*TheGlobalData->m_particleScale;
	info.m_sizeRate = m_sizeRate.getValue()*m_sizeCoeff*TheGlobalData->m_particleScale;
	info.m_sizeRateDamping = m_sizeRateDamping.getValue();

	// Keeping a running tally makes each successive particle spawned start a bit bigger (or smaller).
	info.m_size += m_accumulatedSizeBonus;
	m_accumulatedSizeBonus += m_startSizeRate.getValue();
	if( m_accumulatedSizeBonus )
		m_accumulatedSizeBonus = min( m_accumulatedSizeBonus, (float)MAX_SIZE_BONUS );

	for( int i=0; i<MAX_KEYFRAMES; i++ )
	{
		info.m_alphaKey[i].value = m_alphaKey[i].var.getValue();
		info.m_alphaKey[i].frame = m_alphaKey[i].frame;
	}

/*
	info.m_color.red = m_color.red.getValue();
	info.m_color.green = m_color.green.getValue();
	info.m_color.blue = m_color.blue.getValue();
*/

	for( int i=0; i<MAX_KEYFRAMES; i++ )
	{
		info.m_colorKey[i] = m_colorKey[i];
	}

	info.m_colorScale = m_colorScale.getValue();
#ifdef ALLOW_TEMPORARIES
	Vector3 pos = m_transform * Vector3(0, 0, 0);
#else
	Vector3 pos;
	m_transform.mulVector3(Vector3(0, 0, 0), pos);
#endif
	info.m_emitterPos.x = pos.X;
	info.m_emitterPos.y = pos.Y;
	info.m_emitterPos.z = pos.Z;
	info.m_particleUpTowardsEmitter = m_isParticleUpTowardsEmitter;

	info.m_windRandomness = GameClientRandomValueReal( 0.7f, 1.3f );

	return &info;
}

// ------------------------------------------------------------------------------------------------
/** Update this particle system, potentially generating new particles */
// ------------------------------------------------------------------------------------------------
Bool ParticleSystem::update( Int localPlayerIndex  )
{
	if (TheGlobalData->m_useFX == FALSE)
		return false;

	// do initial delay ... note, this currently delays the lifetime
	if (m_delayLeft)
	{
		--m_delayLeft;

		// system actually "starts" once initial delay is over
		/// @todo reset start time when system is stopped/started
		if (m_delayLeft == 0)
			m_startTimestamp = TheGameClient->getFrame();

		return true;
	}

	// update the wind motion
	updateWindMotion();

	// if this system is attached to a Drawable/Object, update the current transform
	// matrix so generated particles' are relative to the parent Drawable's
	// position and orientation
	Bool transformSet = false;
	const Matrix3D *parentXfrm = NULL;
	Bool isShrouded = false;

	if (m_attachedToDrawableID)
	{
		Drawable *attachedTo = TheGameClient->findDrawableByID( m_attachedToDrawableID );

		if (attachedTo)
		{
			if (attachedTo->getFullyObscuredByShroud())
				isShrouded = true;

			parentXfrm = attachedTo->getTransformMatrix();
			m_lastPos = m_pos;
			m_pos = *attachedTo->getPosition();
		}
		else
		{ 
			// Drawable has been destroyed - lose our attachment to it
			m_attachedToDrawableID = INVALID_DRAWABLE_ID;

			// destroy ourselves
			destroy();
		}
	}

	if (m_attachedToObjectID)
	{
		Object *attachedTo = TheGameLogic->findObjectByID( m_attachedToObjectID );

		if (attachedTo)
		{
			if (!isShrouded)
				isShrouded = (attachedTo->getShroudedStatus(localPlayerIndex) >= OBJECTSHROUD_FOGGED);
 
			parentXfrm = attachedTo->getTransformMatrix();
			m_lastPos = m_pos;
			m_pos = *attachedTo->getPosition();
		}
		else
		{ 
			// Drawable has been destroyed - lose our attachment to it
			m_attachedToObjectID = INVALID_ID;

			// destroy ourselves
			destroy();
		}
	}

	if (parentXfrm)
	{
		// if system has its own local transform, concatenate them
		if (m_isLocalIdentity == false)
#ifdef ALLOW_TEMPORARIES
			m_transform = (*parentXfrm) * m_localTransform;
#else
			m_transform.mul(*parentXfrm, m_localTransform);
#endif
		else
			m_transform = *parentXfrm;

		m_isIdentity = false;
		transformSet = true;
	}


	if (transformSet == false)
	{
		if (m_isLocalIdentity == false)
		{
			m_transform = m_localTransform;
			m_isIdentity = false;
		}
		else
		{
			m_isIdentity = true;
		}
	}

	// if we are controlled by a particle, its position is local origin
	if (m_controlParticle)
	{
		const Coord3D *controlPos = m_controlParticle->getPosition();
		/// @todo Concatenate this, instead of overriding (MSB)
		m_transform.Set_X_Translation( controlPos->x );
		m_transform.Set_Y_Translation( controlPos->y );
		m_transform.Set_Z_Translation( controlPos->z );
		m_isIdentity = false;
		m_lastPos = m_pos;
		m_pos = *controlPos;
	}


	//
	// Generate new particles if the system hasn't been 'stopped' or 'destroyed'
	// If we are a slave system, do not generate particles ourselves - our master will force us to
	//
	if (m_isDestroyed == false)
	{
		if (m_isForever || (m_isForever == false && m_systemLifetimeLeft > 0))
		{
			if (!isShrouded && m_isStopped == false && m_masterSystem == NULL)
			{
				if (m_burstDelayLeft == 0)
				{
					ParticlePriorityType priority = getPriority();

					// emit a burst of particles
					Int count = REAL_TO_INT(m_burstCount.getValue());

					count *= m_countCoeff;

					for( Int i=0; i<count; i++ )
					{
						// generate this particle's unique attributes
						const ParticleInfo *info = generateParticleInfo(i, count);
						if (!m_isEmitAboveGroundOnly || (info->m_pos.z >= TheTerrainLogic->getGroundHeight(info->m_pos.x, info->m_pos.y)))
						{
							// actually create a particle
							Particle *p = createParticle( info, priority );
							if (p == NULL)
								continue;

							if (m_attachedSystemName.isEmpty() == false)
							{
								const ParticleSystemTemplate *tmp = TheParticleSystemManager->findTemplate( m_attachedSystemName );
								if (tmp)
								{
									ParticleSystem *sys = TheParticleSystemManager->createParticleSystem( tmp, TRUE );
									sys->setControlParticle( p );
									p->controlParticleSystem( sys );
								}
							}

							// create a slave particle, if necessary
							if (m_slaveSystem)
							{
								ParticleInfo mergeInfo = ParticleSystem::mergeRelatedParticleSystems(this, m_slaveSystem, false);

								// create slaved particle
								m_slaveSystem->createParticle( &mergeInfo, priority );
							}
						}
					}
						
					// compute next burst delay
					m_burstDelayLeft = (UnsignedInt)m_burstDelay.getValue();
					m_burstDelayLeft *= m_delayCoeff;
				}
				else
				{
					m_burstDelayLeft--;
				}

			} // end if stopped check
		} // end if system lifetime check
	} // end if is destroyed

	//
	// Update all particles in the system
	//
	Particle *p = m_systemParticlesHead;
	Particle *oldParticle;
	while (p)
	{

		// apply 'gravity' force
		if (m_gravity != 0.0f)
		{
			Coord3D force;
			force.x = 0.0f;
			force.y = 0.0f;
			force.z = m_gravity;
			p->applyForce( &force );
		}

		if (p->update() == false)
		{
			oldParticle = p;
			p = p->m_systemNext;
			oldParticle->deleteInstance();
		} else {
			p = p->m_systemNext;
		}
	}

	//
	// If we have been "destroyed", wait for all of our particles to die off,
	// then destroy ourselves (return false).
	//
	if (m_isDestroyed && !m_systemParticlesHead)
		return false;


	// monitor particle system lifetime
	if (m_isForever == false)
	{
		// decrement lifetime if not zero
		if (m_systemLifetimeLeft)
			m_systemLifetimeLeft--;

		// if there are still particles "in the air", don't destroy yet
		if (getParticleCount())
			return true;

		// check if time is up
		if (m_systemLifetimeLeft == 0)
			return false;
	}

	return true;
}

// ------------------------------------------------------------------------------------------------
/** Update the wind motion */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::updateWindMotion( void )
{

	switch( m_windMotion )
	{

		// --------------------------------------------------------------------------------------------
		case ParticleSystemInfo::WIND_MOTION_PING_PONG:
		{
			Real startAngle = m_windMotionStartAngle;
			Real endAngle = m_windMotionEndAngle;

			// this only works when start angle is less than end angle
			DEBUG_ASSERTCRASH( startAngle < endAngle, ("updateWindMotion: startAngle must be < endAngle\n") );

			// how big is the total angle span
			Real totalSpan = endAngle - startAngle;
			Real halfSpan = totalSpan / 2.0f;

			// given our current angle ... how far away from the "center" of the span are we
			Real diffFromCenter = fabs( halfSpan - m_windAngle + startAngle );
	
			//
			// given our distance from the center ... we need to compute how much we will change
			// the angle.  When we are closer to the center we change it faster (more), and when
			// we are near the edges we change is slower (less)
			//
			Real change = (1.0f - (diffFromCenter / halfSpan)) * m_windAngleChange;

			// we will always change a little bit
			#define MINIMUM_CHANGE 0.005f  // lower #'s have softer swings at the edge angles
			if( change < MINIMUM_CHANGE )
				change = MINIMUM_CHANGE;
			
			//
			// if we are moving toward the end angle we add the change, if we're moving away
			// from it we subtract it
			//
			if( m_windMotionMovingToEndAngle )
			{

				// add angle
				m_windAngle += change;

				// see if we're at the end and should switch directions
				if( m_windAngle >= endAngle )
				{

					// change directions
					m_windMotionMovingToEndAngle = FALSE;

					// pick a new change delta
					m_windAngleChange = 
							GameClientRandomValueReal( m_windAngleChangeMin, m_windAngleChangeMax );

					// pick new start and end angles
					m_windMotionStartAngle = 
							GameClientRandomValueReal( m_windMotionStartAngleMin, 
																				 m_windMotionStartAngleMax );
					m_windMotionEndAngle = 
							GameClientRandomValueReal( m_windMotionEndAngleMin, 
																				 m_windMotionEndAngleMax );

				}  // end if

			}  // end if
			else
			{

				// subtract angle
				m_windAngle -= change;

				// see if we're at the end and should switch directions
				if( m_windAngle <= startAngle )
				{

					// change directions
					m_windMotionMovingToEndAngle = TRUE;

					// pick a new change delta
					m_windAngleChange = 
							GameClientRandomValueReal( m_windAngleChangeMin, m_windAngleChangeMax );

					// pick new start and end angles
					m_windMotionStartAngle = 
							GameClientRandomValueReal( m_windMotionStartAngleMin, 
																				 m_windMotionStartAngleMax );
					m_windMotionEndAngle = 
							GameClientRandomValueReal( m_windMotionEndAngleMin, 
																				 m_windMotionEndAngleMax );

				}  // end if

			}  // end else

			break;

		}  // end case

		// --------------------------------------------------------------------------------------------
		case ParticleSystemInfo::WIND_MOTION_CIRCULAR:
		{

			// give us a wind angle change if one hasn't been specifed (this plays nice with the particle editor)
			if( m_windAngleChange == 0.0f )
				m_windAngleChange = GameClientRandomValueReal( m_windAngleChangeMin, m_windAngleChangeMax );

			// add to our wind angle
			m_windAngle += m_windAngleChange;
			
			// keep in 0 to 2PI range just to keep the numbers safe and sane
			if( m_windAngle > TWO_PI )
				m_windAngle -= TWO_PI;
			else if( m_windAngle < 0.0f )
				m_windAngle += TWO_PI;

			break;

		}  // end case

		// ---------------------------------------------------------------------------------------------
		default:
		{

			break;

		}  // end default

	}  // end if

}  // end updateWindMotion

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ParticleSystem::addParticle( Particle *particleToAdd )
{
	if (particleToAdd->m_inSystemList)
		return;

	if (!m_systemParticlesHead)
	{
		m_systemParticlesHead = particleToAdd;
	}

	if (m_systemParticlesTail)
	{
		m_systemParticlesTail->m_systemNext = particleToAdd;
		particleToAdd->m_systemPrev = m_systemParticlesTail;
	}
	else
	{
		particleToAdd->m_systemPrev = NULL;
	}

	m_systemParticlesTail = particleToAdd;
	particleToAdd->m_systemNext = NULL;
	particleToAdd->m_inSystemList = TRUE;

	++m_particleCount;

	particleToAdd->setPersonality( m_personalityStore++ ); 

}

// ------------------------------------------------------------------------------------------------
/** Remove given particle from the list - ONLY FOR USE BY PARTICLE */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::removeParticle( Particle *particleToRemove )
{
	if (!particleToRemove->m_inSystemList)
		return;

	// remove links from prev & next objs
	if (particleToRemove->m_systemNext)
		particleToRemove->m_systemNext->m_systemPrev = particleToRemove->m_systemPrev;
	if (particleToRemove->m_systemPrev)
		particleToRemove->m_systemPrev->m_systemNext = particleToRemove->m_systemNext;

	// update head & tail if neccessary
	if (particleToRemove == m_systemParticlesHead)
		m_systemParticlesHead = particleToRemove->m_systemNext;
	if (particleToRemove == m_systemParticlesTail)
		m_systemParticlesTail = particleToRemove->m_systemPrev;

	particleToRemove->m_systemNext = particleToRemove->m_systemPrev = NULL;
	particleToRemove->m_inSystemList = FALSE;
	--m_particleCount;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ParticleInfo ParticleSystem::mergeRelatedParticleSystems( ParticleSystem *masterParticleSystem, ParticleSystem *slaveParticleSystem, Bool slaveNeedsFullPromotion)
{
	if (!masterParticleSystem || !slaveParticleSystem) {
		DEBUG_CRASH(("masterParticleSystem or slaveParticleSystem was NULL. Should not happen. JKMCD"));
		ParticleInfo bogus;
		memset( &bogus, 0, sizeof( bogus ) );
		return bogus;
	}

	// copy info
	ParticleInfo mergeInfo = *masterParticleSystem->generateParticleInfo(1, 1);

	// generate one from the slave system
	const ParticleInfo *info = slaveParticleSystem->generateParticleInfo(1, 1);

	// override unique attributes of slave particle
	mergeInfo.m_lifetime = info->m_lifetime;

	// size becomes a scale factor of master's particles
	mergeInfo.m_size *= info->m_size;
	mergeInfo.m_sizeRate *= info->m_sizeRate;
	mergeInfo.m_sizeRateDamping *= info->m_sizeRateDamping;

	mergeInfo.m_angleX = info->m_angleX;
	mergeInfo.m_angleY = info->m_angleY;
	mergeInfo.m_angleZ = info->m_angleZ;
	mergeInfo.m_angularRateX = info->m_angularRateX;
	mergeInfo.m_angularRateY = info->m_angularRateY;
	mergeInfo.m_angularRateZ = info->m_angularRateZ;
	mergeInfo.m_angularDamping = info->m_angularDamping;

	for( int i=0; i<MAX_KEYFRAMES; i++ )
		mergeInfo.m_alphaKey[i] = info->m_alphaKey[i];

	for( int i=0; i<MAX_KEYFRAMES; i++ )
		mergeInfo.m_colorKey[i] = info->m_colorKey[i];

	mergeInfo.m_colorScale = info->m_colorScale;

	// offset slave's position relative to master's
	const Coord3D *offset = slaveParticleSystem->getSlavePositionOffset();
	mergeInfo.m_pos.x += offset->x;
	mergeInfo.m_pos.y += offset->y;
	mergeInfo.m_pos.z += offset->z;

	if (slaveNeedsFullPromotion) {
		slaveParticleSystem->m_burstCount = masterParticleSystem->m_burstCount;
		slaveParticleSystem->m_burstDelay = masterParticleSystem->m_burstDelay;

		slaveParticleSystem->m_priority = masterParticleSystem->m_priority;
		slaveParticleSystem->m_emissionVelocity = masterParticleSystem->m_emissionVelocity;
		slaveParticleSystem->m_emissionVelocityType = masterParticleSystem->m_emissionVelocityType;
		slaveParticleSystem->m_emissionVolume = masterParticleSystem->m_emissionVolume;
		slaveParticleSystem->m_emissionVolumeType = masterParticleSystem->m_emissionVolumeType;
		slaveParticleSystem->m_isEmissionVolumeHollow = masterParticleSystem->m_isEmissionVolumeHollow;

		
		slaveParticleSystem->m_startSize.setRange(masterParticleSystem->m_startSize.getMinimumValue() * slaveParticleSystem->m_startSize.getMinimumValue(),
																							masterParticleSystem->m_startSize.getMaximumValue() * slaveParticleSystem->m_startSize.getMaximumValue(),
																							masterParticleSystem->m_startSize.getDistributionType());
	
		slaveParticleSystem->m_sizeRate.setRange(masterParticleSystem->m_sizeRate.getMinimumValue() * slaveParticleSystem->m_sizeRate.getMinimumValue(),
																							masterParticleSystem->m_sizeRate.getMaximumValue() * slaveParticleSystem->m_sizeRate.getMaximumValue(),
																							masterParticleSystem->m_sizeRate.getDistributionType());

		slaveParticleSystem->m_sizeRateDamping.setRange(masterParticleSystem->m_sizeRateDamping.getMinimumValue() * slaveParticleSystem->m_sizeRateDamping.getMinimumValue(),
																							masterParticleSystem->m_sizeRateDamping.getMaximumValue() * slaveParticleSystem->m_sizeRateDamping.getMaximumValue(),
																							masterParticleSystem->m_sizeRateDamping.getDistributionType());
		
//		slaveParticleSystem->m_burstCount.setRange(masterParticleSystem->m_burstCount.getMinimumValue() / 2, 
//																							 masterParticleSystem->m_burstCount.getMaximumValue() / 2,
//																							 masterParticleSystem->m_burstCount.getDistributionType());
		
	}

	return mergeInfo;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ParticleSystem::setLifetimeRange( Real min, Real max )
{
	m_lifetime.setRange( min, max );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ParticleSystem::setControlParticle( Particle *p )
{
	m_controlParticle = p;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::crc( Xfer *xfer )
{

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// base class info
	ParticleSystemInfo::xfer( xfer );

	// particle system ID
	xfer->xferUser( &m_systemID, sizeof( ParticleSystemID ) );

	// attached to drawable id
	xfer->xferDrawableID( &m_attachedToDrawableID );

	// attached to object id
	xfer->xferObjectID( &m_attachedToObjectID );

	// is local identity
	xfer->xferBool( &m_isLocalIdentity );

	// local transform
	xfer->xferUser( &m_localTransform, sizeof( Matrix3D ) );

	// is identity
	xfer->xferBool( &m_isIdentity );

	// transform
	xfer->xferUser( &m_transform, sizeof( Matrix3D ) );

	// burst delay left
	xfer->xferUnsignedInt( &m_burstDelayLeft );

	// delay left
	xfer->xferUnsignedInt( &m_delayLeft );

	// start timestamp
	xfer->xferUnsignedInt( &m_startTimestamp );

	// system lifetime left
	xfer->xferUnsignedInt( &m_systemLifetimeLeft );

	// personality store
	xfer->xferUnsignedInt( &m_personalityStore );

	// is forever
	xfer->xferBool( &m_isForever );

	// accumulated size bonus
	xfer->xferReal( &m_accumulatedSizeBonus );

	// is stopped
	xfer->xferBool( &m_isStopped );

	// we never save destroyed particle systems so there is no need to consider m_isDestroyed
	// m_isDestroyed <-- do nothing with me

	// ditto for m_isSaveable
	// m_isSaveable <-- do nothing with me

	// velCoeff
	xfer->xferCoord3D( &m_velCoeff );

	// count coeff
	xfer->xferReal( &m_countCoeff );

	// delay coeff
	xfer->xferReal( &m_delayCoeff );

	// size coeff
	xfer->xferReal( &m_sizeCoeff );

	// position
	xfer->xferCoord3D( &m_pos );

	// last position
	xfer->xferCoord3D( &m_lastPos );

	// is first pos
	xfer->xferBool( &m_isFirstPos );

	// slave system id
	xfer->xferUser( &m_slaveSystemID, sizeof( ParticleSystemID ) );

	// master system
	xfer->xferUser( &m_masterSystemID, sizeof( ParticleSystemID ) );

	// particle count
	UnsignedInt particleCount = m_particleCount;
	xfer->xferUnsignedInt( &particleCount );

	// particles
	if( xfer->getXferMode() == XFER_SAVE )
	{
		Particle *particle;

		// go through all particles in this system
		for( particle = m_systemParticlesHead; particle; particle = particle->m_systemNext )
		{

			// write particle information
			xfer->xferSnapshot( particle );

		}  // end for, particle

	}  // end if, save
	else
	{
		ParticlePriorityType priority = getPriority();
		const ParticleInfo *info = generateParticleInfo( 0, 1 );
		Particle *particle;

		// read each particle data block
		for( UnsignedInt i = 0; i < particleCount; ++i )
		{

			// create a new particle
			particle = createParticle( info, priority, TRUE );

			// sanity
			DEBUG_ASSERTCRASH( particle, ("ParticleSyste::xfer - Unable to create particle for loading\n") );

			// read in the particle data
			xfer->xferSnapshot( particle );

		}  // end for i

	}  // end else, load

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ParticleSystem::loadPostProcess( void )
{

	// call base class post process
	ParticleSystemInfo::loadPostProcess();

	// reconnect slave pointers if needed
	if( m_slaveSystemID != INVALID_PARTICLE_SYSTEM_ID )
	{

		// sanity
		if( m_slaveSystem != NULL )
		{

			DEBUG_CRASH(( "ParticleSystem::loadPostProcess - m_slaveSystem is not NULL but should be\n" ));
			throw SC_INVALID_DATA;

		}  // end if

		// assign system
		m_slaveSystem = TheParticleSystemManager->findParticleSystem( m_slaveSystemID );

		// sanity
		if( m_slaveSystem == NULL || m_slaveSystem->isDestroyed() == TRUE )
		{

			DEBUG_CRASH(( "ParticleSystem::loadPostProcess - m_slaveSystem is NULL or destroyed\n" ));
			throw SC_INVALID_DATA;

		}  // end if

	}  // end if

	// reconnect master pointers if needed
	if( m_masterSystemID != INVALID_PARTICLE_SYSTEM_ID )
	{

		// sanity
		if( m_masterSystem != NULL )
		{

			DEBUG_CRASH(( "ParticleSystem::loadPostProcess - m_masterSystem is not NULL but should be\n" ));
			throw SC_INVALID_DATA;

		}  // end if

		// assign system
		m_masterSystem = TheParticleSystemManager->findParticleSystem( m_masterSystemID );

		// sanity
		if( m_masterSystem == NULL || m_masterSystem->isDestroyed() == TRUE )
		{

			DEBUG_CRASH(( "ParticleSystem::loadPostProcess - m_masterSystem is NULL or destroyed\n" ));
			throw SC_INVALID_DATA;

		}  // end if

	}  // end if

}  // end loadPostProcess

///////////////////////////////////////////////////////////////////////////////////////////////////
// ParticleSystemTemplate /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
/** INI parse data */
// ------------------------------------------------------------------------------------------------
const FieldParse ParticleSystemTemplate::m_fieldParseTable[] = 
{
	{ "Priority",								INI::parseIndexList, ParticlePriorityNames, offsetof( ParticleSystemTemplate, m_priority ) },
	{ "IsOneShot",							INI::parseBool,						NULL,		offsetof( ParticleSystemTemplate, m_isOneShot ) },
	{ "Shader",									INI::parseIndexList,			ParticleShaderTypeNames,		offsetof( ParticleSystemTemplate, m_shaderType ) },
	{ "Type",										INI::parseIndexList,			ParticleTypeNames,		offsetof( ParticleSystemTemplate, m_particleType ) },
	{ "ParticleName",						INI::parseAsciiString,		NULL,		offsetof( ParticleSystemTemplate, m_particleTypeName ) },
	{ "AngleX",									INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_angleX ) },
	{ "AngleY",									INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_angleY ) },
	{ "AngleZ",									INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_angleZ ) },
	{ "AngularRateX",						INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_angularRateX ) },
	{ "AngularRateY",						INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_angularRateY ) },
	{ "AngularRateZ",						INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_angularRateZ ) },
	{ "AngularDamping",					INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_angularDamping ) },

	{ "VelocityDamping",				INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_velDamping ) },
	{ "Gravity",								INI::parseReal,																NULL,		offsetof( ParticleSystemTemplate, m_gravity ) },
	{ "SlaveSystem",						INI::parseAsciiString,												NULL,		offsetof( ParticleSystemTemplate, m_slaveSystemName ) },
	{ "SlavePosOffset",					INI::parseCoord3D,														NULL,		offsetof( ParticleSystemTemplate, m_slavePosOffset ) },
	{ "PerParticleAttachedSystem",		INI::parseAsciiString,								NULL,		offsetof( ParticleSystemTemplate, m_attachedSystemName ) },

	{ "Lifetime",								INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_lifetime ) },
	{ "SystemLifetime",					INI::parseUnsignedInt,												NULL,		offsetof( ParticleSystemTemplate, m_systemLifetime ) },

	{ "Size",										INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_startSize ) },
	{ "StartSizeRate",					INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_startSizeRate ) },
	{ "SizeRate",								INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_sizeRate ) },
	{ "SizeRateDamping",				INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_sizeRateDamping ) },

	{ "Alpha1",									ParticleSystemTemplate::parseRandomKeyframe,	NULL,		offsetof( ParticleSystemTemplate, m_alphaKey[0] ) },
	{ "Alpha2",									ParticleSystemTemplate::parseRandomKeyframe,	NULL,		offsetof( ParticleSystemTemplate, m_alphaKey[1] ) },
	{ "Alpha3",									ParticleSystemTemplate::parseRandomKeyframe,	NULL,		offsetof( ParticleSystemTemplate, m_alphaKey[2] ) },
	{ "Alpha4",									ParticleSystemTemplate::parseRandomKeyframe,	NULL,		offsetof( ParticleSystemTemplate, m_alphaKey[3] ) },
	{ "Alpha5",									ParticleSystemTemplate::parseRandomKeyframe,	NULL,		offsetof( ParticleSystemTemplate, m_alphaKey[4] ) },
	{ "Alpha6",									ParticleSystemTemplate::parseRandomKeyframe,	NULL,		offsetof( ParticleSystemTemplate, m_alphaKey[5] ) },
	{ "Alpha7",									ParticleSystemTemplate::parseRandomKeyframe,	NULL,		offsetof( ParticleSystemTemplate, m_alphaKey[6] ) },
	{ "Alpha8",									ParticleSystemTemplate::parseRandomKeyframe,	NULL,		offsetof( ParticleSystemTemplate, m_alphaKey[7] ) },

	{ "Color1",									ParticleSystemTemplate::parseRGBColorKeyframe,NULL,		offsetof( ParticleSystemTemplate, m_colorKey[0] ) },
	{ "Color2",									ParticleSystemTemplate::parseRGBColorKeyframe,NULL,		offsetof( ParticleSystemTemplate, m_colorKey[1] ) },
	{ "Color3",									ParticleSystemTemplate::parseRGBColorKeyframe,NULL,		offsetof( ParticleSystemTemplate, m_colorKey[2] ) },
	{ "Color4",									ParticleSystemTemplate::parseRGBColorKeyframe,NULL,		offsetof( ParticleSystemTemplate, m_colorKey[3] ) },
	{ "Color5",									ParticleSystemTemplate::parseRGBColorKeyframe,NULL,		offsetof( ParticleSystemTemplate, m_colorKey[4] ) },
	{ "Color6",									ParticleSystemTemplate::parseRGBColorKeyframe,NULL,		offsetof( ParticleSystemTemplate, m_colorKey[5] ) },
	{ "Color7",									ParticleSystemTemplate::parseRGBColorKeyframe,NULL,		offsetof( ParticleSystemTemplate, m_colorKey[6] ) },
	{ "Color8",									ParticleSystemTemplate::parseRGBColorKeyframe,NULL,		offsetof( ParticleSystemTemplate, m_colorKey[7] ) },

//	{ "COLOR",									ParticleSystemTemplate::parseRandomRGBColor,	NULL,		offsetof( ParticleSystemTemplate, m_color ) },
	{ "ColorScale",							INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_colorScale ) },

	{ "BurstDelay",							INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_burstDelay ) },
	{ "BurstCount",							INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_burstCount ) },

	{ "InitialDelay",						INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_initialDelay ) },

	{ "DriftVelocity",					INI::parseCoord3D,														NULL,		offsetof( ParticleSystemTemplate, m_driftVelocity ) },
	{ "VelocityType",						INI::parseIndexList,													EmissionVelocityTypeNames,		offsetof( ParticleSystemTemplate, m_emissionVelocityType ) },
	{ "VelOrthoX",							INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.ortho.x ) },
	{ "VelOrthoY",							INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.ortho.y ) },
	{ "VelOrthoZ",							INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.ortho.z ) },

	{ "VelSpherical",						INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.spherical.speed ) },
	{ "VelHemispherical",				INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.hemispherical.speed ) },

	{ "VelCylindricalRadial",		INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.cylindrical.radial ) },
	{ "VelCylindricalNormal",		INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.cylindrical.normal ) },

	{ "VelOutward",							INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.outward.speed ) },
	{ "VelOutwardOther",				INI::parseGameClientRandomVariable,	NULL,		offsetof( ParticleSystemTemplate, m_emissionVelocity.outward.otherSpeed ) },

	{ "VolumeType",							INI::parseIndexList,													EmissionVolumeTypeNames,		offsetof( ParticleSystemTemplate, m_emissionVolumeType ) },
	{ "VolLineStart",						INI::parseCoord3D,														NULL,		offsetof( ParticleSystemTemplate, m_emissionVolume.line.start ) },
	{ "VolLineEnd",							INI::parseCoord3D,														NULL,		offsetof( ParticleSystemTemplate, m_emissionVolume.line.end ) },

	{ "VolBoxHalfSize",					INI::parseCoord3D,														NULL,		offsetof( ParticleSystemTemplate, m_emissionVolume.box.halfSize ) },

	{ "VolSphereRadius",				INI::parseReal,																NULL,		offsetof( ParticleSystemTemplate, m_emissionVolume.sphere.radius ) },

	{ "VolCylinderRadius",			INI::parseReal,																NULL,		offsetof( ParticleSystemTemplate, m_emissionVolume.cylinder.radius ) },
	{ "VolCylinderLength",			INI::parseReal,																NULL,		offsetof( ParticleSystemTemplate, m_emissionVolume.cylinder.length ) },

	{ "IsHollow",								INI::parseBool,																NULL,		offsetof( ParticleSystemTemplate, m_isEmissionVolumeHollow ) },
	{ "IsGroundAligned",				INI::parseBool,																NULL,		offsetof( ParticleSystemTemplate, m_isGroundAligned ) },
	{ "IsEmitAboveGroundOnly",	INI::parseBool,																NULL,		offsetof( ParticleSystemTemplate, m_isEmitAboveGroundOnly) },
	{ "IsParticleUpTowardsEmitter",	INI::parseBool,																NULL,		offsetof( ParticleSystemTemplate, m_isParticleUpTowardsEmitter) },

	{ "WindMotion",					INI::parseIndexList, WindMotionNames, offsetof( ParticleSystemTemplate, m_windMotion ) },

	{ "WindAngleChangeMin", INI::parseReal, NULL, offsetof( ParticleSystemTemplate, m_windAngleChangeMin ) },
	{ "WindAngleChangeMax", INI::parseReal, NULL, offsetof( ParticleSystemTemplate, m_windAngleChangeMax ) },

	{ "WindPingPongStartAngleMin",			INI::parseReal, NULL, offsetof( ParticleSystemTemplate, m_windMotionStartAngleMin ) },
	{ "WindPingPongStartAngleMax",			INI::parseReal, NULL, offsetof( ParticleSystemTemplate, m_windMotionStartAngleMax ) },

	{ "WindPingPongEndAngleMin",				INI::parseReal, NULL, offsetof( ParticleSystemTemplate, m_windMotionEndAngleMin ) },
	{ "WindPingPongEndAngleMax",				INI::parseReal, NULL, offsetof( ParticleSystemTemplate, m_windMotionEndAngleMax ) },


	{ NULL,											NULL,																					NULL,		0 },
};

// ------------------------------------------------------------------------------------------------
/** Parse a "random keyframe".
 * The format is "FIELD = low high frame". */
// ------------------------------------------------------------------------------------------------
void ParticleSystemTemplate::parseRandomKeyframe( INI* ini, void *instance,
																											 void *store, const void* /*userData*/ )
{
	RandomKeyframe *key = static_cast<RandomKeyframe *>(store);

	Real low = ini->scanReal(ini->getNextToken());
	Real high = ini->scanReal(ini->getNextToken());
	key->frame = ini->scanUnsignedInt(ini->getNextToken());

	// set the range of the random variable
	key->var.setRange( low, high );
}

// ------------------------------------------------------------------------------------------------
/** Parse a "color keyframe".
 * The format is "FIELD = R:r G:g B:b frame". */
// ------------------------------------------------------------------------------------------------
void ParticleSystemTemplate::parseRGBColorKeyframe( INI* ini, void *instance,
																													void *store, const void* /*userData*/ )
{
	RGBColorKeyframe *key = static_cast<RGBColorKeyframe *>(store);

	INI::parseRGBColor( ini, instance, &key->color, NULL );
	INI::parseUnsignedInt( ini, instance, &key->frame, NULL );
}

// ------------------------------------------------------------------------------------------------
/** Parse a RandomVariable RGB color.
 * Note that the components may be negative, as this is used for rates, as well. */
// ------------------------------------------------------------------------------------------------
void ParticleSystemTemplate::parseRandomRGBColor( INI* ini, void *instance, 
																											 void *store, const void* /*userData*/ )
{
#if 0
	char seps[] = " \n\r\t=:RGB,";
	const char *token;
	Int colors[2][3];
	Int result;

	enum { LO = 0, HI = 1 };
	enum { RED = 0, GREEN = 1, BLUE = 2 };

	// initialize to invalid values
	colors[ LO ][ RED ] = -1;
	colors[ LO ][ GREEN ] = -1;
	colors[ LO ][ BLUE ] = -1;
	colors[ HI ][ RED ] = -1;
	colors[ HI ][ GREEN ] = -1;
	colors[ HI ][ BLUE ] = -1;

	// do each color part
	for( Int i = 0; i < 3; i++ )
	{
		for( Int j = 0; j < 2; j++ )
		{
			// get the color number
			token = ini->getNextToken(seps);

			// convert to number
			colors[j][i] = ini->scanInt(token);

			// check to see if it's within range
			if( colors[j][i] < -255 || colors[j][i] > 255 )
				throw INI_INVALID_DATA;

		}  // end for i
	}

	// assign the color components to the "RGBColor" pointer at 'store'
	ParticleSystemInfo::RandomRGBColor *theColor = (ParticleSystemInfo::RandomRGBColor *)store;

	theColor->red.setRange( (Real)colors[ LO ][ RED ] / 255.0f, (Real)colors[ HI ][ RED ] / 255.0f ); 
	theColor->green.setRange( (Real)colors[ LO ][ GREEN ] / 255.0f, (Real)colors[ HI ][ GREEN ] / 255.0f ); 
	theColor->blue.setRange( (Real)colors[ LO ][ BLUE ] / 255.0f, (Real)colors[ HI ][ BLUE ] / 255.0f ); 
#endif
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ParticleSystemTemplate::ParticleSystemTemplate( const AsciiString &name ) :
	m_name(name)
{ 
	//Added By Sadullah Nader
	//Initializations inserted
	m_slaveTemplate = NULL;
	//
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ParticleSystemTemplate::~ParticleSystemTemplate()
{
 
}

// ------------------------------------------------------------------------------------------------
/** If returns non-NULL, it is a slave system for use ... the create slaves parameter
 * tells *this* slave system whether or not it should create any slaves itself
 * automatically during its own constructor */
// ------------------------------------------------------------------------------------------------
ParticleSystem *ParticleSystemTemplate::createSlaveSystem( Bool createSlaves ) const
{
	if (m_slaveTemplate == NULL && m_slaveSystemName.isEmpty() == false)
		m_slaveTemplate = TheParticleSystemManager->findTemplate( m_slaveSystemName );

	ParticleSystem *slave = NULL;
	if (m_slaveTemplate)
		slave = TheParticleSystemManager->createParticleSystem( m_slaveTemplate, createSlaves );

	return slave;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ParticleSystemManager //////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ParticleSystemManager::ParticleSystemManager( void )
{

	m_uniqueSystemID = INVALID_PARTICLE_SYSTEM_ID;

	m_onScreenParticleCount = 0;
	m_localPlayerIndex = 0;
	
	//Added By Sadullah Nader
	//Initializations inserted
	m_lastLogicFrameUpdate = 0;
	m_particleCount = 0;
	m_fieldParticleCount = 0;
	m_particleSystemCount = 0;
	//

	for( Int i = 0; i < NUM_PARTICLE_PRIORITIES; ++i )
	{
		
		m_allParticlesHead[ i ] = NULL;
		m_allParticlesTail[ i ] = NULL;

	}  // end for, i

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ParticleSystemManager::~ParticleSystemManager()
{
	reset();

	TemplateMap::iterator begin(m_templateMap.begin());
	TemplateMap::iterator end(m_templateMap.end());
	for (; begin != end; ++begin) {
		(*begin).second->deleteInstance();
	}
}

// ------------------------------------------------------------------------------------------------
/** Initialize the manager */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::init( void )
{
	/// Read INI data and build templates
	INI ini;
	ini.load( AsciiString( "Data\\INI\\ParticleSystem.ini" ), INI_LOAD_OVERWRITE, NULL );

	// sanity, our lists must be empty!!
	for( Int i = 0; i < NUM_PARTICLE_PRIORITIES; ++i )
	{

		// sanity		
		DEBUG_ASSERTCRASH( m_allParticlesHead[ i ] == NULL, ("INIT: ParticleSystem all particles head[%d] is not NULL!\n", i) );
		DEBUG_ASSERTCRASH( m_allParticlesTail[ i ] == NULL, ("INIT: ParticleSystem all particles tail[%d] is not NULL!\n", i) );

		// just to be clean set them to NULL
		m_allParticlesHead[ i ] = NULL;
		m_allParticlesTail[ i ] = NULL;

	}  // end for, i

}

// ------------------------------------------------------------------------------------------------
/** Reset the manager and all particle systems */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::reset( void )
{
	while (getParticleSystemCount()) {
		if (m_allParticleSystemList.front()) {
			m_allParticleSystemList.front()->deleteInstance();
		}
	}

	// sanity, our lists must be empty!!
	for( Int i = 0; i < NUM_PARTICLE_PRIORITIES; ++i )
	{

		// sanity		
		DEBUG_ASSERTCRASH( m_allParticlesHead[ i ] == NULL, ("RESET: ParticleSystem all particles head[%d] is not NULL!\n", i) );
		DEBUG_ASSERTCRASH( m_allParticlesTail[ i ] == NULL, ("RESET: ParticleSystem all particles tail[%d] is not NULL!\n", i) );

		// just to be clean set them to NULL
		m_allParticlesHead[ i ] = NULL;
		m_allParticlesTail[ i ] = NULL;

	}  // end for, i

	m_particleCount = 0;
	m_fieldParticleCount = 0;
	m_particleSystemCount = 0;

	m_uniqueSystemID = INVALID_PARTICLE_SYSTEM_ID;
	
	m_lastLogicFrameUpdate = -1;
	// leave templates as-is
}

// ------------------------------------------------------------------------------------------------
/** Update all particle systems */
// ------------------------------------------------------------------------------------------------
//DECLARE_PERF_TIMER(ParticleSystemManager)
void ParticleSystemManager::update( void )
{
	if (m_lastLogicFrameUpdate == TheGameLogic->getFrame()) {
		return;
	}

	// update the last logic frame.
	m_lastLogicFrameUpdate = TheGameLogic->getFrame();

	//USE_PERF_TIMER(ParticleSystemManager)
	ParticleSystem *sys;

	for(ParticleSystemListIt it = m_allParticleSystemList.begin(); it != m_allParticleSystemList.end();) 
	{
		sys = (*it);
		if (!sys) {
			continue;
		}

		if (sys->update(m_localPlayerIndex) == false)
		{
			++it;
			sys->deleteInstance();
		} else {
			++it;
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** sets the count of the particles on screen after each frame */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::setOnScreenParticleCount(int count)
{
	m_onScreenParticleCount = count;
}

// ------------------------------------------------------------------------------------------------
/** Given a file containing particle system properties, create a new instance of it */
// ------------------------------------------------------------------------------------------------
ParticleSystem *ParticleSystemManager::createParticleSystem( const ParticleSystemTemplate *sysTemplate, Bool createSlaves )
{
	// sanity
	if (sysTemplate == NULL)
		return NULL;

	m_uniqueSystemID = (ParticleSystemID)((UnsignedInt)m_uniqueSystemID + 1);
	ParticleSystem *sys = newInstance(ParticleSystem)( sysTemplate, m_uniqueSystemID, createSlaves );
	return sys;
}

// ------------------------------------------------------------------------------------------------
/// given a template, instantiate a particle system attached to the given object, and return its ID
// ------------------------------------------------------------------------------------------------
ParticleSystemID ParticleSystemManager::createAttachedParticleSystemID( 
																			const ParticleSystemTemplate *sysTemplate,
																			Object* attachTo,
																			Bool createSlaves )
{
	ParticleSystem* pSystem = TheParticleSystemManager->createParticleSystem(sysTemplate, createSlaves);
	if (pSystem && attachTo)
		pSystem->attachToObject(attachTo);
	return pSystem ? pSystem->getSystemID() : INVALID_PARTICLE_SYSTEM_ID;
}

// ------------------------------------------------------------------------------------------------
/** Find a particle system with the matching system id  */
// ------------------------------------------------------------------------------------------------
ParticleSystem *ParticleSystemManager::findParticleSystem( ParticleSystemID id )
{
	if (id == INVALID_PARTICLE_SYSTEM_ID)
		return NULL;	// my, that was easy

	ParticleSystem *system = NULL;

	for( ParticleSystemListIt it = m_allParticleSystemList.begin(); it != m_allParticleSystemList.end(); ++it ) {
		system = *it;
		if (!system) {
			continue;
		}

		if( system->getSystemID() == id ) {
			return system;
		}
	}

	return NULL;

}  // end findParticleSystem

// ------------------------------------------------------------------------------------------------
/** destroy the particle system with the given id (if it still exists) */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::destroyParticleSystemByID(ParticleSystemID id)
{
	ParticleSystem* pSystem = findParticleSystem(id);
	if( pSystem )
		pSystem->destroy();
}

// ------------------------------------------------------------------------------------------------
/** Locate an existing ParticleSystemTemplate */
// ------------------------------------------------------------------------------------------------
ParticleSystemTemplate *ParticleSystemManager::findTemplate( const AsciiString &name ) const
{
	ParticleSystemTemplate *sysTemplate = NULL;

	TemplateMap::const_iterator find(m_templateMap.find(name));
	if (find != m_templateMap.end()) {
		sysTemplate = (*find).second;
	}

	return sysTemplate;
}

// ------------------------------------------------------------------------------------------------
/** Create a new ParticleSystemTemplate */
// ------------------------------------------------------------------------------------------------
ParticleSystemTemplate *ParticleSystemManager::newTemplate( const AsciiString &name )
{
	ParticleSystemTemplate *sysTemplate = findTemplate(name);
	if (sysTemplate == NULL) {
		sysTemplate = newInstance(ParticleSystemTemplate)( name );

		if (! m_templateMap.insert(std::make_pair(name, sysTemplate)).second) {
			sysTemplate->deleteInstance();
			sysTemplate = NULL;
		}
	}

	return sysTemplate;
}

// ------------------------------------------------------------------------------------------------
/** Find a particle system's parent. Should really only be called by TheScriptEngine */
// ------------------------------------------------------------------------------------------------
ParticleSystemTemplate *ParticleSystemManager::findParentTemplate( const AsciiString &name, Int parentNum ) const
{
	if (name.isEmpty()) {
		return NULL;
	}

	TemplateMap::const_iterator begin(m_templateMap.begin());
	TemplateMap::const_iterator end(m_templateMap.end());
	for(; begin != end; ++begin) {
		ParticleSystemTemplate *sysTemplate = (*begin).second;
		if (name.compare(sysTemplate->m_slaveSystemName) == 0) {
			if (! parentNum--) {
				return sysTemplate;
			}
		}
	}

	return NULL;	
}

// ------------------------------------------------------------------------------------------------
/** Destroy any particle systems that are attached to this object */	
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::destroyAttachedSystems( Object *obj )
{

	// sanity
	if( obj == NULL )
		return;

	// iterate through all systems
	ParticleSystem *system = NULL;

	for( ParticleSystemListIt it = m_allParticleSystemList.begin(); 
			 it != m_allParticleSystemList.end(); 
			 ++it ) 
	{

		system = *it;
		if( system == NULL )
			continue;
		
		if( system->getAttachedObject() == obj->getID() )
			system->destroy();

	}

}

// ------------------------------------------------------------------------------------------------
/** Add a particle to the global particle list. */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::addParticle( Particle *particleToAdd, ParticlePriorityType priority )
{
	if (particleToAdd->m_inOverallList)
		return;

	if (!m_allParticlesHead[ priority ])
	{
		m_allParticlesHead[ priority ] = particleToAdd;
	}

	if (m_allParticlesTail[ priority ])
	{
		m_allParticlesTail[ priority ]->m_overallNext = particleToAdd;
		particleToAdd->m_overallPrev = m_allParticlesTail[ priority ];
	}
	else
	{
		particleToAdd->m_overallPrev = NULL;
	}

	m_allParticlesTail[ priority ] = particleToAdd;
	particleToAdd->m_overallNext = NULL;
	particleToAdd->m_inOverallList = TRUE;

	++m_particleCount;
	

}

// ------------------------------------------------------------------------------------------------
/** Remove a particle from the global particle list. */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::removeParticle( Particle *particleToRemove)
{
	if (!particleToRemove->m_inOverallList)
		return;

	// get priority of the particle we're removing
	ParticlePriorityType priority = particleToRemove->getPriority();

	// remove links from prev & next objs
	if (particleToRemove->m_overallNext)
		particleToRemove->m_overallNext->m_overallPrev = particleToRemove->m_overallPrev;
	if (particleToRemove->m_overallPrev)
		particleToRemove->m_overallPrev->m_overallNext = particleToRemove->m_overallNext;

	// update head & tail if neccessary
	if (particleToRemove == m_allParticlesHead[ priority ])
		m_allParticlesHead[ priority ] = particleToRemove->m_overallNext;
	if (particleToRemove == m_allParticlesTail[ priority ])
		m_allParticlesTail[ priority ] = particleToRemove->m_overallPrev;

	particleToRemove->m_overallNext = particleToRemove->m_overallPrev = NULL;
	particleToRemove->m_inOverallList = FALSE;
	--m_particleCount;


}

// ------------------------------------------------------------------------------------------------
/** Add a particle system to the master particle system list. */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::friend_addParticleSystem( ParticleSystem *particleSystemToAdd )
{
	m_allParticleSystemList.push_back(particleSystemToAdd);
	++m_particleSystemCount;
}

// ------------------------------------------------------------------------------------------------
/** Remove a particle system from the master particle system list. */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::friend_removeParticleSystem( ParticleSystem *particleSystemToRemove )
{
	ParticleSystemListIt it = std::find(m_allParticleSystemList.begin(), m_allParticleSystemList.end(), particleSystemToRemove);
	if (it != m_allParticleSystemList.end()) {
		m_allParticleSystemList.erase(it);
		--m_particleSystemCount;
	}

}

// ------------------------------------------------------------------------------------------------
/** Remove the oldest N number of particles from the lowest priority lists first.  We will
 * not remove particles from any priorities higher or equal to the priorityCap parameter. */
// ------------------------------------------------------------------------------------------------
Int ParticleSystemManager::removeOldestParticles( UnsignedInt count, 
																									ParticlePriorityType priorityCap )
{
	Int countToRemove = count;

	while (count-- && getParticleCount()) 
	{
		for( Int i = PARTICLE_PRIORITY_LOWEST;
				 i < priorityCap;
				 ++i )
		{
			if( m_allParticlesHead[ i ] ) 
			{
				m_allParticlesHead[ i ]->deleteInstance();
				break;  // exit for
			}
		}
	}

	// return the number of particles actually removed
	return countToRemove - count;

}

// ------------------------------------------------------------------------------------------------
/** Preload particle system textures */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::preloadAssets( TimeOfDay timeOfDay )
{
	TemplateMap::iterator begin(m_templateMap.begin());
	TemplateMap::iterator end(m_templateMap.end());

	for (; begin != end; ++begin) {
		const ParticleSystemTemplate *tmplate = (*begin).second;
		if (tmplate->m_particleType == ParticleSystemInfo::PARTICLE &&
			 	(! tmplate->m_particleTypeName.isEmpty()))
		{
			TheDisplay->preloadTextureAssets(tmplate->m_particleTypeName);
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::crc( Xfer *xfer )
{

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// unique system ID counter
	xfer->xferUser( &m_uniqueSystemID, sizeof( ParticleSystemID ) );

	// count of particle systems in the world
	UnsignedInt systemCount = m_particleSystemCount;
	xfer->xferUnsignedInt( &systemCount );

	// particle systems data
	AsciiString systemName;
	ParticleSystem *system;
	if( xfer->getXferMode() == XFER_SAVE )
	{

		// iterate each particle system
		ParticleSystemListIt it;
		for( it = m_allParticleSystemList.begin(); it != m_allParticleSystemList.end(); ++it )
		{
			systemCount--;
			// get system
			system = *it;

			// ignore destroyed systems and non-saveable systems
			if( system->isDestroyed() == TRUE || system->isSaveable() == FALSE )	{
				AsciiString mtString = "";
				xfer->xferAsciiString(&mtString); // write null string as key for destroyed system.
				continue;
			}

			// write template name
			systemName = system->getTemplate()->getName();
			xfer->xferAsciiString( &systemName );

			// write system data
			xfer->xferSnapshot( system );

		}  // end for, it
		DEBUG_ASSERTCRASH(systemCount==0, ("Mismatch in write count."));

	}  // end if, save
	else
	{
		const ParticleSystemTemplate *systemTemplate;

		// read each particle system
		for( UnsignedInt i = 0; i < systemCount; ++i )
		{

			// read system name and find template
			xfer->xferAsciiString( &systemName );
			if (systemName.isEmpty()) {
				continue; // destroyed particle system.
			}
			systemTemplate = findTemplate( systemName );

			// sanity
			if( systemTemplate == NULL )
			{

				DEBUG_CRASH(( "ParticleSystemManager::xfer - Unknown particle system template '%s'\n",
											systemName.str() ));
				throw SC_INVALID_DATA;

			}  // end if

			// create system
			system = createParticleSystem( systemTemplate, FALSE );

			if( system == NULL )
			{

				DEBUG_CRASH(( "ParticleSystemManager::xfer - Unable to allocate particle system '%s'\n",
											systemName.str() ));
				throw SC_INVALID_DATA;

			}  // end if

			// read system data
			xfer->xferSnapshot( system );

		}  // end for, i

	}  // end else, load

}  // end particleSystemManager

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ParticleSystemManager::loadPostProcess( void )
{

}  // end loadPostProcess

// ------------------------------------------------------------------------------------------------
/** Output particle system statistics to the screen
 * @todo Implement a real console (MSB) */
// ------------------------------------------------------------------------------------------------
void ParticleSystemDebugDisplay( DebugDisplayInterface *dd, void *, FILE *fp )
{
	if (!dd)
		return;

	dd->setCursorPos( 0, 0 );
	dd->setRightMargin( 2 );
	
	dd->printf( "Total Particles: %d\n", TheParticleSystemManager->getParticleCount() );
	dd->printf( "Total Particles (On Screen): %d\n", TheParticleSystemManager->getOnScreenParticleCount());
	dd->printf( "Total Particle Systems: %d\n", TheParticleSystemManager->getParticleSystemCount() );

	ParticleSystemManager::ParticleSystemList list = TheParticleSystemManager->getAllParticleSystems();
	ParticleSystemManager::ParticleSystemList::iterator it;

	std::map<AsciiString, Int> templateMap;
	std::map<AsciiString, Int> templateMapParticleCount;

	std::map<AsciiString, Int>::iterator templateMapIt;
	std::map<AsciiString, Int>::iterator templateMapParticleCountIt;

	for ( it = list.begin(); it != list.end(); ++it )
	{
		AsciiString templateName = (*it)->getTemplate()->getName();
		templateMapIt = templateMap.find(templateName);
		if (templateMapIt == templateMap.end())
		{
			templateMap.insert(std::make_pair(templateName, 1));
			templateMapParticleCount.insert(std::make_pair(templateName, (*it)->getParticleCount()));
		}
		else
		{
			++templateMapIt->second;

			templateMapParticleCountIt = templateMapParticleCount.find(templateName);
			if (templateMapParticleCountIt != templateMapParticleCount.end())
				templateMapParticleCountIt->second += (*it)->getParticleCount();
		}
	}

	for (templateMapIt = templateMap.begin(); templateMapIt != templateMap.end(); ++templateMapIt)
	{
		templateMapParticleCountIt = templateMapParticleCount.find(templateMapIt->first);

		dd->printf("  %s: %d instances", templateMapIt->first.str(), templateMapIt->second);
		if (templateMapParticleCountIt != templateMapParticleCount.end())
			dd->printf("  (Avg per system %.2f)", INT_TO_REAL(templateMapParticleCountIt->second) / templateMapIt->second);
		dd->printf("\n");
	}
}

// ------------------------------------------------------------------------------------------------
