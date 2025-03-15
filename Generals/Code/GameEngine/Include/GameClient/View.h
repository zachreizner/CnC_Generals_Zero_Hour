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

// View.h /////////////////////////////////////////////////////////////////////////////////////////
// A "view", or window, into the World
// Author: Michael S. Booth, February 2001
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _VIEW_H_
#define _VIEW_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/GameType.h"
#include "Common/Snapshot.h"
#include "Lib/BaseType.h"
#include "WW3D2/coltype.h"			///< we don't generally do this, but we need the W3D collision types

#define DEFAULT_VIEW_WIDTH 640
#define DEFAULT_VIEW_HEIGHT 480
#define DEFAULT_VIEW_ORIGIN_X 0
#define DEFAULT_VIEW_ORIGIN_Y 0

// FORWARD DECLARATIONS ///////////////////////////////////////////////////////////////////////////
class Drawable;
class ViewLocation;
class Thing;
class Waypoint;
class LookAtTranslator;
enum FilterModes : int32_t;
enum FilterTypes : int32_t;

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
enum PickType
{
	PICK_TYPE_TERRAIN						= COLLISION_TYPE_0,
	PICK_TYPE_SELECTABLE				= COLLISION_TYPE_1,
	PICK_TYPE_SHRUBBERY					= COLLISION_TYPE_2,
	PICK_TYPE_MINES							= COLLISION_TYPE_3,	// mines aren't normally selectable, but workers/dozers need to
	PICK_TYPE_FORCEATTACKABLE		= COLLISION_TYPE_4,
	PICK_TYPE_ALL_DRAWABLES			= (PICK_TYPE_SELECTABLE | PICK_TYPE_SHRUBBERY | PICK_TYPE_MINES | PICK_TYPE_FORCEATTACKABLE)
};

// ------------------------------------------------------------------------------------------------
/** The implementation of common view functionality. */
// ------------------------------------------------------------------------------------------------
class View : public Snapshot
{

public:

	/// Add an impulse force to shake the camera
	enum CameraShakeType 
	{ 
		SHAKE_SUBTLE = 0, 
		SHAKE_NORMAL, 
		SHAKE_STRONG, 
		SHAKE_SEVERE, 
		SHAKE_CINE_EXTREME,		//Added for cinematics ONLY
		SHAKE_CINE_INSANE,		//Added for cinematics ONLY
		SHAKE_COUNT 
	};

public:

	View( void );
	virtual ~View( void );

	virtual void init( void );
	virtual void reset( void );
	virtual UnsignedInt getID( void ) { return m_id; }

	virtual void setZoomLimited( Bool limit ) { m_zoomLimited = limit; }			///< limit the zoom height
	virtual Bool isZoomLimited( void ) { return m_zoomLimited; }							///< get status of zoom limit

	/// pick drawable given the screen pixel coords.  If force attack, picks bridges as well.
	virtual Drawable *pickDrawable( const ICoord2D *screen, Bool forceAttack, PickType pickType ) = 0;

	/// all drawables in the 2D screen region will call the 'callback'
	virtual Int iterateDrawablesInRegion( IRegion2D *screenRegion,
																				Bool (*callback)( Drawable *draw, void *userData ),
																				void *userData ) = 0;

	/** project the 4 corners of this view into the world and return each point as a parameter,
			the world points are at the requested Z */
	virtual void getScreenCornerWorldPointsAtZ( Coord3D *topLeft, Coord3D *topRight,
																							Coord3D *bottomLeft, Coord3D *bottomRight,
																							Real z );

	virtual void setWidth( Int width ) { m_width = width; }
	virtual Int getWidth( void ) { return m_width; }
	virtual void setHeight( Int height ) { m_height = height; }
	virtual Int getHeight( void ) { return m_height; }
	virtual void setOrigin( Int x, Int y) { m_originX=x; m_originY=y;}				///< Sets location of top-left view corner on display 
	virtual void getOrigin( Int *x, Int *y) { *x=m_originX; *y=m_originY;}			///< Return location of top-left view corner on display

