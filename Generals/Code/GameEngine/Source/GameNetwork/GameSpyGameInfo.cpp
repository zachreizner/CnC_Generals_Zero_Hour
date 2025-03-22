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

// FILE: GameSpyGameInfo.cpp //////////////////////////////////////////////////////
// GameSpy game setup state info
// Author: Matthew D. Campbell, December 2001

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/GameEngine.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/RandomValue.h"
#include "Common/ScoreKeeper.h"
#include "GameClient/Shell.h"
#include "GameClient/GameText.h"
#include "GameNetwork/GameSpy/PeerDefs.h"
#include "GameNetwork/GameSpyGameInfo.h"
#include "GameNetwork/NetworkInterface.h"
#include "GameNetwork/NetworkUtil.h"
#include "GameNetwork/NetworkDefs.h"
#include "GameNetwork/NAT.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/VictoryConditions.h"

// Singleton ------------------------------------------

GameSpyGameInfo *TheGameSpyGame = NULL;

// Helper Functions ----------------------------------------

GameSpyGameSlot::GameSpyGameSlot()
{
	GameSlot();
	m_gameSpyLogin.clear();
	m_gameSpyLocale.clear();
	m_profileID = 0;
}

// Helper Functions ----------------------------------------
/*
** Function definitions for the MIB-II entry points.
*/

// BOOL (__stdcall *SnmpExtensionInitPtr)(IN DWORD dwUpTimeReference, OUT HANDLE *phSubagentTrapEvent, OUT AsnObjectIdentifier *pFirstSupportedRegion);
// BOOL (__stdcall *SnmpExtensionQueryPtr)(IN BYTE bPduType, IN OUT RFC1157VarBindList *pVarBindList, OUT AsnInteger32 *pErrorStatus, OUT AsnInteger32 *pErrorIndex);
// LPVOID (__stdcall *SnmpUtilMemAllocPtr)(IN DWORD bytes);
// VOID (__stdcall *SnmpUtilMemFreePtr)(IN LPVOID pMem);

typedef struct tConnInfoStruct {
	unsigned int State;
	unsigned long LocalIP;
	unsigned short LocalPort;
	unsigned long RemoteIP;
	unsigned short RemotePort;
} ConnInfoStruct;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

/***********************************************************************************************
 * Get_Local_Chat_Connection_Address -- Which address are we using to talk to the chat server? *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Ptr to address to return local address                                            *                                                                                             *
 *                                                                                             *
 * OUTPUT:   True if success                                                                   *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/27/00 3:24PM ST : Created                                                              *
 *=============================================================================================*/
