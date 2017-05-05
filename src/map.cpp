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

#include "map.h"
#include "util.h"
#include "config.h"
#include "gameslot.h"

void Print(const std::string &message);

template <class T, size_t N>
bool ExtractNumbers(const std::string &s, std::array<T, N>& result)
{
	uint32_t c;
	std::stringstream SS;
	SS << s;
	for (size_t i = 0; i < N; ++i)
	{
		if (SS.eof())
			return false;
		SS >> c;
		result[i] = (T)c;
	}
	return true;
}

//
// CMap
//

CMap::CMap(std::string const& MapPath, CConfig *MAP)
{
	Load(MapPath, MAP);
}

CMap::~CMap()
{

}

uint32_t CMap::GetMapGameFlags() const
{
	uint32_t GameFlags = 0;

	// speed

	if (m_MapSpeed == MAPSPEED::SLOW)
		GameFlags = 0x00000000;
	else if (m_MapSpeed == MAPSPEED::NORMAL)
		GameFlags = 0x00000001;
	else
		GameFlags = 0x00000002;

	// visibility

	if (m_MapVisibility == MAPVIS::HIDETERRAIN)
		GameFlags |= 0x00000100;
	else if (m_MapVisibility == MAPVIS::EXPLORED)
		GameFlags |= 0x00000200;
	else if (m_MapVisibility == MAPVIS::ALWAYSVISIBLE)
		GameFlags |= 0x00000400;
	else
		GameFlags |= 0x00000800;

	// observers

	if (m_MapObservers == MAPOBS::ONDEFEAT)
		GameFlags |= 0x00002000;
	else if (m_MapObservers == MAPOBS::ALLOWED)
		GameFlags |= 0x00003000;
	else if (m_MapObservers == MAPOBS::REFEREES)
		GameFlags |= 0x40000000;

	// teams/units/hero/race

	if (m_MapFlags & MAPFLAG::TEAMSTOGETHER)
		GameFlags |= 0x00004000;

	if (m_MapFlags & MAPFLAG::FIXEDTEAMS)
		GameFlags |= 0x00060000;

	if (m_MapFlags & MAPFLAG::UNITSHARE)
		GameFlags |= 0x01000000;

	if (m_MapFlags & MAPFLAG::RANDOMHERO)
		GameFlags |= 0x02000000;

	if (m_MapFlags & MAPFLAG::RANDOMRACES)
		GameFlags |= 0x04000000;

	return GameFlags;
}

uint8_t CMap::GetMapLayoutStyle() const
{
	// 0 = melee
	// 1 = custom forces
	// 2 = fixed player settings (not possible with the Warcraft III map editor)
	// 3 = custom forces + fixed player settings

	if (!(m_MapOptions & MAPOPT::CUSTOMFORCES))
		return 0;

	if (!(m_MapOptions & MAPOPT::FIXEDPLAYERSETTINGS))
		return 1;

	return 3;
}

