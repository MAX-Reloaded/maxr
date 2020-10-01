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

#include "servergame.h"

#include "utility/log.h"
#include "game/data/player/player.h"
#include "game/data/savegame.h"
#include "game/data/map/map.h"
#include "game/logic/turncounter.h"
#include "game/logic/landingpositionmanager.h"
#include "game/logic/server.h"
#include "mapdownloader/mapdownload.h"
#include "mapdownloader/mapuploadmessagehandler.h"
#include "maxrversion.h"
#include "protocol/lobbymessage.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

//------------------------------------------------------------------------------
int serverGameThreadFunction (void* data)
{
	cServerGame* serverGame = reinterpret_cast<cServerGame*> (data);
	serverGame->run();
	return 0;
}

namespace
{
	//--------------------------------------------------------------------------
	class cDedicatedServerChatMessageHandler : public ILobbyMessageHandler
	{
	public:
		explicit cDedicatedServerChatMessageHandler (cServerGame& serverGame) : serverGame (&serverGame) {}
	private:
		bool handleMessage (const cMultiplayerLobbyMessage&) final;

		void serverCommand (int fromPlayer, const std::vector<std::string>& tokens);
	private:
		cServerGame* serverGame;
	};

	//--------------------------------------------------------------------------
	bool cDedicatedServerChatMessageHandler::handleMessage (const cMultiplayerLobbyMessage& message)
	{
		if (message.getType() != cMultiplayerLobbyMessage::eMessageType::MU_MSG_CHAT) return false;
		const auto& chatMessage = static_cast<const cMuMsgChat&>(message);

		const auto& chatText = chatMessage.message;
		size_t serverStringPos = chatText.find ("--server");
		if (serverStringPos == std::string::npos || chatText.length() <= serverStringPos + 9)
		{
			return false;
		}
		std::string command = chatText.substr (serverStringPos + 9);
		std::vector<std::string> tokens;
		std::istringstream iss (command);
		std::copy (std::istream_iterator<std::string> (iss), std::istream_iterator<std::string> (), std::back_inserter<std::vector<std::string> > (tokens));
		serverGame->handleChatCommand (message.playerNr, tokens);
		return true;
	}

	//--------------------------------------------------------------------------
	std::string getServerHelpString()
	{
		std::stringstream oss;
		oss << "--- Dedicated server help ---" << std::endl;
		oss << "Type --server and then one of the following:" << std::endl;
		oss << "go: starts the game" << std::endl;
		oss << "games: shows the running games and their players" << std::endl;
		oss << "maps: shows the available maps" << std::endl;
		oss << "map mapname.wrl: changes the map" << std::endl;
		return oss.str();
	}

}

//------------------------------------------------------------------------------
cServerGame::cServerGame (std::shared_ptr<cConnectionManager> connectionManager, int port) :
	lobbyServer (std::move (connectionManager)),
	port(port),
	thread (nullptr),
	canceled (false),
	shouldSave (false),
	saveGameNumber (-1)
{
	lobbyServer.addLobbyMessageHandler (std::make_unique<cDedicatedServerChatMessageHandler>(*this));

	signalConnectionManager.connect (lobbyServer.onClientConnected, [this](const cPlayerBasicData& player)
	{
		lobbyServer.sendChatMessage ("type --server help for dedicated server help", player.getNr());
	});
	signalConnectionManager.connect (lobbyServer.onDifferentVersion, [this](const std::string& version, const std::string& revision)
	{
		std::cout << "player connects with different version:" << version << " " << revision << std::endl;
	});

	signalConnectionManager.connect (lobbyServer.onMapRequested, [this](const cPlayerBasicData& player)
	{
		std::cout << player.getName() << " requests map." << std::endl;
	});
	signalConnectionManager.connect (lobbyServer.onMapUploaded, [this](const cPlayerBasicData& player)
	{
		std::cout << player.getName() << " finished to download map." << std::endl;
	});

	signalConnectionManager.connect (lobbyServer.onStartNewGame, [this](const sLobbyPreparationData& preparationData, std::shared_ptr<cConnectionManager> connectionManager)
	{
		server = std::make_unique<cServer> (connectionManager);

		server->setUnitsData (preparationData.unitsData);
		//server->setClansData(preparationData.clanData);
		server->setGameSettings (*preparationData.gameSettings);
		server->setMap (preparationData.staticMap);
		server->setPlayers(preparationData.players);

		connectionManager->setLocalServer(server.get());

		server->start();
	});

	signalConnectionManager.connect (lobbyServer.onStartLoadGame, [this](const cSaveGameInfo& saveGameInfo, std::shared_ptr<cConnectionManager> connectionManager)
	{
		server = std::make_unique<cServer> (connectionManager);
		server->loadGameState (saveGameInfo.number);
		connectionManager->setLocalServer(server.get());
		server->start();
	});
}