Bool GetLocalChatConnectionAddress(AsciiString serverName, UnsignedShort serverPort, UnsignedInt& localIP)
{
#if 0
	//return false;
	/*
	** Local defines.
	*/
	enum {
		CLOSED = 1,
		LISTENING,
		SYN_SENT,
		SEN_RECEIVED,
		ESTABLISHED,
		FIN_WAIT,
		FIN_WAIT2,
		CLOSE_WAIT,
		LAST_ACK,
		CLOSING,
		TIME_WAIT,
		DELETE_TCB
	};

	enum {
		tcpConnState = 1,
		tcpConnLocalAddress,
		tcpConnLocalPort,
		tcpConnRemAddress,
		tcpConnRemPort
	};


	/*
	** Locals.
	*/
	unsigned char serverAddress[4];
	unsigned char remoteAddress[4];
	HANDLE trap_handle;
	AsnObjectIdentifier first_supported_region;
	std::vector<ConnInfoStruct> connectionVector;
	int last_field;
	int index;
	AsnInteger error_status;
	AsnInteger error_index;
	int conn_entry_type_index;
	int conn_entry_type;
	Bool found;

	/*
	** Statics.
	*/
	static char _conn_state[][32] = {
		"?",
		"CLOSED",
		"LISTENING",
		"SYN_SENT",
		"SEN_RECEIVED",
		"ESTABLISHED",
		"FIN_WAIT",
		"FIN_WAIT2",
		"CLOSE_WAIT",
		"LAST_ACK",
		"CLOSING",
		"TIME_WAIT",
		"DELETE_TCB"
	};

	DEBUG_LOG(("Finding local address used to talk to the chat server\n"));
	DEBUG_LOG(("Current chat server name is %s\n", serverName.str()));
	DEBUG_LOG(("Chat server port is %d\n", serverPort));

	/*
	** Get the address of the chat server.
	*/
	DEBUG_LOG( ("About to call gethostbyname\n"));
	struct hostent *host_info = gethostbyname(serverName.str());

	if (!host_info) {
		DEBUG_LOG( ("gethostbyname failed! Error code %d\n", WSAGetLastError()));
		return(false);
	}

	memcpy(serverAddress, &host_info->h_addr_list[0][0], 4);
	unsigned long temp = *((unsigned long*)(&serverAddress[0]));
	temp = ntohl(temp);
	*((unsigned long*)(&serverAddress[0])) = temp;

	DEBUG_LOG(("Host address is %d.%d.%d.%d\n", serverAddress[3], serverAddress[2], serverAddress[1], serverAddress[0]));

	/*
	** Load the MIB-II SNMP DLL.
	*/
	DEBUG_LOG(("About to load INETMIB1.DLL\n"));

	HINSTANCE mib_ii_dll = LoadLibrary("inetmib1.dll");
	if (mib_ii_dll == NULL) {
		DEBUG_LOG(("Failed to load INETMIB1.DLL\n"));
		return(false);
	}

	DEBUG_LOG(("About to load SNMPAPI.DLL\n"));

	HINSTANCE snmpapi_dll = LoadLibrary("snmpapi.dll");
	if (snmpapi_dll == NULL) {
		DEBUG_LOG(("Failed to load SNMPAPI.DLL\n"));
		FreeLibrary(mib_ii_dll);
		return(false);
	}

	/*
	** Get the function pointers into the .dll
	*/
	SnmpExtensionInitPtr = (int (__stdcall *)(unsigned long,void ** ,AsnObjectIdentifier *)) GetProcAddress(mib_ii_dll, "SnmpExtensionInit");
	SnmpExtensionQueryPtr = (int (__stdcall *)(unsigned char,SnmpVarBindList *,long *,long *)) GetProcAddress(mib_ii_dll, "SnmpExtensionQuery");
	SnmpUtilMemAllocPtr = (void *(__stdcall *)(unsigned long)) GetProcAddress(snmpapi_dll, "SnmpUtilMemAlloc");
	SnmpUtilMemFreePtr = (void (__stdcall *)(void *)) GetProcAddress(snmpapi_dll, "SnmpUtilMemFree");
	if (SnmpExtensionInitPtr == NULL || SnmpExtensionQueryPtr == NULL || SnmpUtilMemAllocPtr == NULL || SnmpUtilMemFreePtr == NULL) {
		DEBUG_LOG(("Failed to get proc addresses for linked functions\n"));
		FreeLibrary(snmpapi_dll);
		FreeLibrary(mib_ii_dll);
		return(false);
	}


	RFC1157VarBindList *bind_list_ptr = (RFC1157VarBindList *) SnmpUtilMemAllocPtr(sizeof(RFC1157VarBindList));
	RFC1157VarBind *bind_ptr = (RFC1157VarBind *) SnmpUtilMemAllocPtr(sizeof(RFC1157VarBind));

	/*
	** OK, here we go. Try to initialise the .dll
	*/
	DEBUG_LOG(("About to init INETMIB1.DLL\n"));
	int ok = SnmpExtensionInitPtr(GetCurrentTime(), &trap_handle, &first_supported_region);

	if (!ok) {
		/*
		** Aw crap.
		*/
		DEBUG_LOG(("Failed to init the .dll\n"));
		SnmpUtilMemFreePtr(bind_list_ptr);
		SnmpUtilMemFreePtr(bind_ptr);
		FreeLibrary(snmpapi_dll);
		FreeLibrary(mib_ii_dll);
		return(false);
	}

	/*
	** Name of mib_ii object we want to query. See RFC 1213.
	**
	** iso.org.dod.internet.mgmt.mib-2.tcp.tcpConnTable.TcpConnEntry.tcpConnState
	**  1   3   6      1      2     1   6        13          1             1
	*/
	unsigned int mib_ii_name[] = {1,3,6,1,2,1,6,13,1,1};
	unsigned int *mib_ii_name_ptr = (unsigned int *) SnmpUtilMemAllocPtr(sizeof(mib_ii_name));
	memcpy(mib_ii_name_ptr, mib_ii_name, sizeof(mib_ii_name));

	/*
	** Get the index of the conn entry data.
	*/
	conn_entry_type_index = ARRAY_SIZE(mib_ii_name) - 1;

	/*
	** Set up the bind list.
	*/
	bind_ptr->name.idLength = ARRAY_SIZE(mib_ii_name);
	bind_ptr->name.ids = mib_ii_name;
	bind_list_ptr->list = bind_ptr;
	bind_list_ptr->len = 1;


	/*
	** We start with the tcpConnLocalAddress field.
	*/
	last_field = 1;

	/*
	** First connection.
	*/
	index = 0;

	/*
	** Suck out that tcp connection info....
	*/
	while (true) {

		if (!SnmpExtensionQueryPtr(SNMP_PDU_GETNEXT, bind_list_ptr, &error_status, &error_index)) {
		//if (!SnmpExtensionQueryPtr(ASN_RFC1157_GETNEXTREQUEST, bind_list_ptr, &error_status, &error_index)) {
			DEBUG_LOG(("SnmpExtensionQuery returned false\n"));
			SnmpUtilMemFreePtr(bind_list_ptr);
			SnmpUtilMemFreePtr(bind_ptr);
			FreeLibrary(snmpapi_dll);
			FreeLibrary(mib_ii_dll);
			return(false);
		}

		/*
		** If this is something new we aren't looking for then we are done.
		*/
		if (bind_ptr->name.idLength < ARRAY_SIZE(mib_ii_name)) {
			break;
		}

		/*
		** Get the type of info we are looking at. See RFC1213.
		**
		** 1 = tcpConnState
		** 2 = tcpConnLocalAddress
		** 3 = tcpConnLocalPort
		** 4 = tcpConnRemAddress
		** 5 = tcpConnRemPort
		**
		** tcpConnState is one of the following...
		**
		**   1  closed
		**   2  listen
		**   3  synSent
		**   4  synReceived
		**   5  established
		**   6  finWait1
		**   7  finWait2
		**   8  closeWait
		**   9  lastAck
		**   10 closing
		**   11 timeWait
		**   12 deleteTCB
		*/
		conn_entry_type = bind_ptr->name.ids[conn_entry_type_index];

		if (last_field != conn_entry_type) {
			index = 0;
			last_field = conn_entry_type;
		}

		switch (conn_entry_type) {

			/*
			** 1. First field in the entry. Need to create a new connection info struct
			** here to store this connection in.
			*/
			case tcpConnState:
			{
				ConnInfoStruct new_conn;
				new_conn.State = bind_ptr->value.asnValue.number;
				connectionVector.push_back(new_conn);
				break;
			}

			/*
			** 2. Local address field.
			*/
			case tcpConnLocalAddress:
				DEBUG_ASSERTCRASH(index < connectionVector.size(), ("Bad connection index"));
				connectionVector[index].LocalIP = *((unsigned long*)bind_ptr->value.asnValue.address.stream);
				index++;
				break;

			/*
			** 3. Local port field.
			*/
			case tcpConnLocalPort:
				DEBUG_ASSERTCRASH(index < connectionVector.size(), ("Bad connection index"));
				connectionVector[index].LocalPort = bind_ptr->value.asnValue.number;
				//connectionVector[index]->LocalPort = ntohs(connectionVector[index]->LocalPort);
				index++;
				break;

			/*
			** 4. Remote address field.
			*/
			case tcpConnRemAddress:
				DEBUG_ASSERTCRASH(index < connectionVector.size(), ("Bad connection index"));
				connectionVector[index].RemoteIP = *((unsigned long*)bind_ptr->value.asnValue.address.stream);
				index++;
				break;

			/*
			** 5. Remote port field.
			*/
			case tcpConnRemPort:
				DEBUG_ASSERTCRASH(index < connectionVector.size(), ("Bad connection index"));
				connectionVector[index].RemotePort = bind_ptr->value.asnValue.number;
				//connectionVector[index]->RemotePort = ntohs(connectionVector[index]->RemotePort);
				index++;
				break;
		}
	}

	SnmpUtilMemFreePtr(bind_list_ptr);
	SnmpUtilMemFreePtr(bind_ptr);
	SnmpUtilMemFreePtr(mib_ii_name_ptr);

	DEBUG_LOG(("Got %d connections in list, parsing...\n", connectionVector.size()));

	/*
	** Right, we got the lot. Lets see if any of them have the same address as the chat
	** server we think we are talking to.
	*/
	found = false;
	for (Int i=0; i<connectionVector.size(); ++i) {
		ConnInfoStruct connection = connectionVector[i];

		temp = ntohl(connection.RemoteIP);
		memcpy(remoteAddress, (unsigned char*)&temp, 4);

		/*
		** See if this connection has the same address as our server.
		*/
		if (!found && memcmp(remoteAddress, serverAddress, 4) == 0) {
			DEBUG_LOG(("Found connection with same remote address as server\n"));

			if (serverPort == 0 || serverPort == (unsigned int)connection.RemotePort) {

				DEBUG_LOG(("Connection has same port\n"));
				/*
				** Make sure the connection is current.
				*/
				if (connection.State == ESTABLISHED) {
					DEBUG_LOG(("Connection is ESTABLISHED\n"));
					localIP = connection.LocalIP;
					found = true;
				} else {
					DEBUG_LOG(("Connection is not ESTABLISHED - skipping\n"));
				}
			} else {
				DEBUG_LOG(("Connection has different port. Port is %d, looking for %d\n", connection.RemotePort, serverPort));
			}
		}
	}

	if (found) {
		DEBUG_LOG(("Using address 0x%8.8X to talk to chat server\n", localIP));
	}

	FreeLibrary(snmpapi_dll);
	FreeLibrary(mib_ii_dll);
	return(found);
#endif 
	return false;
}


