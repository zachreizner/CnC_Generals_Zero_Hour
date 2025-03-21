#ifndef __GAME_SPY_H__
#define __GAME_SPY_H__

#define GP_NICK_LEN 16
#define GP_EMAIL_LEN 64
#define GP_PASSWORD_LEN 64
#define GP_STATUS_STRING_LEN 32
#define GP_LOCATION_STRING_LEN 128
#define GP_COUNTRYCODE_LEN 2
#define GP_REASON_LEN 16

#define GP_ONLINE 0
#define GP_OFFLINE 1

#define GP_RECV_BUDDY_MESSAGE 0
#define GP_RECV_BUDDY_REQUEST 1
#define GP_RECV_BUDDY_STATUS 2
#define GP_RECV_GAME_INVITE 3

#define PEER_FLAG_OP 0x1

#define PEERJoinSuccess 0
#define PEERFullRoom 1
#define PEERInviteOnlyRoom 2
#define PEERBannedFromRoom 3
#define PEERBadPassword 4
#define PEERAlreadyInRoom 5
#define PEERNoConnection 6

#define PEER_CLEAR 0
#define PEER_ADD 1
#define PEER_UPDATE 2
#define PEER_REMOVE 3

#define PEERTrue true

typedef uint32_t GPResult;
typedef uint32_t GPErrorCode;
typedef int32_t GPenum;
typedef uint64_t GPProfile;

#endif
