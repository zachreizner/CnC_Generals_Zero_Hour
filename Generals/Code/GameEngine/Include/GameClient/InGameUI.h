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

// FILE: InGameUI.h ///////////////////////////////////////////////////////////////////////////////
// Defines the in-game user interface singleton
// Author: Michael S. Booth, March 2001
//				 Colin Day August 2001, or so
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _IN_GAME_UI_H_
#define _IN_GAME_UI_H_

#include "Common/GameCommon.h"
#include "Common/GameType.h"
#include "Common/MessageStream.h"		// for GameMessageTranslator
#include "Common/SpecialPowerType.h"
#include "Common/STLTypedefs.h"
#include "Common/SubsystemInterface.h"
#include "Common/UnicodeString.h"
#include "GameClient/DisplayString.h"
#include "GameClient/Mouse.h"
#include "GameClient/RadiusDecal.h"
#include "GameClient/View.h"
#include "Common/Snapshot.h"

// FORWARD DECLARATIONS ///////////////////////////////////////////////////////////////////////////
class Drawable;
class Object;
class ThingTemplate;
class GameWindow;
class VideoBuffer;
class VideoStreamInterface;
class CommandButton;
class SpecialPowerTemplate;
class WindowLayout;
class Anim2DTemplate;
class Anim2D;
class Shadow;
enum LegalBuildCode : int32_t;
enum KindOfType : int32_t;
enum ShadowType : int32_t;
enum CanAttackResult : int32_t;

// ------------------------------------------------------------------------------------------------
enum RadiusCursorType
{
	RADIUSCURSOR_NONE = 0,
	RADIUSCURSOR_ATTACK_DAMAGE_AREA,
	RADIUSCURSOR_ATTACK_SCATTER_AREA,
	RADIUSCURSOR_ATTACK_CONTINUE_AREA,
	RADIUSCURSOR_GUARD_AREA,
	RADIUSCURSOR_EMERGENCY_REPAIR,
	RADIUSCURSOR_FRIENDLY_SPECIALPOWER,
	RADIUSCURSOR_OFFENSIVE_SPECIALPOWER,
	RADIUSCURSOR_SUPERWEAPON_SCATTER_AREA,
	
	RADIUSCURSOR_PARTICLECANNON, 
	RADIUSCURSOR_A10STRIKE,
	RADIUSCURSOR_CARPETBOMB,
	RADIUSCURSOR_DAISYCUTTER,
	RADIUSCURSOR_PARADROP,
	RADIUSCURSOR_SPYSATELLITE, 

	RADIUSCURSOR_NUCLEARMISSILE, 
	RADIUSCURSOR_EMPPULSE,
	RADIUSCURSOR_ARTILLERYBARRAGE,
	RADIUSCURSOR_NAPALMSTRIKE,
	RADIUSCURSOR_CLUSTERMINES,

	RADIUSCURSOR_SCUDSTORM, 
	RADIUSCURSOR_ANTHRAXBOMB,
	RADIUSCURSOR_AMBUSH, 
	RADIUSCURSOR_RADAR,
	RADIUSCURSOR_SPYDRONE,


	RADIUSCURSOR_COUNT	// keep last
};

#ifdef DEFINE_RADIUSCURSOR_NAMES
static const char *TheRadiusCursorNames[] = 
{
	"NONE",
	"ATTACK_DAMAGE_AREA",
	"ATTACK_SCATTER_AREA",
	"ATTACK_CONTINUE_AREA",
	"GUARD_AREA",
	"EMERGENCY_REPAIR",
	"FRIENDLY_SPECIALPOWER",	//green
	"OFFENSIVE_SPECIALPOWER", //red
	"SUPERWEAPON_SCATTER_AREA",//red

	"PARTICLECANNON", 
	"A10STRIKE",
	"CARPETBOMB",
	"DAISYCUTTER",
	"PARADROP",
	"SPYSATELLITE", 

	"NUCLEARMISSILE", 
	"EMPPULSE",
	"ARTILLERYBARRAGE",
	"NAPALMSTRIKE",
	"CLUSTERMINES",

	"SCUDSTORM", 
	"ANTHRAXBOMB",
	"AMBUSH", 
	"RADAR",
	"SPYDRONE",

	NULL
};
#endif

// ------------------------------------------------------------------------------------------------
/** For keeping track in the UI of how much build progress has been done */
// ------------------------------------------------------------------------------------------------
enum { MAX_BUILD_PROGRESS = 64 };  ///< interface can support building this many different units
struct BuildProgress
{
	const ThingTemplate *m_thingTemplate;
	Real m_percentComplete;
	GameWindow *m_control;
};