// GameSpyGameInfo ----------------------------------------

GameSpyGameInfo::GameSpyGameInfo()
{
	m_isQM = FALSE;
	m_hasBeenQueried = FALSE;
	for (Int i = 0; i< MAX_SLOTS; ++i)
		setSlotPointer(i, &m_GameSpySlot[i]);

	UnsignedInt localIP;
	if (GetLocalChatConnectionAddress("peerchat.gamespy.com", 6667, localIP))
	{
		localIP = ntohl(localIP); // The IP returned from GetLocalChatConnectionAddress is in network byte order.
		setLocalIP(localIP);
	}
	else
	{
		setLocalIP(0);
	}
	m_server = NULL;
	m_transport = NULL;
}

// Misc game-related functionality --------------------

void GameSpyStartGame( void )
{
	if (TheGameSpyGame)
	{
		int i;

		int numUsers = 0;
		for (i=0; i<MAX_SLOTS; ++i)
		{
			GameSlot *slot = TheGameSpyGame->getSlot(i);
			if (slot && slot->isOccupied())
				numUsers++;
		}

		if (numUsers < 2)
		{
			if (TheGameSpyGame->amIHost())
			{
				UnicodeString text;
				text.format(TheGameText->fetch("LAN:NeedMorePlayers"),numUsers);
				TheGameSpyInfo->addText(text, GSCOLOR_DEFAULT, NULL);
			}
			return;
		}

		TheGameSpyGame->startGame(0);
	}
}

