/*

Copyright [2010] [Josko Nikolic]

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

CODE PORTED FROM THE ORIGINAL GHOST PROJECT: http://ghost.pwner.org/

*/

#ifndef AURA_GAMEPROTOCOL_H_
#define AURA_GAMEPROTOCOL_H_

#include <queue>
#include <stdint.h>
typedef std::vector<uint8_t> BYTEARRAY;

//
// CGameProtocol
//

#define W3GS_HEADER_CONSTANT      247

#define GAMETYPE_CUSTOM             1
#define GAMETYPE_BLIZZARD           9

#define PLAYERLEAVE_DISCONNECT      1
#define PLAYERLEAVE_LOST            7
#define PLAYERLEAVE_LOSTBUILDINGS   8
#define PLAYERLEAVE_WON             9
#define PLAYERLEAVE_DRAW           10
#define PLAYERLEAVE_OBSERVER       11
#define PLAYERLEAVE_LOBBY          13

#define REJECTJOIN_FULL             9
#define REJECTJOIN_STARTED         10
#define REJECTJOIN_WRONGPASSWORD   27

class CIncomingJoinPlayer;
class CIncomingAction;
class CIncomingChatPlayer;
class CIncomingMapSize;
class CGameSlot;

class CGameProtocol
{
public:
	enum Protocol
	{
		W3GS_PING_FROM_HOST = 1,  // 0x01
		W3GS_SLOTINFOJOIN = 4,  // 0x04
		W3GS_REJECTJOIN = 5,  // 0x05
		W3GS_PLAYERINFO = 6,  // 0x06
		W3GS_PLAYERLEAVE_OTHERS = 7,  // 0x07
		W3GS_GAMELOADED_OTHERS = 8,  // 0x08
		W3GS_SLOTINFO = 9,  // 0x09
		W3GS_COUNTDOWN_START = 10, // 0x0A
		W3GS_COUNTDOWN_END = 11, // 0x0B
		W3GS_INCOMING_ACTION = 12, // 0x0C
		W3GS_CHAT_FROM_HOST = 15, // 0x0F
		W3GS_START_LAG = 16, // 0x10
		W3GS_STOP_LAG = 17, // 0x11
		W3GS_HOST_KICK_PLAYER = 28, // 0x1C
		W3GS_REQJOIN = 30, // 0x1E
		W3GS_LEAVEGAME = 33, // 0x21
		W3GS_GAMELOADED_SELF = 35, // 0x23
		W3GS_OUTGOING_ACTION = 38, // 0x26
		W3GS_OUTGOING_KEEPALIVE = 39, // 0x27
		W3GS_CHAT_TO_HOST = 40, // 0x28
		W3GS_DROPREQ = 41, // 0x29
		W3GS_SEARCHGAME = 47, // 0x2F (UDP/LAN)
		W3GS_GAMEINFO = 48, // 0x30 (UDP/LAN)
		W3GS_CREATEGAME = 49, // 0x31 (UDP/LAN)
		W3GS_REFRESHGAME = 50, // 0x32 (UDP/LAN)
		W3GS_DECREATEGAME = 51, // 0x33 (UDP/LAN)
		W3GS_CHAT_OTHERS = 52, // 0x34
		W3GS_PING_FROM_OTHERS = 53, // 0x35
		W3GS_PONG_TO_OTHERS = 54, // 0x36
		W3GS_MAPCHECK = 61, // 0x3D
		W3GS_STARTDOWNLOAD = 63, // 0x3F
		W3GS_MAPSIZE = 66, // 0x42
		W3GS_MAPPART = 67, // 0x43
		W3GS_MAPPARTNOTOK = 69, // 0x45 - just a guess, received this packet after forgetting to send a crc in W3GS_MAPPART (f7 45 0a 00 01 02 01 00 00 00)
		W3GS_PONG_TO_HOST = 70, // 0x46
		W3GS_INCOMING_ACTION2 = 72  // 0x48 - received this packet when there are too many actions to fit in W3GS_INCOMING_ACTION
	};