// TYPE DEFINES ///////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
typedef std::list<Drawable *> DrawableList;
typedef std::list<Drawable *>::iterator DrawableListIt;
typedef std::list<Drawable *>::const_iterator DrawableListCIt;

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class SuperweaponInfo : public MemoryPoolObject
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(SuperweaponInfo, "SuperweaponInfo")		

private:
// not saved
	DisplayString *             m_nameDisplayString;						///< display string used to render the message
	DisplayString *             m_timeDisplayString;						///< display string used to render the message
	Color												m_color;
	const SpecialPowerTemplate*	m_powerTemplate;

public:

	SuperweaponInfo(
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
	);

	const SpecialPowerTemplate*	getSpecialPowerTemplate() const { return m_powerTemplate; }
	void setFont(const AsciiString& superweaponNormalFont, Int superweaponNormalPointSize, Bool superweaponNormalBold);
	void setText(const UnicodeString& name, const UnicodeString& time);
	void drawName(Int x, Int y, Color color, Color dropColor);
	void drawTime(Int x, Int y, Color color, Color dropColor);
	Real getHeight() const;

// saved & public
	AsciiString									m_powerName;
	ObjectID										m_id;
	UnsignedInt									m_timestamp;									  ///< seconds shown in display string
	Bool												m_hiddenByScript;
	Bool												m_hiddenByScience;
 	Bool												m_ready;											///< Stores if we were ready last draw, since readyness can change without time changing
// not saved, but public
 	Bool												m_forceUpdateText;

};

// ------------------------------------------------------------------------------------------------
typedef std::list<SuperweaponInfo *> SuperweaponList;
typedef std::map<AsciiString, SuperweaponList> SuperweaponMap;

// ------------------------------------------------------------------------------------------------
// Popup message box
class PopupMessageData : public MemoryPoolObject
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(PopupMessageData, "PopupMessageData")		
public:
	UnicodeString		message;
	Int							x;
	Int							y;
	Int							width;
	Color						textColor;
	Bool						pause;
	Bool						pauseMusic;
	WindowLayout*	layout;
};
EMPTY_DTOR(PopupMessageData)

// ------------------------------------------------------------------------------------------------
class NamedTimerInfo : public MemoryPoolObject
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(NamedTimerInfo, "NamedTimerInfo")		
public:
	AsciiString			m_timerName;							///< Timer name, needed on Load to reconstruct Map.
	UnicodeString		timerText;								///< timer text
	DisplayString*	displayString;						///< display string used to render the message
	UnsignedInt			timestamp;									///< seconds shown in display string
	Color						color;
	Bool						isCountdown;
};
EMPTY_DTOR(NamedTimerInfo)

// ------------------------------------------------------------------------------------------------
typedef std::map<AsciiString, NamedTimerInfo *> NamedTimerMap;
typedef NamedTimerMap::iterator NamedTimerMapIt;

// ------------------------------------------------------------------------------------------------
enum {MAX_SUBTITLE_LINES = 4};							///< The maximum number of lines a subtitle can have

// ------------------------------------------------------------------------------------------------
// Floating Text Data
class FloatingTextData : public MemoryPoolObject
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(FloatingTextData, "FloatingTextData")		
public:
	FloatingTextData(void);
	//~FloatingTextData(void);

	Color						m_color;														///< It's current color
	UnicodeString		m_text;											///< the text we're displaying
	DisplayString*	m_dString;									///< The display string
	Coord3D					m_pos3D;													///< the 3d position in game coords
	Int							m_frameTimeOut;												///< when we want this thing to disappear
	Int							m_frameCount;													///< how many frames have we been displaying text?
};

typedef std::list<FloatingTextData *> FloatingTextList;
typedef FloatingTextList::iterator	FloatingTextListIt;

enum 
{
	DEFAULT_FLOATING_TEXT_TIMEOUT = LOGICFRAMES_PER_SECOND/3,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
enum WorldAnimationOptions
{
	WORLD_ANIM_NO_OPTIONS								= 0x00000000,
	WORLD_ANIM_FADE_ON_EXPIRE						= 0x00000001,
	WORLD_ANIM_PLAY_ONCE_AND_DESTROY		= 0x00000002,
};

// ------------------------------------------------------------------------------------------------
class WorldAnimationData
{

public:

	WorldAnimationData( void );
	~WorldAnimationData( void ) { }

	Anim2D *m_anim;												///< the animation instance
	Coord3D m_worldPos;										///< position in the world
	UnsignedInt m_expireFrame;						///< frame we expire on
	WorldAnimationOptions m_options;			///< options
	Real m_zRisePerSecond;								///< Z units to rise per second

};
typedef std::list< WorldAnimationData *> WorldAnimationList;
typedef WorldAnimationList::iterator WorldAnimationListIterator;



// ------------------------------------------------------------------------------------------------
/** Basic functionality common to all in-game user interfaces */
// ------------------------------------------------------------------------------------------------ 
class InGameUI : public SubsystemInterface, public Snapshot
{
	
friend class Drawable;	// for selection/deselection transactions
		
public:  // ***************************************************************************************