void GameSpyLaunchGame( void )
{
	if (TheGameSpyGame)
	{

		// Set up the game network
		AsciiString user;
		AsciiString userList;
		DEBUG_ASSERTCRASH(TheNetwork == NULL, ("For some reason TheNetwork isn't NULL at the start of this game.  Better look into that."));

		if (TheNetwork != NULL) {
			delete TheNetwork;
			TheNetwork = NULL;
		}

		// Time to initialize TheNetwork for this game.
		TheNetwork = NetworkInterface::createNetwork();
		TheNetwork->init();
		/*
		if (!TheGameSpyGame->amIHost())
			TheNetwork->setLocalAddress((207<<24) | (138<<16) | (47<<8) | 15, 8088);
		else
		*/
		TheNetwork->setLocalAddress(TheGameSpyGame->getLocalIP(), TheNAT->getSlotPort(TheGameSpyGame->getLocalSlotNum()));
		TheNetwork->attachTransport(TheNAT->getTransport());

		user = TheGameSpyInfo->getLocalName();
		for (Int i=0; i<MAX_SLOTS; ++i)
		{
			GameSlot *slot = TheGameSpyGame->getSlot(i);
			if (!slot)
			{
				DEBUG_CRASH(("No GameSlot[%d]!", i));
				delete TheNetwork;
				TheNetwork = NULL;
				return;
			}

//			UnsignedInt ip = htonl(slot->getIP());
			UnsignedInt ip = slot->getIP();
			AsciiString tmpUserName;
			tmpUserName.translate(slot->getName());
			if (ip)
			{
				/*
				if (i == 1)
				{
					user.format(",%s@207.138.47.15:8088", tmpUserName.str());
				}
				else
				*/
				{
				user.format(",%s@%d.%d.%d.%d:%d", tmpUserName.str(),
					((ip & 0xff000000) >> 24),
					((ip & 0xff0000) >> 16),
					((ip & 0xff00) >> 8),
					((ip & 0xff)),
					TheNAT->getSlotPort(i)
					);
				}
				userList.concat(user);
			}
		}
		userList.trim();

		TheNetwork->parseUserList(TheGameSpyGame);

		// shutdown the top, but do not pop it off the stack
//		TheShell->hideShell();
		// setup the Global Data with the Map and Seed
		TheGlobalData->m_pendingFile = TheGameSpyGame->getMap();

		if (TheGameLogic->isInGame()) {
			TheGameLogic->clearGameData();
		}
		// send a message to the logic for a new game
		GameMessage *msg = TheMessageStream->appendMessage( GameMessage::MSG_NEW_GAME );
		msg->appendIntegerArgument(GAME_INTERNET);

		TheGlobalData->m_useFpsLimit = false;

		// Set the random seed
		InitGameLogicRandom( TheGameSpyGame->getSeed() );
		DEBUG_LOG(("InitGameLogicRandom( %d )\n", TheGameSpyGame->getSeed()));

		if (TheNAT != NULL) {
			delete TheNAT;
			TheNAT = NULL;
		}
	}
}

