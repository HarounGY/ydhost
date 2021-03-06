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

#ifndef AURA_GAME_H_
#define AURA_GAME_H_

#include "gameslot.h"
#include <vector>
#include <queue>
typedef std::vector<uint8_t> BYTEARRAY;

//
// CGame
//

class CUDPSocket;
class CTCPServer;
class CGameProtocol;
class CPotentialPlayer;
class CGamePlayer;
class CMap;
class CIncomingJoinPlayer;
class CIncomingAction;
class CIncomingChatPlayer;
class CIncomingMapSize;

class CTimer
{
public:
	CTimer()
		: m_Ticks(0)
	{ }

	bool update(uint32_t CurTicks, int32_t Timeout)
	{
		if (CurTicks < m_Ticks + Timeout)
		{
			return false;
		}
		reset(m_Ticks + Timeout);
		return true;
	}

	void reset(uint32_t CurTicks)
	{
		m_Ticks = CurTicks;
	}

private:
	uint32_t m_Ticks;
};

struct CGameConfig
{
	std::string GameName;
	std::string VirtualHostName;
	uint8_t     War3Version;
	uint32_t    Latency;
	uint32_t    AutoStart;
};

class CGame
{
protected:
	CUDPSocket *m_UDPSocket;
	CTCPServer *m_Socket;                         // listening socket
	CGameProtocol *m_Protocol;                    // game protocol
	std::vector<CGameSlot> m_Slots;               // std::vector of slots
	std::vector<CPotentialPlayer *> m_Potentials; // std::vector of potential players (connections that haven't sent a W3GS_REQJOIN packet yet)
	std::vector<CGamePlayer *> m_Players;         // std::vector of players
	std::vector<CIncomingAction *> m_Actions;     // queue of actions to be sent
	const CMap *m_Map;                            // map data
	const CGameConfig* m_Config;
	uint32_t m_RandomSeed;                        // the random seed sent to the Warcraft III clients
	uint32_t m_HostCounter;                       // a unique game number
	uint32_t m_EntryKey;                          // random entry key for LAN, used to prove that a player is actually joining from LAN
	uint32_t m_SyncLimit;                         // the maximum number of packets a player can fall out of sync before starting the lag screen
	uint32_t m_SyncCounter;                       // the number of actions sent so far (for determining if anyone is lagging)
	uint32_t m_CountDownCounter;                  // the countdown is finished when this reaches zero
	uint32_t m_StartedLaggingTicks;               // GetTicks when the last lag screen started
	uint32_t m_LastLagScreenTicks;                // GetTicks when the last lag screen was active (continuously updated)
	uint32_t m_EmptyWaitingTicks;
	CTimer m_ActionSentTimer;                     // GetTicks when the last action packet was sent
	CTimer m_PingTimer;                           // GetTicks when the last ping was sent
	CTimer m_DownloadTimer;                       // GetTicks when the last map download cycle was performed
	CTimer m_SyncSlotInfoTimer;                   // GetTicks when the download counter was last reset
	CTimer m_CountDownTimer;                      // GetTicks when the last countdown message was sent
	CTimer m_LagScreenResetTimer;                 // GetTicks when the "lag" screen was last reset
	uint16_t m_HostPort;                          // the port to host games on
	uint8_t m_VirtualHostPID;                     // host's PID
	bool m_Exiting;                               // set to true and this class will be deleted next update
	bool m_SlotInfoChanged;                       // if the slot info has changed and hasn't been sent to the players yet (optimization)

	bool m_Lagging;                               // if the lag screen is active or not
	bool m_Desynced;                              // if the game has desynced or not

	enum class State
	{
		Waiting,
		CountDown,
		Loading,
		Loaded,
	};

	State m_State;

public:
	CGame(const CMap* Map, const CGameConfig* Config, CUDPSocket* UDPSocket, uint32_t HostCounter);
	~CGame();
	CGame(CGame &) = delete;

	inline std::string GetGameName() const            { return m_Config->GameName; }
	inline std::string GetVirtualHostName() const     { return m_Config->VirtualHostName; }
	inline uint32_t GetLatency() const                { return m_Config->Latency; }
	inline uint32_t GetLastLagScreenTicks() const     { return m_LastLagScreenTicks; }
	
	uint32_t GetNumPlayers() const;

	inline void SetExiting(bool nExiting)                      { m_Exiting = nExiting; }

	// processing functions

	uint32_t SetFD(void *fd, void *send_fd, int32_t *nfds);
	bool Update(void *fd, void *send_fd);
	void UpdatePost(void *send_fd);

	// generic functions to send packets to players

	void Send(CGamePlayer *player, const BYTEARRAY &data);
	void SendAll(const BYTEARRAY &data);

	// functions to send packets to players

	void SendAllChat(const std::string &message);
	void SendAllSlotInfo();
	void SendVirtualHostPlayerInfo(CGamePlayer *player);
	void SendAllActions();

	// events
	// note: these are only called while iterating through the m_Potentials or m_Players std::vectors
	// therefore you can't modify those std::vectors and must use the player's m_DeleteMe member to flag for deletion

	void EventPlayerDeleted(uint32_t Ticks, CGamePlayer *player);
	void EventPlayerDisconnectTimedOut(CGamePlayer *player);
	void EventPlayerDisconnectSocketError(CGamePlayer *player);
	void EventPlayerDisconnectConnectionClosed(CGamePlayer *player);
	void EventPlayerJoined(CPotentialPlayer *potential, CIncomingJoinPlayer *joinPlayer);
	void EventPlayerLeft(CGamePlayer *player, uint32_t reason);
	void EventPlayerLoaded(CGamePlayer *player);
	void EventPlayerAction(CGamePlayer *player, CIncomingAction *action);
	void EventPlayerKeepAlive(CGamePlayer *player);
	void EventPlayerChatToHost(CGamePlayer *player, CIncomingChatPlayer *chatPlayer);
	void EventPlayerChangeTeam(CGamePlayer *player, uint8_t team);
	void EventPlayerChangeColour(CGamePlayer *player, uint8_t colour);
	void EventPlayerChangeRace(CGamePlayer *player, uint8_t race);
	void EventPlayerChangeHandicap(CGamePlayer *player, uint8_t handicap);
	void EventPlayerDropRequest(CGamePlayer *player);
	void EventPlayerMapSize(CGamePlayer *player, CIncomingMapSize *mapSize);

	// these events are called outside of any iterations

	void EventGameStarted(uint32_t Ticks);

	// other functions

	void DeletePlayer(CGamePlayer* player, uint32_t nLeftCode);
	uint8_t GetSIDFromPID(uint8_t PID) const;
	uint8_t GetNewPID();
	uint8_t GetNewColour();
	BYTEARRAY GetPIDs();
	uint8_t GetHostPID();
	uint8_t GetEmptySlot();
	uint8_t GetEmptySlot(uint8_t team, uint8_t PID);
	void SwapSlots(uint8_t SID1, uint8_t SID2);
	void ColourSlot(uint8_t SID, uint8_t colour);
	void StartCountDown();
	void StopLaggers();
	void CreateVirtualHost();
	void DeleteVirtualHost();
};

#endif  // AURA_GAME_H_