void CMap::Load(std::string const& MapPath, CConfig *MAP)
{
	m_Valid = true;

	m_MapPath = MapPath;

	m_MapSize = ByteArrayToUInt32(ExtractNumbers(MAP->GetString("map_size", std::string()), 4), false);
	m_MapInfo = ByteArrayToUInt32(ExtractNumbers(MAP->GetString("map_info", std::string()), 4), false);
	m_MapCRC = ByteArrayToUInt32(ExtractNumbers(MAP->GetString("map_crc", std::string()), 4), false);
	std::string sha1_str = MAP->GetString("map_sha1", std::string());
	if (!ExtractNumbers(sha1_str, m_MapSHA1)) {
		m_Valid = false;
		Print("[MAP] invalid map_sha1 detected");
		return;
	}

	Print("[MAP] map_size = " + std::to_string(m_MapSize));
	Print("[MAP] map_info = " + std::to_string(m_MapInfo));
	Print("[MAP] map_crc = " + std::to_string(m_MapCRC));
	Print("[MAP] map_sha1 = " + sha1_str);

	m_MapOptions = MAP->GetInt("map_options", 0);
	m_MapWidth = ByteArrayToUInt16(ExtractNumbers(MAP->GetString("map_width", std::string()), 2), false);
	m_MapHeight = ByteArrayToUInt16(ExtractNumbers(MAP->GetString("map_height", std::string()), 2), false);

	Print("[MAP] map_options = " + std::to_string(m_MapOptions));
	Print("[MAP] map_width = " + std::to_string(m_MapWidth));
	Print("[MAP] map_height = " + std::to_string(m_MapHeight));

	m_MapNumPlayers = MAP->GetInt("map_numplayers", 0);
	m_Slots.clear();
	for (uint32_t Slot = 1; Slot <= 12; ++Slot)
	{
		std::string SlotString = MAP->GetString("map_slot" + std::to_string(Slot), std::string());
		Print("[MAP] map_slot" + std::to_string(Slot) + " = " + SlotString);
		if (SlotString.empty())
			break;
		std::array<uint8_t, 9> SlotData;
		if (!ExtractNumbers(SlotString, SlotData)) {
			break;
		}
		m_Slots.push_back(CGameSlot(SlotData[0], SlotData[1], SlotData[2], SlotData[3], SlotData[4], SlotData[5], SlotData[6], SlotData[7], SlotData[8]));
	}
	m_MapNumPlayers = m_Slots.size();

	m_MapSpeed = MAPSPEED::FAST;
	m_MapVisibility = MAPVIS::DEFAULT;
	m_MapObservers = MAPOBS::NONE;
	m_MapFlags = MAPFLAG::TEAMSTOGETHER | MAPFLAG::FIXEDTEAMS;

	if (m_MapOptions & MAPOPT::MELEE)
	{
		uint8_t Team = 0;
		for (auto & Slot : m_Slots)
		{
			(Slot).SetTeam(Team++);
			(Slot).SetRace(SLOTRACE_RANDOM);
		}
		// force melee maps to have observer slots enabled by default
		if (m_MapObservers == MAPOBS::NONE)
			m_MapObservers = MAPOBS::ALLOWED;
	}

	if (!(m_MapOptions & MAPOPT::FIXEDPLAYERSETTINGS))
	{
		for (auto & Slot : m_Slots)
			(Slot).SetRace((Slot).GetRace() | SLOTRACE_SELECTABLE);
	}

	// if random races is set force every slot's race to random

	if (m_MapFlags & MAPFLAG::RANDOMRACES)
	{
		Print("[MAP] forcing races to random");

		for (auto & slot : m_Slots)
			slot.SetRace(SLOTRACE_RANDOM);
	}

	// add observer slots

	if (m_MapObservers == MAPOBS::ALLOWED || m_MapObservers == MAPOBS::REFEREES)
	{
		Print("[MAP] adding " + std::to_string(12 - m_Slots.size()) + " observer slots");

		while (m_Slots.size() < 12)
			m_Slots.push_back(CGameSlot(0, 255, SLOTSTATUS_OPEN, 0, 12, 12, SLOTRACE_RANDOM));
	}

	CheckValid();
}

void CMap::CheckValid()
{
	// TODO: should this code fix any errors it sees rather than just warning the user?

	if (m_MapPath.empty() || m_MapPath.length() > 53)
	{
		m_Valid = false;
		Print("[MAP] invalid map_path detected");
	}

	if (m_MapPath.find('/') != std::string::npos)
		Print("[MAP] warning - map_path contains forward slashes '/' but it must use Windows style back slashes '\\'");


	if (!m_MapData.empty() && m_MapData.size() != m_MapSize)
	{
		m_Valid = false;
		Print("[MAP] invalid map_size detected - size mismatch with actual map data");
	}

	if (m_MapNumPlayers == 0 || m_MapNumPlayers > 12)
	{
		m_Valid = false;
		Print("[MAP] invalid map_numplayers detected");
	}

	if (m_Slots.empty() || m_Slots.size() > 12)
	{
		m_Valid = false;
		Print("[MAP] invalid map_slot<x> detected");
	}
}

const std::string* CMap::GetMapData() const
{
	// todo; ���ص�ͼ֧��
	return &m_MapData;
}