void GameSpyGameInfo::init( void )
{
	GameInfo::init();

	m_hasBeenQueried = false;
}

void GameSpyGameInfo::resetAccepted( void )
{
	GameInfo::resetAccepted();

	if (m_hasBeenQueried && amIHost())
	{
		// ANCIENTMUNKEE peerStateChanged(TheGameSpyChat->getPeer());
		m_hasBeenQueried = false;
		DEBUG_LOG(("resetAccepted() called peerStateChange()\n"));
	}
}

Int GameSpyGameInfo::getLocalSlotNum( void ) const
{
	DEBUG_ASSERTCRASH(m_inGame, ("Looking for local game slot while not in game"));
	if (!m_inGame)
		return -1;

	AsciiString localName = TheGameSpyInfo->getLocalName();

	for (Int i=0; i<MAX_SLOTS; ++i)
	{
		const GameSlot *slot = getConstSlot(i);
		if (slot == NULL) {
			continue;
		}
		if (slot->isPlayer(localName))
			return i;
	}
	return -1;
}

void GameSpyGameInfo::gotGOACall( void )
{
	DEBUG_LOG(("gotGOACall()\n"));
	m_hasBeenQueried = true;
}

void GameSpyGameInfo::startGame(Int gameID)
{
	DEBUG_LOG(("GameSpyGameInfo::startGame - game id = %d\n", gameID));
	DEBUG_ASSERTCRASH(m_transport == NULL, ("m_transport is not NULL when it should be"));
	DEBUG_ASSERTCRASH(TheNAT == NULL, ("TheNAT is not NULL when it should be"));

	// fill in GS-specific info
	for (Int i=0; i<MAX_SLOTS; ++i)
	{
		if (m_GameSpySlot[i].isHuman())
		{
			AsciiString gsName;
			gsName.translate( m_GameSpySlot[i].getName() );
			m_GameSpySlot[i].setLoginName( gsName );

			PlayerInfoMap *pInfoMap = TheGameSpyInfo->getPlayerInfoMap();
			PlayerInfoMap::iterator it = pInfoMap->find(gsName);
			if (it != pInfoMap->end())
			{
				m_GameSpySlot[i].setProfileID(it->second.m_profileID);
				m_GameSpySlot[i].setLocale(it->second.m_locale);
			}
			else
			{
				DEBUG_CRASH(("No player info for %s", gsName.str()));
			}
		}
	}

	if (TheNAT != NULL) {
		delete TheNAT;
		TheNAT = NULL;
	}
	TheNAT = NEW NAT();
	TheNAT->attachSlotList(m_slot, getLocalSlotNum(), m_localIP);
	TheNAT->establishConnectionPaths();
}