	explicit CGameProtocol();
	~CGameProtocol();

	// receive functions

	CIncomingJoinPlayer *RECEIVE_W3GS_REQJOIN(const BYTEARRAY &data);
	uint32_t RECEIVE_W3GS_LEAVEGAME(const BYTEARRAY &data);
	bool RECEIVE_W3GS_GAMELOADED_SELF(const BYTEARRAY &data);
	CIncomingAction *RECEIVE_W3GS_OUTGOING_ACTION(const BYTEARRAY &data, uint8_t PID);
	uint32_t RECEIVE_W3GS_OUTGOING_KEEPALIVE(const BYTEARRAY &data);
	CIncomingChatPlayer *RECEIVE_W3GS_CHAT_TO_HOST(const BYTEARRAY &data);
	CIncomingMapSize *RECEIVE_W3GS_MAPSIZE(const BYTEARRAY &data);
	uint32_t RECEIVE_W3GS_PONG_TO_HOST(const BYTEARRAY &data);

	// send functions

	BYTEARRAY SEND_W3GS_PING_FROM_HOST(uint32_t ticks);
	BYTEARRAY SEND_W3GS_SLOTINFOJOIN(uint8_t PID, uint16_t port, uint32_t externalIP, const std::vector<CGameSlot> &slots, uint32_t randomSeed, uint8_t layoutStyle, uint8_t playerSlots);
	BYTEARRAY SEND_W3GS_REJECTJOIN(uint32_t reason);
	BYTEARRAY SEND_W3GS_PLAYERINFO(uint8_t PID, const std::string &name, uint32_t externalIP, uint32_t internalIP);
	BYTEARRAY SEND_W3GS_PLAYERLEAVE_OTHERS(uint8_t PID, uint32_t leftCode);
	BYTEARRAY SEND_W3GS_GAMELOADED_OTHERS(uint8_t PID);
	BYTEARRAY SEND_W3GS_SLOTINFO(const std::vector<CGameSlot> &slots, uint32_t randomSeed, uint8_t layoutStyle, uint8_t playerSlots);
	BYTEARRAY SEND_W3GS_COUNTDOWN_START();
	BYTEARRAY SEND_W3GS_COUNTDOWN_END();
	BYTEARRAY SEND_W3GS_INCOMING_ACTION(const std::vector<CIncomingAction *>& actions, uint16_t sendInterval);
	BYTEARRAY SEND_W3GS_INCOMING_ACTION2(const std::vector<CIncomingAction *>& actions);
	BYTEARRAY SEND_W3GS_CHAT_FROM_HOST(uint8_t fromPID, const BYTEARRAY &toPIDs, uint8_t flag, uint32_t flagExtra, const std::string &message);
	BYTEARRAY SEND_W3GS_START_LAG(const std::vector<std::pair<uint8_t, uint32_t>>& lags);
	BYTEARRAY SEND_W3GS_STOP_LAG(uint8_t pid, uint32_t time);
	BYTEARRAY SEND_W3GS_GAMEINFO(uint8_t war3Version, uint32_t mapGameType, uint32_t mapFlags, uint16_t mapWidth, uint16_t mapHeight, const std::string &gameName, const std::string &hostName, uint32_t upTime, const std::string &mapPath, uint32_t mapCRC, uint32_t slotsTotal, uint32_t slotsOpen, uint16_t port, uint32_t hostCounter, uint32_t entryKey);
	BYTEARRAY SEND_W3GS_CREATEGAME(uint8_t war3Version);
	BYTEARRAY SEND_W3GS_REFRESHGAME(uint32_t players, uint32_t playerSlots);
	BYTEARRAY SEND_W3GS_DECREATEGAME();
	BYTEARRAY SEND_W3GS_MAPCHECK(const std::string &mapPath, uint32_t mapSize, uint32_t mapInfo, uint32_t mapCRC, const BYTEARRAY &mapSHA1);
	BYTEARRAY SEND_W3GS_STARTDOWNLOAD(uint8_t fromPID);
	BYTEARRAY SEND_W3GS_MAPPART(uint8_t fromPID, uint8_t toPID, uint32_t start, const std::string *mapData);

