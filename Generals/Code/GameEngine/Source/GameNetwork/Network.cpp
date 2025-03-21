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

// FILE: Network.cpp ////////////////////////////////////////////////////
// Implementation of Network singleton
// Author: Matthew D. Campbell, July 2001
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES ////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/GameEngine.h"
#include "Common/MessageStream.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "GameNetwork/NetworkInterface.h"
#include "GameNetwork/udp.h"
#include "GameNetwork/Transport.h"
#include "strtok_r.h"
#include "GameClient/Shell.h"
#include "Common/CRCDebug.h"
#include "GameLogic/GameLogic.h"

#include "Common/RandomValue.h"


#include "GameLogic/ScriptActions.h"
#include "GameLogic/ScriptEngine.h"
#include "Common/Recorder.h"
#include "GameClient/MessageBox.h"

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

#if defined(DEBUG_CRC)
Int NET_CRC_INTERVAL = 1;
#else
Int NET_CRC_INTERVAL = 100;
#endif

// DEFINES ////////////////////////////////////////////////////////////////////

#define RESEND_INTERVAL 1

// PRIVATE TYPES //////////////////////////////////////////////////////////////

/**
 * Connection message - encapsulating info kept by the connection layer about each
 * packet.  These structs make up the in/out buffers at the connection layer.
 */
#pragma pack(push, 1)
struct ConnectionMessage
{
	Int id;
	NetMessageFlags flags;
	UnsignedByte data[MAX_MESSAGE_LEN];
	time_t lastSendTime;
	Int retries;
	Int length;
};
#pragma pack(pop)

static const int CmdMsgLen = 6; //< Minimum size of a command packet (Int + Unsigned Short)

// PRIVATE DATA ///////////////////////////////////////////////////////////////



// PUBLIC DATA ////////////////////////////////////////////////////////////////

/// The Network singleton instance
NetworkInterface *TheNetwork = NULL;

// PRIVATE PROTOTYPES /////////////////////////////////////////////////////////

/**
 * The Network class is used to instantiate a singleton which 
 * implements the interface to all Network operations such as message stream processing and network communications.
 */
class Network : public NetworkInterface
{
public:
	//---------------------------------------------------------------------------------------
	// Setup / Teardown functions
	Network();
	~Network();
	void init( void );																				///< Initialize or re-initialize the instance
	void reset( void );																				///< Reinitialize the network
	void update( void );																			///< Process command list
	void liteupdate( void );																	///< Do a lightweight update to send packets and pass messages.
	Bool deinit( void );																			///< Shutdown connections, release memory

	void setLocalAddress(UnsignedInt ip, UnsignedInt port);
	inline UnsignedInt getRunAhead(void) { return m_runAhead; }
	inline UnsignedInt getFrameRate(void) { return m_frameRate; }
	UnsignedInt getPacketArrivalCushion(void);								///< Returns the smallest packet arrival cushion since this was last called.
	Bool isFrameDataReady( void );
	void parseUserList( const GameInfo *game );
	void startGame(void);																			///< Sets the network game frame counter to -1

	void sendChat(UnicodeString text, Int playerMask);
	void sendDisconnectChat(UnicodeString text);

	void sendFile(AsciiString path, UnsignedByte playerMask, UnsignedShort commandID);
	UnsignedShort sendFileAnnounce(AsciiString path, UnsignedByte playerMask);
	Int getFileTransferProgress(Int playerID, AsciiString path);
	Bool areAllQueuesEmpty(void);

	void quitGame();
	virtual void selfDestructPlayer(Int index);


	void voteForPlayerDisconnect(Int slot);
	virtual Bool isPacketRouter( void );

	// Bandwidth metrics
	Real getIncomingBytesPerSecond( void );
	Real getIncomingPacketsPerSecond( void );
	Real getOutgoingBytesPerSecond( void );
	Real getOutgoingPacketsPerSecond( void );
	Real getUnknownBytesPerSecond( void );
	Real getUnknownPacketsPerSecond( void );

	// Multiplayer Load Progress Functions
	void updateLoadProgress( Int percent );
	void loadProgressComplete( void );
	void sendTimeOutGameStart( void );

#if defined(_INTERNAL) || defined(_DEBUG)
	// Disconnect screen testing
	virtual void toggleNetworkOn();
#endif

	// Exposing some info contained in the Connection Manager
	UnsignedInt getLocalPlayerID( void );
	UnicodeString getPlayerName(Int playerNum);
	Int getNumPlayers(void );

	Int getAverageFPS() { return m_conMgr->getAverageFPS(); }
	Int getSlotAverageFPS(Int slot);

	void attachTransport(Transport *transport);
	void initTransport();

	void setSawCRCMismatch( void );
	Bool sawCRCMismatch( void ) { return m_sawCRCMismatch; }
	Bool isPlayerConnected( Int playerID );