	enum SelectionRules
	{
		SELECTION_ANY, //Only one of the selected units has to qualify
		SELECTION_ALL, //All selected units have to qualify
	};
	enum ActionType
	{
		ACTIONTYPE_NONE,
		ACTIONTYPE_ATTACK_OBJECT,
		ACTIONTYPE_GET_REPAIRED_AT,
		ACTIONTYPE_DOCK_AT,
		ACTIONTYPE_GET_HEALED_AT,
		ACTIONTYPE_REPAIR_OBJECT,
		ACTIONTYPE_RESUME_CONSTRUCTION,
		ACTIONTYPE_ENTER_OBJECT,
		ACTIONTYPE_HIJACK_VEHICLE,
		ACTIONTYPE_CONVERT_OBJECT_TO_CARBOMB,
		ACTIONTYPE_CAPTURE_BUILDING,
		ACTIONTYPE_DISABLE_VEHICLE_VIA_HACKING,
#ifdef ALLOW_SURRENDER
		ACTIONTYPE_PICK_UP_PRISONER,
#endif
		ACTIONTYPE_STEAL_CASH_VIA_HACKING,
		ACTIONTYPE_DISABLE_BUILDING_VIA_HACKING,
		ACTIONTYPE_MAKE_DEFECTOR,
		ACTIONTYPE_SET_RALLY_POINT,
		ACTIONTYPE_COMBATDROP_INTO,

		//Keep last.
		NUM_ACTIONTYPES
	};

	InGameUI( void );
	virtual ~InGameUI( void );
	
	// Inherited from subsystem interface -----------------------------------------------------------
	virtual	void init( void );															///< Initialize the in-game user interface
	virtual void update( void );														///< Update the UI by calling preDraw(), draw(), and postDraw()
	virtual void reset( void );															///< Reset
	//-----------------------------------------------------------------------------------------------

	// interface for the popup messages
	virtual void popupMessage( const AsciiString& message, Int x, Int y, Int width, Bool pause, Bool pauseMusic);
	virtual void popupMessage( const AsciiString& message, Int x, Int y, Int width, Color textColor, Bool pause, Bool pauseMusic);
	PopupMessageData *getPopupMessageData( void ) { return m_popupMessageData; }
	void clearPopupMessageData( void );

	// interface for messages to the user
	// srj sez: passing as const-ref screws up varargs for some reason. dunno why. just pass by value.
	virtual void messageColor( const RGBColor *rgbColor, UnicodeString format, ... );	///< display a colored message to the user
	virtual void message( UnicodeString format, ... );				  ///< display a message to the user
	virtual void message( AsciiString stringManagerLabel, ... );///< display a message to the user
	virtual void toggleMessages( void ) { m_messagesOn = 1 - m_messagesOn; }	///< toggle messages on/off
	virtual Bool isMessagesOn( void ) { return m_messagesOn; }	///< are the display messages on
	void freeMessageResources( void );				///< free resources for the ui messages
	Color getMessageColor(Bool altColor) { return (altColor)?m_messageColor2:m_messageColor1; }
	
	// interface for military style messages
	virtual void militarySubtitle( const AsciiString& label, Int duration );			// time in milliseconds
	virtual void removeMilitarySubtitle( void );
	
	// for can't build messages
	virtual void displayCantBuildMessage( LegalBuildCode lbc ); ///< display message to use as to why they can't build here

	// interface for graphical "hints" which provide visual feedback for user-interface commands
	virtual void beginAreaSelectHint( const GameMessage *msg );	///< Used by HintSpy. An area selection is occurring, start graphical "hint"
	virtual void endAreaSelectHint( const GameMessage *msg );		///< Used by HintSpy. An area selection had occurred, finish graphical "hint"
	virtual void createMoveHint( const GameMessage *msg );			///< A move command has occurred, start graphical "hint"
	virtual void createAttackHint( const GameMessage *msg );		///< An attack command has occurred, start graphical "hint"
	virtual void createForceAttackHint( const GameMessage *msg );		///< A force attack command has occurred, start graphical "hint"

	virtual void createMouseoverHint( const GameMessage *msg );	///< An object is mouse hovered over, start hint if any
	virtual void createCommandHint( const GameMessage *msg );		///< Used by HintSpy. Someone is selected so generate the right Cursor for the potential action
	virtual void createGarrisonHint( const GameMessage *msg );  ///< A garrison command has occurred, start graphical "hint"
	
