#include "PreRTS.h"

#include "Common/MapObject.h"
#include "Common/DataChunk.h"
#include "Common/FileSystem.h" // for LOAD_TEST_ASSETS
#include "Common/GlobalData.h"
#include "Common/MapReaderWriterInfo.h"
#include "Common/TerrainTypes.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/WellKnownKeys.h"
#include "GameLogic/SidesList.h"

/* ********* MapObject class ****************************/
/*static*/ MapObject *MapObject::TheMapObjectListPtr = NULL;
/*static*/ Dict MapObject::TheWorldDict;

MapObject::MapObject(Coord3D loc, AsciiString name, Real angle, Int flags, const Dict* props,
										 const ThingTemplate *thingTemplate )
{
	m_objectName = name;
	m_thingTemplate = thingTemplate;
	m_nextMapObject = NULL;
	m_location = loc;
	m_angle = normalizeAngle(angle);
	m_color = (0xff)<<8; // Bright green.
	m_flags = flags;
	m_renderObj = NULL;
	m_shadowObj = NULL;
	m_runtimeFlags = 0;

	if (props)
	{
		m_properties = *props;
		if (thingTemplate && !props->known(TheKey_objectSelectable, Dict::DICT_BOOL)) {
			Bool selectable = thingTemplate->isKindOf(KINDOF_SELECTABLE);
			m_properties.setBool(TheKey_objectSelectable, selectable);
		}
	} 
	else 
	{
		m_properties.setInt(TheKey_objectInitialHealth, 100);
		m_properties.setBool(TheKey_objectEnabled, true);
		m_properties.setBool(TheKey_objectIndestructible, false);
		m_properties.setBool(TheKey_objectUnsellable, false);
		m_properties.setBool(TheKey_objectPowered, true);
		m_properties.setBool(TheKey_objectRecruitableAI, true);
		m_properties.setBool(TheKey_objectTargetable, false );
		
		Bool selectable = false;
		if (thingTemplate) {
			selectable = thingTemplate->isKindOf(KINDOF_SELECTABLE);
		}
		m_properties.setBool(TheKey_objectSelectable, selectable);
	}

	for( Int i = 0; i < BRIDGE_MAX_TOWERS; ++i )
		setBridgeRenderObject( (BridgeTowerType)i, NULL );

}	


MapObject::~MapObject(void)
{
	// setRenderObj(NULL);
	// setShadowObj(NULL);
	if (m_nextMapObject) {
		MapObject *cur = m_nextMapObject;
		MapObject *next;
		while (cur) {
			next = cur->getNext();
			cur->setNextMap(NULL); // prevents recursion. 
			cur->deleteInstance();
			cur = next;
		}
	}
	for( Int i = 0; i < BRIDGE_MAX_TOWERS; ++i )	
		setBridgeRenderObject( (BridgeTowerType)i, NULL );

}

MapObject *MapObject::duplicate(void)
{
	MapObject *pObj = newInstance( MapObject)(m_location, m_objectName, m_angle, m_flags, &m_properties, m_thingTemplate);
	pObj->setColor(getColor());
	pObj->m_runtimeFlags = m_runtimeFlags;
	return pObj;
}

#if 0
void MapObject::setRenderObj(RenderObjClass *pObj)
{
	REF_PTR_SET(m_renderObj, pObj);
}

void MapObject::setBridgeRenderObject( BridgeTowerType type, RenderObjClass* renderObj )
{

	if( type >= 0 && type < BRIDGE_MAX_TOWERS )
		REF_PTR_SET( m_bridgeTowers[ type ], renderObj );

}
#endif

RenderObjClass* MapObject::getBridgeRenderObject( BridgeTowerType type )
{

	if( type >= 0 && type < BRIDGE_MAX_TOWERS )
		return m_bridgeTowers[ type ];
	return NULL;

}

void MapObject::validate(void)
{
	verifyValidTeam();
	verifyValidUniqueID();
}

void MapObject::verifyValidTeam(void)
{
	// if this map object has a valid team, then do nothing.
	// if it has an invalid team, the place it on the default neutral team, (by clearing the 
	// existing team name.)
	Bool exists;
	AsciiString teamName = getProperties()->getAsciiString(TheKey_originalOwner, &exists);
	if (exists) {
		Bool valid = false;

		int numSides = TheSidesList->getNumTeams();

		for (int i = 0; i < numSides; ++i) {
			TeamsInfo *teamInfo = TheSidesList->getTeamInfo(i);
			if (!teamInfo) {
				continue;
			}
			
			Bool itBetter;
			AsciiString testAgainstTeamName = teamInfo->getDict()->getAsciiString(TheKey_teamName, &itBetter);
			if (itBetter) {
				if (testAgainstTeamName.compare(teamName) == 0) {
					valid = true;
				}
			}
		}

		if (!valid) {
			getProperties()->remove(TheKey_originalOwner);
		}
	}
}