	virtual void forceRedraw() = 0;

	virtual void lookAt( const Coord3D *o );														///< Center the view on the given coordinate
	virtual void initHeightForMap( void ) {};														///<  Init the camera height for the map at the current position.
	virtual void scrollBy( Coord2D *delta );														///< Shift the view by the given delta

	virtual void moveCameraTo(const Coord3D *o, Int frames, Int shutter, Bool orient) { lookAt( o ); }
	virtual void moveCameraAlongWaypointPath(Waypoint *way, Int frames, Int shutter, Bool orient) { }
	virtual Bool isCameraMovementFinished( void ) { return TRUE; }
	virtual void cameraModFinalZoom(Real finalZoom){};			 ///< Final zoom for current camera movement.
	virtual void cameraModRollingAverage(Int framesToAverage){}; ///< Number of frames to average movement for current camera movement.
	virtual void cameraModFinalTimeMultiplier(Int finalMultiplier){}; ///< Final time multiplier for current camera movement.
	virtual void cameraModFinalPitch(Real finalPitch){};		 ///< Final pitch for current camera movement.
	virtual void cameraModFreezeTime(void){ }					///< Freezes time during the next camera movement.
	virtual void cameraModFreezeAngle(void){ }					///< Freezes time during the next camera movement.
	virtual void cameraModLookToward(Coord3D *pLoc){}			///< Sets a look at point during camera movement.
	virtual void cameraModFinalLookToward(Coord3D *pLoc){}			///< Sets a look at point during camera movement.
	virtual void cameraModFinalMoveTo(Coord3D *pLoc){ };			///< Sets a final move to.

	virtual enum FilterModes getViewFilterMode(void) {return (enum FilterModes)0;}			///< Turns on viewport special effect (black & white mode)
	virtual enum FilterTypes getViewFilterType(void) {return (enum FilterTypes)0;}			///< Turns on viewport special effect (black & white mode)
	virtual Bool setViewFilterMode(enum FilterModes filterMode) { return FALSE; }			///< Turns on viewport special effect (black & white mode)
	virtual void setViewFilterPos(const Coord3D *pos) { };			///<  Passes a position to the special effect filter.
	virtual Bool setViewFilter(	enum FilterTypes filter) { return FALSE;}			///< Turns on viewport special effect (black & white mode)

	virtual void setFadeParameters(Int fadeFrames, Int direction) { };
	virtual void set3DWireFrameMode(Bool enable) { };

 	virtual void resetCamera(const Coord3D *location, Int frames) {}; ///< Move camera to location, and reset to default angle & zoom.
 	virtual void rotateCamera(Real rotations, Int frames) {}; ///< Rotate camera about current viewpoint.
	virtual void rotateCameraTowardObject(ObjectID id, Int milliseconds, Int holdMilliseconds) {};	///< Rotate camera to face an object, and hold on it
	virtual void rotateCameraTowardPosition(const Coord3D *pLoc, Int milliseconds) {};	///< Rotate camera to face a location.
	virtual Bool isTimeFrozen(void){ return false;}					///< Freezes time during the next camera movement.
	virtual Int	 getTimeMultiplier(void) {return 1;};				///< Get the time multiplier.
	virtual void setTimeMultiplier(Int multiple) {}; ///< Set the time multiplier.
	virtual void setDefaultView(Real pitch, Real angle, Real maxHeight) {};
	virtual void zoomCamera( Real finalZoom, Int milliseconds ) {};
	virtual void pitchCamera( Real finalPitch, Int milliseconds ) {};

	virtual void setAngle( Real angle );																///< Rotate the view around the up axis to the given angle
	virtual Real getAngle( void ) { return m_angle; }
	virtual void setPitch( Real angle );																///< Rotate the view around the horizontal axis to the given angle
	virtual Real getPitch( void ) { return m_pitchAngle; }							///< Return current camera pitch
	virtual void setAngleAndPitchToDefault( void );											///< Set the view angle back to default 
	virtual void getPosition(Coord3D *pos)	{ *pos=m_pos;}							///< Returns position camera is looking at (z will be zero)