	virtual void addSuperweapon(Int playerIndex, const AsciiString& powerName, ObjectID id, const SpecialPowerTemplate *powerTemplate);
	virtual Bool removeSuperweapon(Int playerIndex, const AsciiString& powerName, ObjectID id, const SpecialPowerTemplate *powerTemplate);
	virtual void objectChangedTeam(const Object *obj, Int oldPlayerIndex, Int newPlayerIndex);	// notification for superweapons, etc

	virtual void setSuperweaponDisplayEnabledByScript( Bool enable );	///< Set the superweapon display enabled or disabled
	virtual Bool getSuperweaponDisplayEnabledByScript( void ) const;				///< Get the current superweapon display status

	virtual void hideObjectSuperweaponDisplayByScript(const Object *obj);
	virtual void showObjectSuperweaponDisplayByScript(const Object *obj);

	void addNamedTimer( const AsciiString& timerName, const UnicodeString& text, Bool isCountdown );
	void removeNamedTimer( const AsciiString& timerName );
	void showNamedTimerDisplay( Bool show );

	// mouse mode interface
	virtual void setScrolling( Bool isScrolling );							///< set right-click scroll mode
	virtual Bool isScrolling( void );														///< are we scrolling?
	virtual void setSelecting( Bool isSelecting );							///< set drag select mode
	virtual Bool isSelecting( void );														///< are we selecting?
	virtual void setScrollAmount( Coord2D amt );								///< set scroll amount
	virtual Coord2D getScrollAmount( void );										///< get scroll amount

	// gui command interface
	virtual void setGUICommand( const CommandButton *command );				///< the command has been clicked in the UI and needs additional data
	virtual const CommandButton *getGUICommand( void ) const;								///< get the pending gui command

	// build interface
	virtual void placeBuildAvailable( const ThingTemplate *build, Drawable *buildDrawable );				///< built thing being placed
	virtual const ThingTemplate *getPendingPlaceType( void );					///< get item we're trying to place
	virtual const ObjectID getPendingPlaceSourceObjectID( void );			///< get producing object
	virtual void setPlacementStart( const ICoord2D *start );					///< placement anchor point (for choosing angles)
	virtual void setPlacementEnd( const ICoord2D *end );							///< set target placement point (for choosing angles)
	virtual Bool isPlacementAnchored( void );													///< is placement arrow anchor set
	virtual void getPlacementPoints( ICoord2D *start, ICoord2D *end );///< get the placemnt arrow points
	virtual Real getPlacementAngle( void );														///< placement angle of drawable at cursor when placing down structures

	// Drawable selection mechanisms
	virtual void selectDrawable( Drawable *draw );					///< Mark given Drawable as "selected"
	virtual void deselectDrawable( Drawable *draw );				///< Clear "selected" status from Drawable
	virtual void deselectAllDrawables( Bool postMsg = true );							///< Clear the "select" flag from all drawables
	virtual Int getSelectCount( void ) { return m_selectCount; }		///< Get count of currently selected drawables
	virtual Int getMaxSelectCount( void ) { return m_maxSelectCount; }	///< Get the max number of selected drawables
	virtual UnsignedInt getFrameSelectionChanged( void ) { return m_frameSelectionChanged; }	///< Get the max number of selected drawables
	virtual const DrawableList *getAllSelectedDrawables( void ) const;	///< Return the list of all the currently selected Drawable IDs.
	virtual const DrawableList *getAllSelectedLocalDrawables( void );		///< Return the list of all the currently selected Drawable IDs owned by the current player.
	virtual Drawable *getFirstSelectedDrawable( void );							///< get the first selected drawable (if any)
	virtual DrawableID getSoloNexusSelectedDrawableID( void ) { return m_soloNexusSelectedDrawableID; }  ///< Return the one drawable of the nexus if only 1 angry mob is selected 
	virtual Bool isDrawableSelected( DrawableID idToCheck ) const;	///< Return true if the selected ID is in the drawable list
	virtual Bool isAnySelectedKindOf( KindOfType kindOf ) const;		///< is any selected object a kind of 
	virtual Bool isAllSelectedKindOf( KindOfType kindOf ) const;		///< are all selected objects a kind of

	virtual void setRadiusCursor(RadiusCursorType r, const SpecialPowerTemplate* sp, WeaponSlotType wslot);
	virtual void setRadiusCursorNone() { setRadiusCursor(RADIUSCURSOR_NONE, NULL, PRIMARY_WEAPON); }

	virtual void setInputEnabled( Bool enable );										///< Set the input enabled or disabled
	virtual Bool getInputEnabled( void ) { return m_inputEnabled; }	///< Get the current input status

	virtual void disregardDrawable( Drawable *draw );				///< Drawable is being destroyed, clean up any UI elements associated with it

	virtual void preDraw( void );														///< Logic which needs to occur before the UI renders
	virtual void draw( void ) = 0;													///< Render the in-game user interface
	virtual void postDraw( void );													///< Logic which needs to occur after the UI renders