	void notifyOthersOfCurrentFrame();														///< Tells all the other players what frame we are on.
	void notifyOthersOfNewFrame(UnsignedInt frame);								///< Tells all the other players that we are on a new frame.

	Int  getExecutionFrame();																			///< Returns the next valid frame for simultaneous command execution.

	// For disconnect blame assignment
	UnsignedInt getPingFrame();
	Int getPingsSent();
	Int getPingsRecieved();

protected:
	void GetCommandsFromCommandList();														///< Remove commands from TheCommandList and put them on the Network command list.
	void SendCommandsToConnectionManager();												///< Send the new commands to the ConnectionManager
	Bool AllCommandsReady(UnsignedInt frame);											///< Do we have all the commands for the given frame?
	void RelayCommandsToCommandList(UnsignedInt frame);						///< Put the commands for the given frame onto TheCommandList.
	Bool isTransferCommand(GameMessage *msg);											///< Is this a command that needs to be transfered to the other clients?
	Bool processCommand(GameMessage *msg);												///< Whatever needs to be done as a result of this command, do it now.
	void processFrameSynchronizedNetCommand(NetCommandRef *msg);	///< If there is a network command that needs to be executed at the same frame number on all clients, it happens here.
	void processRunAheadCommand(NetRunAheadCommandMsg *msg);			///< Do what needs to be done when we get a new run ahead command.
	void processDestroyPlayerCommand(NetDestroyPlayerCommandMsg *msg);	///< Do what needs to be done when we need to destroy a player.
	void endOfGameCheck();																				///< Checks to see if its ok to leave this game.  If it is, send the apropriate command to the game logic.
	Bool timeForNewFrame();

	ConnectionManager *m_conMgr;																	///< The connection manager object

	UnsignedInt m_lastFrame;																			///< The last game logic frame that was processed.

	NetLocalStatus m_localStatus;															///< My local status as a player in this game.

	Int m_runAhead;																						///< The current run ahead of the game.
	Int m_frameRate;
	Int m_lastExecutionFrame;																	///< The highest frame number that a command could have been executed on.
	Int m_lastFrameCompleted;
	Bool m_didSelfSlug;
	__int64 m_perfCountFreq;														///< The frequency of the performance counter.

	__int64 m_nextFrameTime;														///< When did we execute the last frame?  For slugging the GameLogic...

	Bool m_frameDataReady;																		///< Is the frame data for the next frame ready to be executed by TheGameLogic?

	// CRC info
	Bool m_checkCRCsThisFrame;
	Bool m_sawCRCMismatch;
	std::vector<UnsignedInt> m_CRC[MAX_SLOTS];
	std::list<Int> m_playersToDisconnect;
	GameWindow *m_messageWindow;

#if defined(_DEBUG) || defined(_INTERNAL)
	Bool m_networkOn;
#endif
};

UnsignedInt Network::getPingFrame()
{
	return (m_conMgr)?m_conMgr->getPingFrame():0;
}

Int Network::getPingsSent()
{
	return (m_conMgr)?m_conMgr->getPingsSent():0;
}

Int Network::getPingsRecieved()
{
	return (m_conMgr)?m_conMgr->getPingsRecieved():0;
}

Bool Network::isPlayerConnected( Int playerID ) {
	if (playerID == getLocalPlayerID()) {
		return m_localStatus == NETLOCALSTATUS_INGAME || m_localStatus == NETLOCALSTATUS_LEAVING;
	}
	return m_conMgr->isPlayerConnected(playerID);
}


// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * This creates a network object and returns it.
 */