//------------------------------------------------------------------------------
cServerGame::~cServerGame()
{
	if (thread != 0)
	{
		canceled = true;
		SDL_WaitThread (thread, nullptr);
		thread = 0;
	}
}

//------------------------------------------------------------------------------
void cServerGame::runInThread()
{
	thread = SDL_CreateThread (serverGameThreadFunction, "servergame", this);
}

//------------------------------------------------------------------------------
void cServerGame::pushMessage (std::unique_ptr<cNetMessage> message)
{
 	lobbyServer.pushMessage (std::move (message));
}

//------------------------------------------------------------------------------
std::unique_ptr<cNetMessage> cServerGame::popMessage()
{
	return lobbyServer.popMessage();
}
//------------------------------------------------------------------------------
bool cServerGame::loadGame (int saveGameNumber)
{
	cSaveGameInfo saveGameInfo = cSavegame().loadSaveInfo(saveGameNumber);
	lobbyServer.selectSaveGameInfo (saveGameInfo);
	return true;
}

//------------------------------------------------------------------------------
void cServerGame::saveGame (int saveGameNumber)
{
	if (server == nullptr)
	{
		std::cout << "Server not running. Can't save game." << std::endl;
		return;
	}
	std::cout << "Scheduling save command for server thread..." << std::endl;
	this->saveGameNumber = saveGameNumber;
	shouldSave = true;
}

//------------------------------------------------------------------------------
void cServerGame::prepareGameData()
{
	cGameSettings settings;
	settings.setMetalAmount (eGameSettingsResourceAmount::Normal);
	settings.setOilAmount (eGameSettingsResourceAmount::Normal);
	settings.setGoldAmount (eGameSettingsResourceAmount::Normal);
	settings.setResourceDensity (eGameSettingsResourceDensity::Normal);
	settings.setStartCredits (cGameSettings::defaultCreditsNormal);
	settings.setBridgeheadType (eGameSettingsBridgeheadType::Definite);
	//settings.alienTech = SETTING_ALIENTECH_OFF;
	settings.setClansEnabled (true);
	settings.setGameType (eGameSettingsGameType::Simultaneous);
	settings.setVictoryCondition (eGameSettingsVictoryCondition::Death);

	auto map = std::make_shared<cStaticMap> ();
	const std::string mapName = "Mushroom.wrl";
	map->loadMap (mapName);

	lobbyServer.selectGameSettings (std::make_shared<cGameSettings> (settings));
	lobbyServer.selectMap (map);
}

//------------------------------------------------------------------------------
void cServerGame::run()
{
	while (canceled == false)
	{
		lobbyServer.run();
		SDL_Delay(10);
#if 1
		// don't do anything if games haven't been started yet!
		if (server && shouldSave)
		{
			server->saveGameState(saveGameNumber, "Dedicated Server Savegame");
			std::cout << "...saved to slot " << saveGameNumber << std::endl;
			shouldSave = false;
		}
#endif
	}
	server = nullptr;
}

//--------------------------------------------------------------------------
void cServerGame::handleChatCommand (int fromPlayer, const std::vector<std::string>& tokens)
{
	const auto* senderPlayer = lobbyServer.getConstPlayer (fromPlayer);
	if (tokens.size() == 1)
	{
		if (tokens[0] == "go")
		{
			lobbyServer.askToFinishLobby (fromPlayer);
		}
		else if (tokens[0] == "help")
		{
			lobbyServer.sendChatMessage (getServerHelpString(), fromPlayer);
		}
		else if (tokens[0] == "games")
		{
			lobbyServer.sendChatMessage (getGamesString(), fromPlayer);
		}
		else if (tokens[0] == "maps")
		{
			lobbyServer.sendChatMessage (getAvailableMapsString(), fromPlayer);
		}
	}
	else if (tokens.size() >= 2)
	{
		if (tokens[0] == "map")
		{
			std::string mapName = tokens[1];
			for (size_t i = 2; i < tokens.size(); i++)
			{
				mapName += " ";
				mapName += tokens[i];
			}
			auto map = std::make_shared<cStaticMap>();
			if (map->loadMap (mapName))
			{
				lobbyServer.selectMap (map);
				std::string reply = senderPlayer->getName();
				reply += " changed the map.";
				lobbyServer.sendChatMessage (reply);
			}
			else
			{
				std::string reply = "Could not load map ";
				reply += mapName;
				lobbyServer.sendChatMessage (reply, senderPlayer->getNr());
			}
		}
	}
}

//------------------------------------------------------------------------------
std::string cServerGame::getGameState() const
{
	return (server == nullptr) ? lobbyServer.getGameState() : server->getGameState();
}