	/// Ingame video playback 
	virtual void playMovie( const AsciiString& movieName );
	virtual void stopMovie( void );
	virtual VideoBuffer* videoBuffer( void );

	/// Ingame cameo video playback 
	virtual void playCameoMovie( const AsciiString& movieName );
	virtual void stopCameoMovie( void );
	virtual VideoBuffer* cameoVideoBuffer( void );

  // mouse over information	
	virtual DrawableID getMousedOverDrawableID( void ) const;	///< Get drawble ID of drawable under cursor
	
	/// Set the ingame flag as to if we have the Quit menu up or not
	virtual void setQuitMenuVisible( Bool t ) { m_isQuitMenuVisible = t; }
	virtual Bool isQuitMenuVisible( void ) const { return m_isQuitMenuVisible; }

	// INI file parsing
	virtual const FieldParse* getFieldParse( void ) const { return s_fieldParseTable; }


	//Provides a global way to determine whether or not we can issue orders to what we have selected.
	Bool areSelectedObjectsControllable() const;
	//Wrapper function that includes any non-attack canSelectedObjectsXXX checks.
	Bool canSelectedObjectsNonAttackInteractWithObject( const Object *objectToInteractWith, SelectionRules rule ) const;
	//Wrapper function that checks a specific action.
	CanAttackResult getCanSelectedObjectsAttack( ActionType action, const Object *objectToInteractWith, SelectionRules rule, Bool additionalChecking = FALSE ) const;
	Bool canSelectedObjectsDoAction( ActionType action, const Object *objectToInteractWith, SelectionRules rule, Bool additionalChecking = FALSE ) const;
	Bool canSelectedObjectsDoSpecialPower( const CommandButton *command, const Object *objectToInteractWith, const Coord3D *position, SelectionRules rule, UnsignedInt commandOptions, Object* ignoreSelObj ) const;
	Bool canSelectedObjectsEffectivelyUseWeapon( const CommandButton *command, const Object *objectToInteractWith, const Coord3D *position, SelectionRules rule ) const;
	Bool canSelectedObjectsOverrideSpecialPowerDestination( const Coord3D *loc, SelectionRules rule, SpecialPowerType spType = SPECIAL_INVALID ) const;

	// Selection Methods
	virtual Int selectMatchingUnits();                        ///< selects matching units
	virtual Int selectAcrossScreen();                         ///< selects matching units across screen
	virtual Int selectAcrossMap();                            ///< selects matching units across map
	virtual Int selectAcrossRegion( IRegion2D *region );			// -1 = no locally-owned selection, 0+ = # of units selected
	virtual void buildRegion( const ICoord2D *anchor, const ICoord2D *dest, IRegion2D *region );  ///< builds a region around the specified coordinates

	virtual Bool getDisplayedMaxWarning( void ) { return m_displayedMaxWarning; }
	virtual void setDisplayedMaxWarning( Bool selected ) { m_displayedMaxWarning = selected; }

	// Floating Test Methods
	virtual void addFloatingText(const UnicodeString& text,const Coord3D * pos, Color color);

	// Drawable caption stuff
	AsciiString	getDrawableCaptionFontName( void )	{ return m_drawableCaptionFont; }
	Int					getDrawableCaptionPointSize( void )	{ return m_drawableCaptionPointSize; }
	Bool				isDrawableCaptionBold( void )				{ return m_drawableCaptionBold; }
	Color				getDrawableCaptionColor( void )			{ return m_drawableCaptionColor; }

	inline Bool shouldMoveRMBScrollAnchor( void ) { return m_moveRMBScrollAnchor; }

	Bool isClientQuiet( void ) const			{ return m_clientQuiet; }
	Bool isInWaypointMode( void ) const			{ return m_waypointMode; }
	Bool isInForceAttackMode( void ) const	{ return m_forceAttackMode; }
	Bool isInForceMoveToMode( void ) const	{ return m_forceMoveToMode; }
	Bool isInPreferSelectionMode( void ) const { return m_preferSelection; }

	void setClientQuiet( Bool enabled )  { m_clientQuiet = enabled; }
	void setWaypointMode( Bool enabled )		{ m_waypointMode = enabled; }
	void setForceMoveMode( Bool enabled )		{ m_forceMoveToMode = enabled; }
	void setForceAttackMode( Bool enabled )		{ m_forceAttackMode = enabled; }
	void setPreferSelectionMode( Bool enabled )		{ m_preferSelection = enabled; }
	
	void toggleAttackMoveToMode( void )				{ m_attackMoveToMode = !m_attackMoveToMode; }
	Bool isInAttackMoveToMode( void ) const		{ return m_attackMoveToMode; }
	void clearAttackMoveToMode( void )				{ m_attackMoveToMode = FALSE; }
	