	virtual const Coord3D& get3DCameraPosition() const = 0;							///< Returns the actual camera position

	virtual Real getZoom() { return m_zoom; }
	virtual void setZoom(Real z) { }
	virtual Real getHeightAboveGround() { return m_heightAboveGround; }
	virtual void setHeightAboveGround(Real z) { m_heightAboveGround = z; }
	virtual void zoomIn( void );																				///< Zoom in, closer to the ground, limit to min
	virtual void zoomOut( void );																				///< Zoom out, farther away from the ground, limit to max
	virtual void setZoomToDefault( void ) { }														///< Set zoom to default value
	virtual Real getMaxZoom( void ) { return m_maxZoom; }								///< return max zoom value
	virtual void setOkToAdjustHeight( Bool val ) { m_okToAdjustHeight = val; }	///< Set this to adjust camera height

	// for debugging
	virtual Real getTerrainHeightUnderCamera() { return m_terrainHeightUnderCamera; }
	virtual void setTerrainHeightUnderCamera(Real z) { m_terrainHeightUnderCamera = z; }
	virtual Real getCurrentHeightAboveGround() { return m_currentHeightAboveGround; }
	virtual void setCurrentHeightAboveGround(Real z) { m_currentHeightAboveGround = z; }

	virtual void setFieldOfView( Real angle ) { m_FOV = angle; }				///< Set the horizontal field of view angle
	virtual Real getFieldOfView( void ) { return m_FOV; }								///< Get the horizontal field of view angle

	virtual Bool worldToScreen( const Coord3D *w, ICoord2D *s ) = 0;										///< Transform world coordinate "w" into screen coordinate "s"
	virtual void screenToWorld( const ICoord2D *s, Coord3D *w ) = 0;										///< Transform screen coordinate "s" into world coordinate "w"
	virtual void screenToTerrain( const ICoord2D *screen, Coord3D *world ) = 0;  ///< transform screen coord to a point on the 3D terrain
	virtual void screenToWorldAtZ( const ICoord2D *s, Coord3D *w, Real z ) = 0;  ///< transform screen point to world point at the specified world Z value

	virtual void getLocation ( ViewLocation *location );								///< write the view's current location in to the view location object
	virtual void setLocation ( const ViewLocation *location );					///< set the view's current location from to the view location object


	virtual void drawView( void ) = 0;															///< Render the world visible in this view.
	virtual void updateView(void) = 0;					///<called once per frame to determine the final camera and object transforms




	virtual ObjectID getCameraLock() const { return m_cameraLock; }
	virtual void setCameraLock(ObjectID id) { m_cameraLock = id; m_lockDist = 0.0f; m_lockType = LOCK_FOLLOW; }
	virtual void snapToCameraLock( void ) { m_snapImmediate = TRUE; }
	enum CameraLockType { LOCK_FOLLOW, LOCK_TETHER };
	virtual void setSnapMode( CameraLockType lockType, Real lockDist ) { m_lockType = lockType; m_lockDist = lockDist; }

	virtual Drawable *getCameraLockDrawable() const { return m_cameraLockDrawable; }
	virtual void setCameraLockDrawable(Drawable *drawable) { m_cameraLockDrawable = drawable; m_lockDist = 0.0f; }

	virtual void setMouseLock( Bool mouseLocked ) { m_mouseLocked = mouseLocked; }					///< lock/unlock the mouse input to the tactical view
	virtual Bool isMouseLocked( void ) { return m_mouseLocked; }														///< is the mouse input locked to the tactical view?

	/// Add an impulse force to shake the camera
	virtual void shake( const Coord3D *epicenter, CameraShakeType shakeType ) { };

	virtual Real getFXPitch( void ) const { return 1.0f; }					///< returns the FX pitch angle
	virtual void forceCameraConstraintRecalc(void) {}
	virtual void setGuardBandBias( const Coord2D *gb ) = 0;

protected:

	friend class Display;

	// snapshot methods
	virtual void crc( Xfer *xfer ) { }
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess( void ) { }

	void setPosition( const Coord3D *pos ) { m_pos = *pos; }					
	const Coord3D *getPosition( void ) const { return &m_pos; }					

	virtual View *prependViewToList( View *list );							///< Prepend this view to the given list, return the new list
	virtual View *getNextView( void ) { return m_next; }				///< Return next view in the set


	// **********************************************************************************************

	View *m_next;																								///< List links used by the Display class

	UnsignedInt m_id;																						///< Rhe ID of this view
	static UnsignedInt m_idNext;																///< Used for allocating view ID's for all views

	Coord3D m_pos;																							///< Position of this view, in world coordinates
	Int m_width, m_height;																			///< Dimensions of the view
	Int m_originX, m_originY;																		///< Location of top/left view corner

	Real m_angle;																								///< Angle at which view has been rotated about the Z axis
	Real m_pitchAngle;																					///< Rotation of view direction around horizontal (X) axis

	Real m_maxZoom;																							///< Largest zoom value (minimum actual zoom)
	Real m_minZoom;																							///< Smallest zoom value (maximum actual zoom)
	Real m_maxHeightAboveGround;
	Real m_minHeightAboveGround;
	Real m_zoom;																								///< Current zoom value
	Real m_heightAboveGround;																		///< User's desired height above ground
	Bool m_zoomLimited;																					///< Camera restricted in zoom height
	Real m_defaultAngle;
	Real m_defaultPitchAngle;
	Real m_currentHeightAboveGround;														///< Cached value for debugging
	Real m_terrainHeightUnderCamera;														///< Cached value for debugging

	ObjectID m_cameraLock;																			///< if nonzero, id of object that the camera should follow
	Drawable *m_cameraLockDrawable;															///< if nonzero, drawble of object that camera should follow.
	CameraLockType m_lockType;																	///< are we following or just tethering?
	Real m_lockDist;																						///< how far can we be when tethered?

	Real m_FOV;																									///< the current field of view angle
	Bool m_mouseLocked;																					///< is the mouse input locked to the tactical view?

	Bool m_okToAdjustHeight;																		///< Should we attempt to adjust camera height?
	Bool m_snapImmediate;																				///< Should we immediately snap to the object we're following?

	Coord2D m_guardBandBias; ///< Exttra beefy margins so huge thins can stay "on-screen"

};

// ------------------------------------------------------------------------------------------------
/** Used to save and restore view position */
// ------------------------------------------------------------------------------------------------
class ViewLocation
{
	friend class View;
	friend class LookAtTranslator;

	protected:
		Bool m_valid;																								///< Is this location valid
		Coord3D m_pos;																							///< Position of this view, in world coordinates
		Real m_angle;																								///< Angle at which view has been rotated about the Z axis
		Real m_pitch;																								///< Angle at which view has been rotated about the Y axis
		Real m_zoom;																								///< Current zoom value

	public:

		ViewLocation() 
		{ 
			m_valid = FALSE;
			//Added By Sadullah Nader
			//Initialization(s) inserted
			m_pos.zero();
			m_angle = m_pitch = m_zoom = 0.0;
			//
		}
		
		const Coord3D& getPosition() const { return m_pos; }
		Bool isValid() const { return m_valid; }
		Real getAngle() const { return m_angle; }
		Real getPitch() const { return m_pitch; }
		Real getZoom() const { return m_zoom; }

		void init(Real x, Real y, Real z, Real angle, Real pitch, Real zoom)
		{
			m_pos.x = x;
			m_pos.y = y;
			m_pos.z = z;
			m_angle = angle;
			m_pitch = pitch;
			m_zoom = zoom;
			m_valid = true;
		}
};

// EXTERNALS //////////////////////////////////////////////////////////////////////////////////////
extern View *TheTacticalView;		///< the main tactical interface to the game world

#endif // _VIEW_H_