AsciiString GameSpyGameInfo::generateGameResultsPacket( void )
{
	Int i;
	Int endFrame = TheVictoryConditions->getEndFrame();
	Int localSlotNum = getLocalSlotNum();
	//GameSlot *localSlot = getSlot(localSlotNum);
	Bool sawGameEnd = (endFrame > 0);// && localSlot->lastFrameInGame() <= endFrame);
	Int winningTeam = -1;
	Int numPlayers = 0;
	Int numTeamsAtGameEnd = 0;
	Int lastTeamAtGameEnd = -1;
	for (i=0; i<MAX_SLOTS; ++i)
	{
		AsciiString playerName;
		playerName.format("player%d", i);
		Player *p = ThePlayerList->findPlayerWithNameKey(NAMEKEY(playerName));
		if (p)
		{
			++numPlayers;
			if (TheVictoryConditions->hasAchievedVictory(p))
			{
				winningTeam = getSlot(i)->getTeamNumber();
			}

			// check if he lasted
			GameSlot *slot = getSlot(i);
			if (!slot->disconnected())
			{
				if (slot->getTeamNumber() != lastTeamAtGameEnd || numTeamsAtGameEnd == 0)
				{
					lastTeamAtGameEnd = slot->getTeamNumber();
					++numTeamsAtGameEnd;
				}
			}
		}
	}

	AsciiString results;
	results.format("seed=%d,slotNum=%d,sawDesync=%d,sawGameEnd=%d,winningTeam=%d,disconEnd=%d,duration=%d,numPlayers=%d,isQM=%d",
		getSeed(), localSlotNum, TheNetwork->sawCRCMismatch(), sawGameEnd, winningTeam, (numTeamsAtGameEnd != 0),
		endFrame, numPlayers, m_isQM);

	Int playerID = 0;
	for (i=0; i<MAX_SLOTS; ++i)
	{
		AsciiString playerName;
		playerName.format("player%d", i);
		Player *p = ThePlayerList->findPlayerWithNameKey(NAMEKEY(playerName));
		if (p)
		{
			GameSpyGameSlot *slot = &(m_GameSpySlot[i]);
			ScoreKeeper *keeper = p->getScoreKeeper();
			AsciiString playerName = slot->getLoginName();
			Int gsPlayerID = slot->getProfileID();
			AsciiString locale = slot->getLocale();
			Int fps = TheNetwork->getAverageFPS();
			Int unitsKilled = keeper->getTotalUnitsDestroyed();
			Int unitsLost = keeper->getTotalUnitsLost();
			Int unitsBuilt = keeper->getTotalUnitsBuilt();
			Int buildingsKilled = keeper->getTotalBuildingsDestroyed();
			Int buildingsLost = keeper->getTotalBuildingsLost();
			Int buildingsBuilt = keeper->getTotalBuildingsBuilt();
			Int earnings = keeper->getTotalMoneyEarned();
			Int techCaptured = keeper->getTotalTechBuildingsCaptured();
			Bool disconnected = slot->disconnected();

			AsciiString playerStr;
			playerStr.format(",player%d=%s,playerID%d=%d,locale%d=%s",
				playerID, playerName.str(), playerID, gsPlayerID, playerID, locale.str());
			results.concat(playerStr);
			playerStr.format(",unitsKilled%d=%d,unitsLost%d=%d,unitsBuilt%d=%d",
				playerID, unitsKilled, playerID, unitsLost, playerID, unitsBuilt);
			results.concat(playerStr);
			playerStr.format(",buildingsKilled%d=%d,buildingsLost%d=%d,buildingsBuilt%d=%d",
				playerID, buildingsKilled, playerID, buildingsLost, playerID, buildingsBuilt);
			results.concat(playerStr);
			playerStr.format(",fps%d=%d,cash%d=%d,capturedTech%d=%d,discon%d=%d",
				playerID, fps, playerID, earnings, playerID, techCaptured, playerID, disconnected);
			results.concat(playerStr);

			++playerID;
		}
	}

	// Add a trailing size value (so the server can ensure it got the entire packet)
	int resultsLen = results.getLength()+10;
	AsciiString tail;
	tail.format("%10.10d", resultsLen);
	results.concat(tail);

	return results;
}