	// other functions

private:
	bool ValidateLength(const BYTEARRAY &content);
	BYTEARRAY EncodeSlotInfo(const std::vector<CGameSlot> &slots, uint32_t randomSeed, uint8_t layoutStyle, uint8_t playerSlots);
};

//
// CIncomingJoinPlayer
//

class CIncomingJoinPlayer
{
private:
	std::string m_Name;
	uint32_t m_InternalIP;
	uint32_t m_HostCounter;
	uint32_t m_EntryKey;

public:
	CIncomingJoinPlayer(uint32_t nHostCounter, uint32_t nEntryKey, const std::string &nName, uint32_t nInternalIP);
	~CIncomingJoinPlayer();

	inline uint32_t GetHostCounter() const                     { return m_HostCounter; }
	inline uint32_t GetEntryKey() const                        { return m_EntryKey; }
	inline std::string GetName() const                         { return m_Name; }
	inline uint32_t GetInternalIP() const                      { return m_InternalIP; }
};

//
// CIncomingAction
//

class CIncomingAction
{
private:
	BYTEARRAY m_CRC;
	BYTEARRAY m_Action;
	uint8_t m_PID;

public:
	CIncomingAction(uint8_t nPID, const BYTEARRAY &nCRC, const BYTEARRAY &nAction);
	~CIncomingAction();

	inline uint8_t GetPID() const                              { return m_PID; }
	inline BYTEARRAY GetCRC() const                            { return m_CRC; }
	inline BYTEARRAY *GetAction()                              { return &m_Action; }
	inline uint32_t GetLength() const                          { return m_Action.size() + 3; }
};

//
// CIncomingChatPlayer
//

class CIncomingChatPlayer
{
public:

	enum ChatToHostType
	{
		CTH_MESSAGE = 0,  // a chat message
		CTH_MESSAGEEXTRA = 1,  // a chat message with extra flags
		CTH_TEAMCHANGE = 2,  // a team change request
		CTH_COLOURCHANGE = 3,  // a colour change request
		CTH_RACECHANGE = 4,  // a race change request
		CTH_HANDICAPCHANGE = 5   // a handicap change request
	};

private:
	std::string m_Message;
	BYTEARRAY m_ToPIDs;
	BYTEARRAY m_ExtraFlags;
	ChatToHostType m_Type;
	uint8_t m_FromPID;
	uint8_t m_Flag;
	uint8_t m_Byte;

public:
	CIncomingChatPlayer(uint8_t nFromPID, const BYTEARRAY &nToPIDs, uint8_t nFlag, const std::string &nMessage);
	CIncomingChatPlayer(uint8_t nFromPID, const BYTEARRAY &nToPIDs, uint8_t nFlag, const std::string &nMessage, const BYTEARRAY &nExtraFlags);
	CIncomingChatPlayer(uint8_t nFromPID, const BYTEARRAY &nToPIDs, uint8_t nFlag, uint8_t nByte);
	~CIncomingChatPlayer();

	inline ChatToHostType GetType() const                      { return m_Type; }
	inline uint8_t GetFromPID() const                          { return m_FromPID; }
	inline BYTEARRAY GetToPIDs() const                         { return m_ToPIDs; }
	inline uint8_t GetFlag() const                             { return m_Flag; }
	inline std::string GetMessage() const                      { return m_Message; }
	inline uint8_t GetByte() const                             { return m_Byte; }
	inline BYTEARRAY GetExtraFlags() const                     { return m_ExtraFlags; }
};

class CIncomingMapSize
{
private:
	uint32_t m_MapSize;
	uint8_t m_SizeFlag;

public:
	CIncomingMapSize(uint8_t nSizeFlag, uint32_t nMapSize);
	~CIncomingMapSize();

	inline uint8_t GetSizeFlag() const                         { return m_SizeFlag; }
	inline uint32_t GetMapSize() const                         { return m_MapSize; }
};

#endif  // AURA_GAMEPROTOCOL_H_