	void setCameraRotateLeft( Bool set )		{ m_cameraRotatingLeft = set; }
	void setCameraRotateRight( Bool set )		{ m_cameraRotatingRight = set; }
	void setCameraZoomIn( Bool set )				{ m_cameraZoomingIn = set; }
	void setCameraZoomOut( Bool set )				{ m_cameraZoomingOut = set; }
	Bool isCameraRotatingLeft() const { return m_cameraRotatingLeft; }
	Bool isCameraRotatingRight() const { return m_cameraRotatingRight; }
	Bool isCameraZoomingIn() const { return m_cameraZoomingIn; }
	Bool isCameraZoomingOut() const { return m_cameraZoomingOut; }
	void resetCamera();

	virtual void addIdleWorker( Object *obj );
	virtual void removeIdleWorker( Object *obj, Int playerNumber );
	virtual void selectNextIdleWorker( void );

	virtual void recreateControlBar( void );

	virtual void disableTooltipsUntil(UnsignedInt frameNum);
	virtual void clearTooltipsDisabled();
	virtual Bool areTooltipsDisabled() const;

	Bool getDrawRMBScrollAnchor() const { return m_drawRMBScrollAnchor; }
	Bool getMoveRMBScrollAnchor() const { return m_moveRMBScrollAnchor; }

	void setDrawRMBScrollAnchor(Bool b) { m_drawRMBScrollAnchor = b; }
	void setMoveRMBScrollAnchor(Bool b) { m_moveRMBScrollAnchor = b; }

private:
	virtual Int getIdleWorkerCount( void );
	virtual Object *findIdleWorker( Object *obj);
	virtual void showIdleWorkerLayout( void );
	virtual void hideIdleWorkerLayout( void );
	virtual void updateIdleWorker( void );
	virtual void resetIdleWorker( void );

public:
	void registerWindowLayout(WindowLayout *layout); // register a layout for updates
	void unregisterWindowLayout(WindowLayout *layout); // stop updates for this layout

public:
	// World 2D animation methods
	void addWorldAnimation( Anim2DTemplate *animTemplate, 
													const Coord3D *pos,
													WorldAnimationOptions options,
													Real durationInSeconds,
													Real zRisePerSecond );

#if defined(_DEBUG) || defined(_INTERNAL)
	virtual void DEBUG_addFloatingText(const AsciiString& text,const Coord3D * pos, Color color);
#endif

protected:
	// snapshot methods
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess( void );

protected: 

	// ----------------------------------------------------------------------------------------------
	// Protected Types ------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------

	enum HintType
	{
		MOVE_HINT = 0,
		ATTACK_HINT,
#ifdef _DEBUG
		DEBUG_HINT,
#endif
		NUM_HINT_TYPES  // keep this one last
	};

	// mouse mode interface
	enum MouseMode 
	{
		MOUSEMODE_DEFAULT = 0,
		MOUSEMODE_BUILD_PLACE,
		MOUSEMODE_GUI_COMMAND,
		MOUSEMODE_MAX
	};

	enum { MAX_MOVE_HINTS = 256 };
	struct MoveHintStruct
	{
		Coord3D pos;						///< World coords of destination point
		UnsignedInt sourceID;		///< id of who will move to this point
		UnsignedInt frame;			///< frame the command was issued on
	};

	struct UIMessage
	{
		UnicodeString fullText;									///< the whole text message
		DisplayString *displayString;						///< display string used to render the message
		UnsignedInt timestamp;									///< logic frame message was created on
		Color color;														///< color to render this in
	};
	enum { MAX_UI_MESSAGES = 6 };

	struct MilitarySubtitleData
	{
		UnicodeString subtitle;										///< The complete subtitle to be drawn, each line is separated by L"\n"
		UnsignedInt index;												///< the current index that we are at through the sibtitle
		ICoord2D position;												///< Where on the screen the subtitle should be drawn
		DisplayString *displayStrings[MAX_SUBTITLE_LINES];	///< We'll only allow MAX_SUBTITLE_LINES worth of display strings
		UnsignedInt currentDisplayString;					///< contains the current display string we're on. (also lets us know the last display string allocated
		UnsignedInt lifetime;											///< the Lifetime of the Military Subtitle in frames
		Bool blockDrawn;													///< True if the block is drawn false if it's blank
		UnsignedInt blockBeginFrame;							///< The frame at which the block started it's current state
		ICoord2D blockPos;												///< where the upper left of the block should begin
		UnsignedInt incrementOnFrame;							///< if we're currently on a frame greater then this, increment our position
		Color color;															///< what color should we display the military subtitles
	};

	typedef std::list<Object *> ObjectList;
	typedef std::list<Object *>::iterator ObjectListIt;
	
