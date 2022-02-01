/***************************************************************************
*      Mechanized Assault and Exploration Reloaded Projectfile            *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "savegame.h"

#include "defines.h"
#include "game/data/gamesettings.h"
#include "game/data/model.h"
#include "game/data/player/player.h"
#include "game/data/savegameinfo.h"
#include "game/logic/server.h"
#include "game/logic/turntimeclock.h"
#include "game/protocol/netmessage.h"
#include "game/serialization/serialization.h"
#include "game/serialization/jsonarchive.h"
#include "maxrversion.h"
#include "settings.h"
#include "utility/files.h"
#include "utility/log.h"
#include "utility/ranges.h"

#include <ctime>
#include <config/workaround/cpp17/optional.h>
#include <regex>

#define SAVE_FORMAT_VERSION ((std::string)"1.0")

namespace
{
	//--------------------------------------------------------------------------
	std::optional<nlohmann::json> loadDocument (int slot)
	{
		const std::string fileName = cSavegame::getFileName (slot);
		std::ifstream file (fileName);
		nlohmann::json json;
		if (!(file >> json))
		{
			Log.write ("Error loading savegame file: " + fileName, cLog::eLOG_TYPE_ERROR);
			return std::nullopt;
		}
		return json;
	}

	//--------------------------------------------------------------------------
	std::optional<cVersion> loadVersion (const nlohmann::json& json, int slot)
	{
		const auto& jsonVersion = json["version"];
		if (!jsonVersion.is_string())
		{
			Log.write ("Error loading savegame file " + std::to_string (slot) + ": \"version\" field not found.", cLog::eLOG_TYPE_ERROR);
			return std::nullopt;
		}
		cVersion version;
		version.parseFromString (jsonVersion);
		return version;
	}

	//--------------------------------------------------------------------------
	void writeHeader (nlohmann::json& json, int slot, const std::string& saveName, const cModel &model)
	{
		//init document
		json["version"] = SAVE_FORMAT_VERSION;

		//write header
		char timestr[21];
		time_t tTime = time (nullptr);
		tm* tmTime = localtime (&tTime);
		strftime (timestr, 21, "%d.%m.%y %H:%M", tmTime);

		eGameType type = eGameType::Single;
		int humanPlayers = 0;
		for (auto player : model.getPlayerList())
		{
			if (player->isHuman())
				humanPlayers++;
		}
		if (humanPlayers > 1)
			type = eGameType::TcpIp;
		if (model.getGameSettings()->gameType == eGameSettingsGameType::HotSeat)
			type = eGameType::Hotseat;

		cJsonArchiveOut archive (json["header"]);
		archive << serialization::makeNvp ("gameVersion", std::string (PACKAGE_VERSION  " "  PACKAGE_REV));
		archive << serialization::makeNvp ("gameName", saveName);
		archive << serialization::makeNvp ("type", type);
		archive << serialization::makeNvp ("date", std::string (timestr));
	}

}

//------------------------------------------------------------------------------
void cSavegame::save (const cModel& model, int slot, const std::string& saveName)
{
	nlohmann::json json;
	writeHeader (json, slot, saveName, model);

	cJsonArchiveOut archive (json);
	archive << NVP (model);

	// add model crc
	cJsonArchiveOut archiveCrc (json);
	archiveCrc << serialization::makeNvp ("modelcrc", model.getChecksum());

	makeDirectories (cSettings::getInstance().getSavesPath());
	{
		std::ofstream file (getFileName (slot));
		file << json.dump(2);
	}
#if 1
	if (cSettings::getInstance().isDebug()) // Check Save/Load consistency
	{
		cModel model2;

		loadModel (model2, slot);

		if (model.getChecksum() != model2.getChecksum())
		{
			Log.write ("Checksum issue when saving", cLog::eLOG_TYPE_ERROR);
		}
	}
#endif
}

//------------------------------------------------------------------------------
void cSavegame::saveGuiInfo (const cNetMessageGUISaveInfo& guiInfo)
{
	auto json = loadDocument (guiInfo.slot);
	if (!json)
	{
		return;
	}
	cJsonArchiveOut archive ((*json)["GuiInfo"].emplace_back());
	archive << serialization::makeNvp ("playerNr", guiInfo.playerNr);
	archive << serialization::makeNvp ("guiState", guiInfo.guiInfo);

	makeDirectories (cSettings::getInstance().getSavesPath());
	int loadedSlot = guiInfo.slot;
	std::ofstream file (getFileName (loadedSlot));
	file << json->dump(2);
}

//------------------------------------------------------------------------------
cSaveGameInfo cSavegame::loadSaveInfo (int slot)
{
	cSaveGameInfo info (slot);

	const auto& json = loadDocument (slot);
	if (!json)
	{
		info.gameName = "Load Error";
		return info;
	}

	auto saveVersion = loadVersion (*json, slot);
	if (!saveVersion)
	{
		info.gameName = "File Error";
		return info;
	}
	info.saveVersion = *saveVersion;

	try
	{
		cJsonArchiveIn header (json->at("header"));

		header >> serialization::makeNvp ("gameVersion", info.gameVersion);
		header >> serialization::makeNvp ("gameName", info.gameName);
		header >> serialization::makeNvp ("type", info.type);
		header >> serialization::makeNvp ("date", info.date);

		int numPlayers = 0;
		for (auto& playerJson : json->at("model").at("players"))
		{
			cJsonArchiveIn archive (playerJson);
			sPlayerSettings player;
			int id;
			bool isDefeated;

			archive >> NVP (player);
			archive >> NVP (id);
			archive >> NVP (isDefeated);

			info.players.emplace_back (cPlayerBasicData (player, id, isDefeated));
		}
		cJsonArchiveIn mapArchive (json->at("model").at("map").at("mapFile"));
		mapArchive >> serialization::makeNvp ("filename", info.mapName);
		mapArchive >> serialization::makeNvp ("crc", info.mapCrc);

		cJsonArchiveIn turnArchive (json->at("model").at("turnCounter"));
		turnArchive >> serialization::makeNvp ("turn", info.turn);
	}
	catch (const std::runtime_error& e)
	{
		Log.write ("Error loading savegame file " + std::to_string (slot) + ": " + e.what(), cLog::eLOG_TYPE_ERROR);
		info.gameName = "File Error";
		return info;
	}
	return info;
}

//------------------------------------------------------------------------------
std::string cSavegame::getFileName (int slot)
{
	char numberstr[4];
	snprintf (numberstr, sizeof (numberstr), "%.3d", slot);
	return cSettings::getInstance().getSavesPath() + PATH_DELIMITER + "Save" + numberstr + ".json";
}

//------------------------------------------------------------------------------
void cSavegame::loadModel (cModel& model, int slot)
{
	const auto& json = loadDocument (slot);
	if (!json)
	{
		throw std::runtime_error ("Could not load savegame file " + std::to_string (slot));
	}

	auto saveVersion = loadVersion (*json, slot);
	if (!saveVersion)
	{
		throw std::runtime_error ("Could not load version info from savegame file " + std::to_string (slot));
	}

	if (*saveVersion < cVersion (1, 0))
	{
		throw std::runtime_error ("Savegame version is not compatible. Versions < 1.0 are not supported.");
	}
	try
	{
		serialization::cPointerLoader loader (model);
		cJsonArchiveIn archive (*json, &loader);
		archive >> NVP (model);

		// check crc
		cJsonArchiveIn modelArchive (*json, &loader);
		uint32_t crcFromSave;
		modelArchive >> serialization::makeNvp ("modelcrc", crcFromSave);
		Log.write (" Checksum from save file: " + std::to_string (crcFromSave), cLog::eLOG_TYPE_NET_DEBUG);

		uint32_t modelCrc = model.getChecksum();
		Log.write (" Checksum after loading model: " + std::to_string (modelCrc), cLog::eLOG_TYPE_NET_DEBUG);
		Log.write (" GameId: " + std::to_string (model.getGameId()), cLog::eLOG_TYPE_NET_DEBUG);

		if (crcFromSave != modelCrc)
		{
			Log.write (" Crc of loaded model does not match the saved crc!", cLog::eLOG_TYPE_NET_ERROR);
			//TODO: what to do in this case?
		}
	}
	catch (const std::exception& e)
	{
		Log.write ("Error loading savegame file " + std::to_string (slot) + ": " + e.what(), cLog::eLOG_TYPE_ERROR);
	}
}

//------------------------------------------------------------------------------
void cSavegame::loadGuiInfo (const cServer* server, int slot, int playerNr)
{
	const auto& json = loadDocument (slot);
	if (!json)
	{
		throw std::runtime_error ("Could not load savegame file " + std::to_string (slot));
	}

	for (const auto& player : json->at("GuiInfo"))
	{
		cJsonArchiveIn archive (player);
		cNetMessageGUISaveInfo guiInfo (slot, -1);

		archive >> serialization::makeNvp ("playerNr", guiInfo.playerNr);
		archive >> serialization::makeNvp ("guiState", guiInfo.guiInfo);

		if (guiInfo.playerNr == playerNr || playerNr == -1)
		{
			server->sendMessageToClients (guiInfo, guiInfo.playerNr);
		}
	}
}

//------------------------------------------------------------------------------
void fillSaveGames (std::size_t minIndex, std::size_t maxIndex, std::vector<cSaveGameInfo>& saveGames)
{
	cSavegame savegame;
	const auto saveFileNames = getFilesOfDirectory (cSettings::getInstance().getSavesPath());
	const std::regex savename_regex{R"(Save(\d{3})\.json)"};

	for (const auto& filename : saveFileNames)
	{
		std::smatch match;
		if (!std::regex_match (filename, match, savename_regex)) continue;
		const std::size_t number = atoi (match[1].str().c_str());

		if (number <= minIndex || number > maxIndex) continue;

		if (ranges::find_if (saveGames, [=](const cSaveGameInfo& save) { return std::size_t (save.number) == number; }) != saveGames.end()) continue;

		// read the information and add it to the saves list
		cSaveGameInfo saveInfo = savegame.loadSaveInfo (number);
		saveGames.push_back (saveInfo);
	}
}