NetworkInterface *NetworkInterface::createNetwork() 
{
	return NEW Network;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * The constructor.
 */
Network::Network()
{
	//Added By Sadullah Nader
	//Initializations inserted
	m_checkCRCsThisFrame = FALSE;
	m_didSelfSlug = FALSE;
	m_frameDataReady = FALSE;
	m_sawCRCMismatch = FALSE;
	//
	
	m_conMgr = NULL;
	m_messageWindow = NULL;

#if defined(_DEBUG) || defined(_INTERNAL)
	m_networkOn = TRUE;
#endif
}

/**
 * The destructor.
 */
Network::~Network()
{
	deinit();
}

/**
 * This basically releases all the memory.
 */
Bool Network::deinit( void )
{
	if (m_conMgr)
	{
		m_conMgr->destroyGameMessages();
		delete m_conMgr;
		m_conMgr = NULL;
	}
	if (m_messageWindow) {
		TheWindowManager->winDestroy(m_messageWindow);
		m_messageWindow = NULL;
	}

	return true;
}

/**
 * Takes the network back to the initial state.
 */
void Network::reset() {
	init();
}

/**
 * Initializes all the network subsystems.
 */
void Network::init()
{
	if (!deinit())
	{
		DEBUG_LOG(("Could not deinit network prior to init!\n"));
		return;
	}

	m_conMgr = NEW ConnectionManager;
	m_conMgr->init();

	m_lastFrame = 0;
	m_runAhead = min(max(30, MIN_RUNAHEAD), MAX_FRAMES_AHEAD/2); ///< @todo: don't hard-code the run-ahead.
	m_frameRate = 30;
	m_lastExecutionFrame = m_runAhead - 1; // subtract 1 since we're starting on frame 0
	m_lastFrameCompleted = m_runAhead - 1; // subtract 1 since we're starting on frame 0
	m_frameDataReady = FALSE;
	m_didSelfSlug = FALSE;

	m_localStatus = NETLOCALSTATUS_PREGAME;

	QueryPerformanceFrequency((LARGE_INTEGER *)&m_perfCountFreq);
	m_nextFrameTime = 0;
	m_sawCRCMismatch = FALSE;
	m_checkCRCsThisFrame = FALSE;

	DEBUG_LOG(("Network timing values:\n"));
	DEBUG_LOG(("NetworkFPSHistoryLength: %d\n", TheGlobalData->m_networkFPSHistoryLength));
	DEBUG_LOG(("NetworkLatencyHistoryLength: %d\n", TheGlobalData->m_networkLatencyHistoryLength));
	DEBUG_LOG(("NetworkRunAheadMetricsTime: %d\n", TheGlobalData->m_networkRunAheadMetricsTime));
	DEBUG_LOG(("NetworkCushionHistoryLength: %d\n", TheGlobalData->m_networkCushionHistoryLength));
	DEBUG_LOG(("NetworkRunAheadSlack: %d\n", TheGlobalData->m_networkRunAheadSlack));
	DEBUG_LOG(("NetworkKeepAliveDelay: %d\n", TheGlobalData->m_networkKeepAliveDelay));
	DEBUG_LOG(("NetworkDisconnectTime: %d\n", TheGlobalData->m_networkDisconnectTime));
	DEBUG_LOG(("NetworkPlayerTimeoutTime: %d\n", TheGlobalData->m_networkPlayerTimeoutTime));
	DEBUG_LOG(("NetworkDisconnectScreenNotifyTime: %d\n", TheGlobalData->m_networkDisconnectScreenNotifyTime));
	DEBUG_LOG(("Other network stuff:\n"));
	DEBUG_LOG(("FRAME_DATA_LENGTH = %d\n", FRAME_DATA_LENGTH));
	DEBUG_LOG(("FRAMES_TO_KEEP = %d\n", FRAMES_TO_KEEP));


#if defined(_DEBUG) || defined(_INTERNAL)
	m_networkOn = TRUE;
#endif

	return;
}

void Network::setSawCRCMismatch( void )
{
	m_sawCRCMismatch = TRUE;

	TheScriptActions->closeWindows( TRUE );
	m_messageWindow = TheWindowManager->winCreateFromScript("Menus/CRCMismatch.wnd");
	TheScriptEngine->startEndGameTimer();

	TheRecorder->logCRCMismatch();

	// dump GameLogic random seed
	DEBUG_LOG(("GameLogic frame = %d\n", TheGameLogic->getFrame()));
	DEBUG_LOG(("GetGameLogicRandomSeedCRC() = %d\n", GetGameLogicRandomSeedCRC()));

	// dump CRCs
	{
		DEBUG_LOG(("--- GameState Dump ---\n"));
#ifdef DEBUG_CRC
		outputCRCDumpLines();
#endif
		DEBUG_LOG(("------ End Dump ------\n"));
	}
	{
		DEBUG_LOG(("--- DebugInfo Dump ---\n"));
#ifdef DEBUG_CRC
		outputCRCDebugLines();
#endif
		DEBUG_LOG(("------ End Dump ------\n"));
	}
}

/**
 * Take a user list and build the connection queues and player lists and stuff like that.
 */
void Network::parseUserList( const GameInfo *game )
{
	if (!game)
	{
		DEBUG_LOG(("FAILED parseUserList with a NULL game\n"));
		return;
	}

	m_conMgr->parseUserList(game);

	// Now that we have the players in this game, we need to reset the FrameData stuff.
	m_conMgr->destroyGameMessages();
	m_conMgr->zeroFrames(1, m_runAhead-1); ///< we zero out m_runAhead frames +1 because the game actually starts at frame 1.
}

/**
 * Guess what, we're starting a game!
 */
void Network::startGame() {
}

/**
 * Tell the network which ip address the user has chosen to use.  Well ok, they probably didn't choose
 * it explicitly, but regardless, this is the one we're going to use.
 */
void Network::setLocalAddress(UnsignedInt ip, UnsignedInt port) {
	DEBUG_ASSERTCRASH(m_conMgr != NULL, ("Connection manager does not exist."));
	if (m_conMgr != NULL) {
		m_conMgr->setLocalAddress(ip, port);
	}
}

/**
 * Tell the network to initialize the transport object
 */
void Network::initTransport() {
	DEBUG_ASSERTCRASH(m_conMgr != NULL, ("Connection manager does not exist."));
	if (m_conMgr != NULL) {
		m_conMgr->initTransport();
	}
}

void Network::attachTransport(Transport *transport) {
	DEBUG_ASSERTCRASH(m_conMgr != NULL, ("Connection manager does not exist."));
	if (m_conMgr != NULL) {
		m_conMgr->attachTransport(transport);
	}
}

/**
 * Does this command need to be transfered to the other game clients?
 */
Bool Network::isTransferCommand(GameMessage *msg) {
	if ((msg != NULL) && ((msg->getType() > GameMessage::MSG_BEGIN_NETWORK_MESSAGES) && (msg->getType() < GameMessage::MSG_END_NETWORK_MESSAGES))) {
		return TRUE;
	}
	return FALSE;
}

/**
 * Take commands from TheCommandList and give them to the connection manager for transport.
 */
void Network::GetCommandsFromCommandList() {
	GameMessage *msg = TheCommandList->getFirstMessage();
	GameMessage *next = NULL;
	while (msg != NULL) {
		next = msg->next();
		if (isTransferCommand(msg)) { // Is this something we should be sending to the other players?
			if (m_localStatus == NETLOCALSTATUS_INGAME) {
				m_conMgr->sendLocalGameMessage(msg, getExecutionFrame());
			}
			TheCommandList->removeMessage(msg); // This does not destroy msg's prev and next pointers, so they should still be valid.
			msg->deleteInstance();
		} else {
			if (processCommand(msg)) {
				TheCommandList->removeMessage(msg);
				msg->deleteInstance();
			}
		}
		msg = next;
	}
}

Int Network::getExecutionFrame() {
	Int logicFrame = TheGameLogic->getFrame() + m_runAhead;
	if (logicFrame > m_lastExecutionFrame) {
		m_lastExecutionFrame = logicFrame;
		return (logicFrame);
	}
	return m_lastExecutionFrame;
}

/**
 * This is where any processing that needs to be done for specific game messages.
 * Also check here to see if the logic frame number has changed to see if we need to
 * send our info for the last frame to the other players.
 * Return true if the message should be "eaten" by the network.
 */
Bool Network::processCommand(GameMessage *msg) 
{
	if ((m_lastFrame != TheGameLogic->getFrame()) || (m_localStatus == NETLOCALSTATUS_PREGAME)) {
		// If this is the start of a new game logic frame, then tell the connection manager that the last
		// frame is over and that it should now produce a frame info packet for the other players.

		if (m_localStatus == NETLOCALSTATUS_PREGAME) {
			// a sort-of-hack that prevents extraneous frames from being executed before the game actually starts.
			// Idealy this shouldn't be necessary, but I don't think its hurting anything by being here.
			if (TheGameLogic->getFrame() == 1) {
				m_localStatus = NETLOCALSTATUS_INGAME;
				NetCommandList *netcmdlist = m_conMgr->getFrameCommandList(0); // clear out frame 0 since we skipped it
				netcmdlist->deleteInstance();
			} else {
				return FALSE;
			}
		}

		// Only send frame info packets for frames where we are actually going to be in the game.
		// The reason is so we don't have to wait for frame info packets to be sent that aren't going to
		// matter anyways when we actually try to leave the game ourselves.
		if (m_localStatus == NETLOCALSTATUS_INGAME) {
			Int executionFrame = getExecutionFrame();

			// Send command counts for all the frames we can.
			for (Int i = m_lastFrameCompleted + 1; i < executionFrame; ++i) {
				m_conMgr->processFrameTick(i);
				//DEBUG_LOG(("Network::processCommand - calling processFrameTick for frame %d\n", i));
				m_lastFrameCompleted = i;
			}
		}

		//DEBUG_LOG(("Next Execution Frame - %d, last frame completed - %d\n", getExecutionFrame(), m_lastFrameCompleted));
		m_lastFrame = TheGameLogic->getFrame();
	}

	// Are we leaving the game?
	// This has to happen after the check to see if this is the start of a new logic frame.
	// The reason is that we have to send all the frame info packets necessary to get to the
	// frame where everyone else is going to see that we left.
	if ((msg->getType() == GameMessage::MSG_CLEAR_GAME_DATA) && (m_localStatus == NETLOCALSTATUS_INGAME)) {
		Int executionFrame = getExecutionFrame();
		DEBUG_LOG(("Network::processCommand - local player leaving, executionFrame = %d, player leaving on frame %d\n", executionFrame, executionFrame+1));

		m_conMgr->handleLocalPlayerLeaving(executionFrame+1);
		m_conMgr->processFrameTick(executionFrame); // This is the last command we will execute, so send the command count.
																								// Also, we are guaranteed not to send any more commands for this frame
																								// since the local status will change to "Leaving" so we don't have to
																								// worry about messing up the other players.
		m_conMgr->processFrameTick(executionFrame+1); // since we send it for executionFrame+1, we need to process both ticks
		m_lastFrameCompleted = executionFrame;
		DEBUG_LOG(("Network::processCommand - player leaving on frame %d\n", executionFrame));
		m_localStatus = NETLOCALSTATUS_LEAVING;
		return TRUE;
	}
	return FALSE;
}

/**
 * returns true if all the commands are ready for the given frame.
 */
Bool Network::AllCommandsReady(UnsignedInt frame) {
	if (m_conMgr == NULL) {
		return TRUE;
	}

	if (m_localStatus == NETLOCALSTATUS_PREGAME) {
		return TRUE;
	}

	if (m_localStatus == NETLOCALSTATUS_POSTGAME) {
		return TRUE;
	}

	return m_conMgr->allCommandsReady(frame);// && m_conMgr->allCRCsReady(frame);
}

/**
 * Take commands from the connection manager and put them on TheCommandList.
 * The commands need to be put on in the same order across all clients.
 */
void Network::RelayCommandsToCommandList(UnsignedInt frame) {
	if ((m_conMgr == NULL) || (m_localStatus == NETLOCALSTATUS_PREGAME)) {
		return;
	}
	m_checkCRCsThisFrame = FALSE;
	NetCommandList *netcmdlist = m_conMgr->getFrameCommandList(frame);
	NetCommandRef *msg = netcmdlist->getFirstMessage();
	while (msg != NULL) {
		NetCommandType cmdType = msg->getCommand()->getNetCommandType();
		if (cmdType == NETCOMMANDTYPE_GAMECOMMAND) {
			//DEBUG_LOG(("Network::RelayCommandsToCommandList - appending command %d of type %s to command list on frame %d\n", msg->getCommand()->getID(), ((NetGameCommandMsg *)msg->getCommand())->constructGameMessage()->getCommandAsAsciiString().str(), TheGameLogic->getFrame()));
			TheCommandList->appendMessage(((NetGameCommandMsg *)msg->getCommand())->constructGameMessage());
		} else {
			processFrameSynchronizedNetCommand(msg);
		}
		msg = msg->getNext();
	}

	for (std::list<Int>::iterator selfDestructIt = m_playersToDisconnect.begin(); selfDestructIt != m_playersToDisconnect.end(); ++selfDestructIt)
	{
		//Int playerToDisconnect = *selfDestructIt;
		//GameMessage *msg = newInstance(GameMessage)( GameMessage::MSG_SELF_DESTRUCT );
		//msg->friend_setPlayerIndex(playerToDisconnect);
		//msg->appendBooleanArgument(TRUE);
		//TheCommandList->appendMessage(msg);
	}
	m_playersToDisconnect.clear();

	netcmdlist->deleteInstance();
}

/**
 * This is where network commands that need to be executed on the same frame should be executed.
 */
void Network::processFrameSynchronizedNetCommand(NetCommandRef *msg) {
	NetCommandMsg *cmdMsg = msg->getCommand();
	if (cmdMsg->getNetCommandType() == NETCOMMANDTYPE_PLAYERLEAVE) {
		PlayerLeaveCode retval = m_conMgr->processPlayerLeave((NetPlayerLeaveCommandMsg *)cmdMsg);
		if (retval == PLAYERLEAVECODE_LOCAL) {
			DEBUG_LOG(("Network::processFrameSynchronizedNetCommand - Local player left the game on frame %d.\n", TheGameLogic->getFrame()));
			m_localStatus = NETLOCALSTATUS_LEFT;
		} else if (retval == PLAYERLEAVECODE_PACKETROUTER) {
			DEBUG_LOG(("Network::processFrameSynchronizedNetCommand - Packet router left the game on frame %d\n", TheGameLogic->getFrame()));
		} else {
			DEBUG_LOG(("Network::processFrameSynchronizedNetCommand - Client left the game on frame %d\n", TheGameLogic->getFrame()));
		}
	}
	else if (cmdMsg->getNetCommandType() == NETCOMMANDTYPE_RUNAHEAD) {
		NetRunAheadCommandMsg *netmsg = (NetRunAheadCommandMsg *)cmdMsg;
		processRunAheadCommand(netmsg);
		DEBUG_LOG_LEVEL(DEBUG_LEVEL_NET, ("command to set run ahead to %d and frame rate to %d on frame %d actually executed on frame %d\n", netmsg->getRunAhead(), netmsg->getFrameRate(), netmsg->getExecutionFrame(), TheGameLogic->getFrame()));
	}
	else if (cmdMsg->getNetCommandType() == NETCOMMANDTYPE_DESTROYPLAYER) {
		NetDestroyPlayerCommandMsg *netmsg = (NetDestroyPlayerCommandMsg *)cmdMsg;
		processDestroyPlayerCommand(netmsg);
		//DEBUG_LOG(("CRC command (%8.8X) on frame %d actually executed on frame %d\n", netmsg->getCRC(), netmsg->getExecutionFrame(), TheGameLogic->getFrame()));
	}
}

void Network::processRunAheadCommand(NetRunAheadCommandMsg *msg) {
	m_runAhead = msg->getRunAhead();
	m_frameRate = msg->getFrameRate();
	time_t frameGrouping = (1000 * m_runAhead) / m_frameRate; // number of miliseconds between packet sends
	frameGrouping = frameGrouping / 2; // since we only want the latency for one way to be a factor.
//	DEBUG_LOG(("Network::processRunAheadCommand - trying to set frame grouping to %d.  run ahead = %d, m_frameRate = %d\n", frameGrouping, m_runAhead, m_frameRate));
	if (frameGrouping < 1) {
		frameGrouping = 1; // Having a value less than 1 doesn't make sense.
	}
	if (frameGrouping > 500) {
		frameGrouping = 500; // Max of a half a second.
	}
	m_conMgr->setFrameGrouping(frameGrouping);
}

void Network::processDestroyPlayerCommand(NetDestroyPlayerCommandMsg *msg)
{
	UnsignedInt playerIndex = msg->getPlayerIndex();
	DEBUG_ASSERTCRASH(playerIndex < MAX_SLOTS, ("Bad player index"));
	if (playerIndex >= MAX_SLOTS)
		return;

	AsciiString playerName;
	playerName.format("player%d", playerIndex);
	Player *pPlayer = ThePlayerList->findPlayerWithNameKey(NAMEKEY(playerName));
	if (pPlayer)
	{
		GameMessage *msg = newInstance(GameMessage)(GameMessage::MSG_SELF_DESTRUCT);
		msg->appendBooleanArgument(FALSE);
		msg->friend_setPlayerIndex(pPlayer->getPlayerIndex());
		TheCommandList->appendMessage(msg);
	}

	DEBUG_LOG(("Saw DestroyPlayer from %d about %d for frame %d on frame %d\n", msg->getPlayerID(), msg->getPlayerIndex(),
		msg->getExecutionFrame(), TheGameLogic->getFrame()));
}

/**
 * Service queues, process message stream, etc
 */
void Network::update( void )
{
//
// 1. Take Commands off TheCommandList, hand them off to the ConnectionManager.
// 2. Call ConnectionManager->update;
// 3. Check to see if all the commands for the next frame are there.
// 4. If all commands are there, put that frame's commands on TheCommandList.
//
	m_frameDataReady = FALSE;

#if defined(_DEBUG) || defined(_INTERNAL)
	if (m_networkOn == FALSE) {
		return;
	}
#endif

	GetCommandsFromCommandList(); // Remove commands from TheCommandList and send them to the connection manager.
	if (m_conMgr != NULL) {
		if (m_localStatus == NETLOCALSTATUS_INGAME) {
			m_conMgr->updateRunAhead(m_runAhead, m_frameRate, m_didSelfSlug, getExecutionFrame());
			m_didSelfSlug = FALSE;
		}
		//m_conMgr->update(); // Do the priority thing, packetize thing. This also calls the Transport::update function.
									 // depacketizes the incoming packets and puts them on the Network command list.
	}

	liteupdate();

	if ((m_localStatus == NETLOCALSTATUS_LEFT)) {// || (m_localStatus == NETLOCALSTATUS_LEAVING)) {
		endOfGameCheck();
	}

	if (AllCommandsReady(TheGameLogic->getFrame())) { // If all the commands are ready for the next frame...
		m_conMgr->handleAllCommandsReady();
//		DEBUG_LOG(("Network::update - frame %d is ready\n", TheGameLogic->getFrame()));
		if (timeForNewFrame()) { // This needs to come after any other pre-frame execution checks as this changes the timing variables.
			RelayCommandsToCommandList(TheGameLogic->getFrame());	// Put the commands for the next frame on TheCommandList.
			m_frameDataReady = TRUE; // Tell the GameEngine to run the commands for the new frame.
		}
	}
}

void Network::liteupdate() {

#if defined(_DEBUG) || defined(_INTERNAL)
	if (m_networkOn == FALSE) {
		return;
	}
#endif

	if (m_conMgr != NULL) {
		if (m_localStatus == NETLOCALSTATUS_PREGAME) {
			m_conMgr->update(FALSE);
		} else {
			m_conMgr->update(TRUE);
		}
	}
}

void Network::endOfGameCheck() {
	if (m_conMgr != NULL) {
		if (m_conMgr->canILeave()) {
			m_conMgr->disconnectLocalPlayer();
			TheMessageStream->appendMessage(GameMessage::MSG_CLEAR_GAME_DATA);
			m_localStatus = NETLOCALSTATUS_POSTGAME;

			DEBUG_LOG(("Network::endOfGameCheck - about to show the shell\n"));
		}
#if defined(_DEBUG) || defined(_INTERNAL)
		else {
			m_conMgr->debugPrintConnectionCommands();
		}
#endif
	}
}

Bool Network::timeForNewFrame() {
	__int64 curTime;
	QueryPerformanceCounter((LARGE_INTEGER *)&curTime);
	__int64 frameDelay = m_perfCountFreq / m_frameRate;

	/*
	 * If we're pushing up against the edge of our run ahead, we should slow the framerate down a bit
	 * to avoid being frozen by spikes in network lag.  This will happen if another user's computer is
	 * running too far behind us, so we need to slow down to let them catch up.
	 */
	if (m_conMgr != NULL) {
		Real cushion = m_conMgr->getMinimumCushion();
		Real runAheadPercentage = m_runAhead * (TheGlobalData->m_networkRunAheadSlack / (Real)100.0); // If we are at least 50% into our slack, we need to slow down.
		if (cushion < runAheadPercentage) {
//			DEBUG_LOG(("Average cushion = %f, run ahead percentage = %f.  Adjusting frameDelay from %I64d to ", cushion, runAheadPercentage, frameDelay));
			frameDelay += frameDelay / 10; // temporarily decrease the frame rate by 20%.
//			DEBUG_LOG(("%I64d\n", frameDelay));
			m_didSelfSlug = TRUE;
//		} else {
//			DEBUG_LOG(("Average cushion = %f, run ahead percentage = %f\n", cushion, runAheadPercentage));
		}
	}

	// Check to see if we can run another frame.
	if (curTime >= m_nextFrameTime) {
//		DEBUG_LOG(("Allowing a new frame, frameDelay = %I64d, curTime - m_nextFrameTime = %I64d\n", frameDelay, curTime - m_nextFrameTime));

//		if (m_nextFrameTime + frameDelay < curTime) {
		if ((m_nextFrameTime + (2 * frameDelay)) < curTime) {
			// If we get too far behind on our framerate we need to reset the nextFrameTime thing.
			m_nextFrameTime = curTime;
//			DEBUG_LOG(("Initializing m_nextFrameTime to %I64d\n", m_nextFrameTime));
		} else {
			// Set the soonest possible starting time for the next frame.
			m_nextFrameTime += frameDelay;
//			DEBUG_LOG(("m_nextFrameTime = %I64d\n", m_nextFrameTime));
		}

		return TRUE;
	}
//	DEBUG_LOG(("Slowing down frame rate. frame rate = %d, frame delay = %I64d, curTime - m_nextFrameTime = %I64d\n", m_frameRate, frameDelay, curTime - m_nextFrameTime));
	return FALSE;
}

/**
 * Returns true if the game commands for the next frame have been put on the command list.
 */
Bool Network::isFrameDataReady() {
	return (m_frameDataReady || (m_localStatus == NETLOCALSTATUS_LEFT));
}

/**
 * returns the number of incoming bytes per second averaged over the last 30 sec.
 */
Real Network::getIncomingBytesPerSecond( void )
{
	if (m_conMgr)
		return m_conMgr->getIncomingBytesPerSecond();
	else
	  return 0.0;
}

/**
 * returns the number of incoming packets per second averaged over the last 30 sec.
 */
Real Network::getIncomingPacketsPerSecond( void )
{
	if (m_conMgr)
		return m_conMgr->getIncomingPacketsPerSecond();
	else
	  return 0.0;
}

/**
 * returns the number of outgoing bytes per second averaged over the last 30 sec.
 */
Real Network::getOutgoingBytesPerSecond( void )
{
	if (m_conMgr)
		return m_conMgr->getOutgoingBytesPerSecond();
	else
	  return 0.0;
}

/**
 * returns the number of outgoing packets per second averaged over the last 30 sec.
 */
Real Network::getOutgoingPacketsPerSecond( void )
{
	if (m_conMgr)
		return m_conMgr->getOutgoingPacketsPerSecond();
	else
	  return 0.0;
}

/**
 * returns the number of bytes received per second that are not from a generals client averaged over 30 sec.
 */
Real Network::getUnknownBytesPerSecond( void )
{
	if (m_conMgr)
		return m_conMgr->getUnknownBytesPerSecond();
	else
	  return 0.0;
}

/**
 * returns the number of packets received per second that are not from a generals client averaged over 30 sec.
 */
Real Network::getUnknownPacketsPerSecond( void )
{
	if (m_conMgr)
		return m_conMgr->getUnknownPacketsPerSecond();
	else
	  return 0.0;
}

/**
 * returns the smallest packet arrival cushion since this was last called.
 */
UnsignedInt Network::getPacketArrivalCushion( void )
{
	if (m_conMgr)
		return m_conMgr->getPacketArrivalCushion();
	else
		return 0;
}

/**
 * Sends a line of chat to the other players
 */
void Network::sendChat(UnicodeString text, Int playerMask) {
	Int executionFrame = getExecutionFrame();
	m_conMgr->sendChat(text, playerMask, executionFrame);
}

/**
 * Sends a line of chat to the other players using the disconnect manager.
 */
void Network::sendDisconnectChat(UnicodeString text) {
	m_conMgr->sendDisconnectChat(text);
}

// send a file.  woohoo.
void Network::sendFile(AsciiString path, UnsignedByte playerMask, UnsignedShort commandID)
{
	m_conMgr->sendFile(path, playerMask, commandID);
}

// send a file.  woohoo.
UnsignedShort Network::sendFileAnnounce(AsciiString path, UnsignedByte playerMask)
{
	return m_conMgr->sendFileAnnounce(path, playerMask);
}

Int Network::getFileTransferProgress(Int playerID, AsciiString path)
{
	return m_conMgr->getFileTransferProgress(playerID, path);
}

Bool Network::areAllQueuesEmpty(void)
{
	return m_conMgr->canILeave();
}

/**
 * Quit the game now.  This should only be called from the disconnect screen.
 */
void Network::quitGame() {
	if (m_conMgr != NULL) {
		m_conMgr->quitGame();
	}

	TheMessageStream->appendMessage(GameMessage::MSG_CLEAR_GAME_DATA);
	m_localStatus = NETLOCALSTATUS_POSTGAME;
	DEBUG_LOG(("Network::quitGame - quitting game..."));
}

void Network::selfDestructPlayer(Int index)
{
	m_playersToDisconnect.push_back(index);
}


Bool Network::isPacketRouter( void )
{
	return m_conMgr && m_conMgr->isPacketRouter();
}

/**
 * Register a vote towards a player being disconnected.
 */
void Network::voteForPlayerDisconnect(Int slot) {
	if (m_conMgr != NULL) {
		m_conMgr->voteForPlayerDisconnect(slot);
	}
}

void Network::updateLoadProgress( Int percent )
{
	if (m_conMgr != NULL) {
		m_conMgr->updateLoadProgress( percent );
	}
}

void Network::loadProgressComplete( void )
{
	if (m_conMgr != NULL) {
		m_conMgr->loadProgressComplete();
	}
}

void Network::sendTimeOutGameStart( void )
{
	if (m_conMgr != NULL) {
		m_conMgr->sendTimeOutGameStart();
	}
}


UnsignedInt Network::getLocalPlayerID()
{
if (m_conMgr != NULL) {
		return m_conMgr->getLocalPlayerID();
	}
	return 49;
}
UnicodeString Network::getPlayerName(Int playerNum)
{
	if (playerNum == m_conMgr->getLocalPlayerID()) {
		if (m_localStatus != NETLOCALSTATUS_INGAME) {
			return UnicodeString::TheEmptyString;
		}
	}
	if (m_conMgr != NULL) {
		return m_conMgr->getPlayerName( playerNum );
	}
	return UnicodeString::TheEmptyString;
}
Int Network::getNumPlayers()
{
	if (m_conMgr != NULL) {
		return m_conMgr->getNumPlayers();
	}
	return -1;
}

Int Network::getSlotAverageFPS(Int slot) {
	if (m_conMgr != NULL) {
		return m_conMgr->getSlotAverageFPS(slot);
	}
	return -1;
}

#if defined(_DEBUG) || defined(_INTERNAL)
void Network::toggleNetworkOn() {
	if (m_networkOn == TRUE) {
		m_networkOn = FALSE;
	} else {
		m_networkOn = TRUE;
	}
}
#endif

void Network::notifyOthersOfCurrentFrame() {
	if (m_conMgr != NULL) {
		m_conMgr->notifyOthersOfCurrentFrame(TheGameLogic->getFrame());
	}
}

void Network::notifyOthersOfNewFrame(UnsignedInt frame) {
	if (m_conMgr != NULL) {
		m_conMgr->notifyOthersOfNewFrame(frame);
	}
}