	// ----------------------------------------------------------------------------------------------
	// Protected Methods ----------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------

	void destroyPlacementIcons( void );													///< Destroy placement icons
	void handleBuildPlacements( void );													///< handle updating of placement icons based on mouse pos
	void handleRadiusCursor();																	///< handle updating of "radius cursors" that follow the mouse pos

	void incrementSelectCount( void ) { ++m_selectCount; }			///< Increase by one the running total of "selected" drawables
	void decrementSelectCount( void ) { --m_selectCount; }			///< Decrease by one the running total of "selected" drawables
	virtual View *createView( void ) = 0;												///< Factory for Views
	void evaluateSoloNexus( Drawable *newlyAddedDrawable = NULL );

	/// expire a hint from of the specified type at the hint index
	void expireHint( HintType type, UnsignedInt hintIndex );

	void createControlBar( void );			///< create the control bar user interface
	void createReplayControl( void );		///< create the replay control window

	void setMouseCursor(Mouse::MouseCursor c);

	
	void addMessageText( const UnicodeString& formattedMessage, const RGBColor *rgbColor = NULL );  ///< internal workhorse for adding plain text for messages
	void removeMessageAtIndex( Int i );				///< remove the message at index i

	void updateFloatingText( void );						///< Update function to move our floating text
	void drawFloatingText( void );							///< Draw all our floating text
	void clearFloatingText( void );							///< clear the floating text list

	void clearWorldAnimations( void );					///< delete all world animations
	void updateAndDrawWorldAnimations( void );	///< update and draw visible world animations

	SuperweaponInfo* findSWInfo(Int playerIndex, const AsciiString& powerName, ObjectID id, const SpecialPowerTemplate *powerTemplate);

	// ----------------------------------------------------------------------------------------------
	// Protected Data THAT IS SAVED/LOADED ----------------------------------------------------------
	// ----------------------------------------------------------------------------------------------

	Bool												m_superweaponHiddenByScript;
	Bool												m_inputEnabled;		/// sort of

	// ----------------------------------------------------------------------------------------------
	// Protected Data -------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------

	std::list<WindowLayout *>		m_windowLayouts;
	AsciiString									m_currentlyPlayingMovie;											///< Used to push updates to TheScriptEngine
	DrawableList								m_selectedDrawables;													///< A list of all selected drawables.
	DrawableList								m_selectedLocalDrawables;											///< A list of all selected drawables owned by the local player
	Bool												m_isDragSelecting;														///< If TRUE, an area selection is in progress
	IRegion2D										m_dragSelectRegion;														///< if isDragSelecting is TRUE, this contains select region
	Bool												m_displayedMaxWarning;                        ///< keeps the warning from being shown over and over
	MoveHintStruct							m_moveHint[ MAX_MOVE_HINTS ];
	Int													m_nextMoveHint;
	const CommandButton *				m_pendingGUICommand;										///< GUI command that needs additional interaction from the user
	BuildProgress								m_buildProgress[ MAX_BUILD_PROGRESS ];	///< progress for building units
	const ThingTemplate *				m_pendingPlaceType;											///< type of built thing we're trying to place
	ObjectID										m_pendingPlaceSourceObjectID;						///< source object of the thing constructing the item
	Drawable **									m_placeIcon;														///< array for drawables to appear at the cursor when building in the world
	Bool												m_placeAnchorInProgress;								///< is place angle interface for placement active
	ICoord2D										m_placeAnchorStart;											///< place angle anchor start
	ICoord2D										m_placeAnchorEnd;												///< place angle anchor end
	Int													m_selectCount;													///< Number of objects currently "selected"
	Int													m_maxSelectCount;												///< Max number of objects to select
	UnsignedInt									m_frameSelectionChanged;								///< Frame when the selection last changed.


	// Video playback data
	VideoBuffer*								m_videoBuffer;			///< video playback buffer
	VideoStreamInterface*				m_videoStream;			///< Video stream;

	// Video playback data
	VideoBuffer*								m_cameoVideoBuffer;///< video playback buffer
	VideoStreamInterface*				m_cameoVideoStream;///< Video stream;

	// message data
	UIMessage										m_uiMessages[ MAX_UI_MESSAGES ];/**< messages to display to the user, the
																						array is organized with newer messages at
																						index 0, and increasing to older ones */
	// superweapon timer data
	SuperweaponMap							m_superweapons[MAX_PLAYER_COUNT];
	Coord2D											m_superweaponPosition;
	Real												m_superweaponFlashDuration;
	
	// superweapon timer font info
	AsciiString									m_superweaponNormalFont;
	Int													m_superweaponNormalPointSize;
	Bool												m_superweaponNormalBold;
	AsciiString									m_superweaponReadyFont;
	Int													m_superweaponReadyPointSize;
	Bool												m_superweaponReadyBold;