void MapObject::verifyValidUniqueID(void)
{
	Bool exists;
	AsciiString uniqueID = getProperties()->getAsciiString(TheKey_uniqueID, &exists);
	MapObject *obj = MapObject::getFirstMapObject();

	// -1 is the sentinel
	int highestIndex = -1;

	while (obj) {
		if (obj == this) {
			// the first object is THIS OBJECT, cause we've already been added. 
			obj = obj->getNext();
			continue;
		}

		if (obj->isWaypoint()) {
			// waypoints throw this off. Sad but true. :-(
			obj = obj->getNext();
			continue;
		}

		Bool iterateExists;
		AsciiString tempStr = obj->getProperties()->getAsciiString(TheKey_uniqueID, &iterateExists);
		const char* lastSpace = tempStr.reverseFind(' ');

		int testIndex = -1; 
		if (lastSpace) {
			testIndex = atoi(lastSpace);
		}

		if (testIndex > highestIndex) {
			highestIndex = testIndex;
		}
		break;
	}

	int indexOfThisObject = highestIndex + 1;
	
	const char* thingName;
	if (getThingTemplate()) {
		thingName = getThingTemplate()->getName().str();
	} else if (isWaypoint()) {
		thingName = getWaypointName().str();
	} else {
		thingName = getName().str();
	}
	const char* pName = thingName;

	while (*thingName) {
		if ((*thingName) == '/') {
			pName = thingName + 1;
		}
		++thingName;
	}

	AsciiString newID;
	if (isWaypoint()) {
		newID.format("%s", pName);
	} else {
		newID.format("%s %d", pName, indexOfThisObject);
	}
	getProperties()->setAsciiString(TheKey_uniqueID, newID);
}

void MapObject::fastAssignAllUniqueIDs(void)
{
	// here's what we do. Take all of them, push them onto a stack. Then, pop each one, setting its id.
	// should be much faster than what we currently do.

	MapObject *pMapObj = getFirstMapObject();

	std::stack<MapObject*> objStack;
	Int actualNumObjects = 0;
	
	while (pMapObj) {
		++actualNumObjects;
		objStack.push(pMapObj);
		pMapObj = pMapObj->getNext();
	}

	Int indexOfThisObject = 0;
	while (actualNumObjects) {
		MapObject *obj = objStack.top();
		

		const char* thingName;
		if (obj->getThingTemplate()) {
			thingName = obj->getThingTemplate()->getName().str();
		} else if (obj->isWaypoint()) {
			thingName = obj->getWaypointName().str();
		} else {
			thingName = obj->getName().str();
		}
		const char* pName = thingName;

		while (*thingName) {
			if ((*thingName) == '/') {
				pName = thingName + 1;
			}
			++thingName;
		}

		AsciiString newID;
		if (obj->isWaypoint()) {
			newID.format("%s", pName);
		} else {
			newID.format("%s %d", pName, indexOfThisObject);
		}

		obj->getProperties()->setAsciiString(TheKey_uniqueID, newID);
		objStack.pop();
	
		++indexOfThisObject;
		--actualNumObjects;
	}
}



void MapObject::setThingTemplate(const ThingTemplate *thing)
{
	m_thingTemplate = thing;
	m_objectName = thing->getName();
}


void MapObject::setName(AsciiString name)
{
	m_objectName = name;
}

WaypointID MapObject::getWaypointID() { return (WaypointID)getProperties()->getInt(TheKey_waypointID); }
AsciiString MapObject::getWaypointName() { return getProperties()->getAsciiString(TheKey_waypointName); }
void MapObject::setWaypointID(Int i) { getProperties()->setInt(TheKey_waypointID, i); }
void MapObject::setWaypointName(AsciiString n) { getProperties()->setAsciiString(TheKey_waypointName, n); }

/*static */ Int MapObject::countMapObjectsWithOwner(const AsciiString& n)
{
	Int count = 0;
	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext()) 
	{
		if (pMapObj->getProperties()->getAsciiString(TheKey_originalOwner) == n)
			++count;
	}
	return count;
}

//-------------------------------------------------------------------------------------------------
const ThingTemplate *MapObject::getThingTemplate( void ) const
{
	if (m_thingTemplate)
		return (const ThingTemplate*) m_thingTemplate->getFinalOverride(); 
	
	return NULL;
}