	Int													m_superweaponLastFlashFrame;										///< for flashing the text when the weapon is ready
	Color												m_superweaponFlashColor;
	Bool												m_superweaponUsedFlashColor;

	NamedTimerMap								m_namedTimers;
	Coord2D											m_namedTimerPosition;
	Real												m_namedTimerFlashDuration;
	Int													m_namedTimerLastFlashFrame;
	Color												m_namedTimerFlashColor;
	Bool												m_namedTimerUsedFlashColor;
	Bool												m_showNamedTimers;

	AsciiString									m_namedTimerNormalFont;
	Int													m_namedTimerNormalPointSize;
	Bool												m_namedTimerNormalBold;
	Color												m_namedTimerNormalColor;
	AsciiString									m_namedTimerReadyFont;
	Int													m_namedTimerReadyPointSize;
	Bool												m_namedTimerReadyBold;
	Color												m_namedTimerReadyColor;

	// Drawable caption data
	AsciiString									m_drawableCaptionFont;
	Int													m_drawableCaptionPointSize;
	Bool												m_drawableCaptionBold;
	Color												m_drawableCaptionColor;

	UnsignedInt									m_tooltipsDisabledUntil;

	// Military Subtitle data
	MilitarySubtitleData *			m_militarySubtitle;		///< The pointer to subtitle class, if it's present then draw it.
	Bool												m_isScrolling;
	Bool												m_isSelecting;
	MouseMode										m_mouseMode;
	Int													m_mouseModeCursor;
	DrawableID									m_mousedOverDrawableID;
	Coord2D											m_scrollAmt;
	Bool												m_isQuitMenuVisible;
	Bool												m_messagesOn;

	Color												m_messageColor1;
	Color												m_messageColor2;
	ICoord2D										m_messagePosition;
	AsciiString									m_messageFont;
	Int													m_messagePointSize;
	Bool												m_messageBold;
	Int													m_messageDelayMS;

	RGBAColorInt								m_militaryCaptionColor;				///< color for the military-style caption
	ICoord2D										m_militaryCaptionPosition;					///< position for the military-style caption

	AsciiString									m_militaryCaptionTitleFont;
	Int													m_militaryCaptionTitlePointSize;
	Bool												m_militaryCaptionTitleBold;

	AsciiString									m_militaryCaptionFont;
	Int													m_militaryCaptionPointSize;
	Bool												m_militaryCaptionBold;

	Bool												m_militaryCaptionRandomizeTyping;
	Int													m_militaryCaptionDelayMS;
	Int													m_militaryCaptionSpeed;

	RadiusDecalTemplate					m_radiusCursors[RADIUSCURSOR_COUNT];
	RadiusDecal									m_curRadiusCursor;
	RadiusCursorType						m_curRcType;

	//Floating Text Data
	FloatingTextList						m_floatingTextList;				///< Our list of floating text
	UnsignedInt									m_floatingTextTimeOut;									///< Ini value of our floating text timeout
	Real												m_floatingTextMoveUpSpeed;							///< INI value of our Move up speed
	Real												m_floatingTextMoveVanishRate;					///< INI value of our move vanish rate

	PopupMessageData *					m_popupMessageData;
	Color												m_popupMessageColor;
	
 	Bool												m_waypointMode;			///< are we in waypoint plotting mode?
	Bool												m_forceAttackMode;		///< are we in force attack mode?
	Bool												m_forceMoveToMode;		///< are we in force move mode?
	Bool												m_attackMoveToMode;	///< are we in attack move mode?
	Bool												m_preferSelection;		///< the shift key has been depressed.

	Bool												m_cameraRotatingLeft; 
	Bool 												m_cameraRotatingRight;
	Bool 												m_cameraZoomingIn;
	Bool 												m_cameraZoomingOut;
	
	Bool												m_drawRMBScrollAnchor;
	Bool												m_moveRMBScrollAnchor;
	Bool												m_clientQuiet;         ///< When the user clicks exit,restart, etc. this is set true 
																												///< to skip some client sounds/fx during shutdown

	// World Animation Data
	WorldAnimationList					m_worldAnimationList;		///< the list of world animations

	// Idle worker animation
	ObjectList									m_idleWorkers[MAX_PLAYER_COUNT];
	GameWindow *								m_idleWorkerWin;
	Int													m_currentIdleWorkerDisplay;

	DrawableID									m_soloNexusSelectedDrawableID;  ///< The drawable of the nexus, if only one angry mob is selected, otherwise, null

	// ----------------------------------------------------------------------------------------------
	// STATIC Protected Data -------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------

	static const FieldParse s_fieldParseTable[];

};

// the singleton
extern InGameUI *TheInGameUI;

#endif // _IN_GAME_UI_H_
