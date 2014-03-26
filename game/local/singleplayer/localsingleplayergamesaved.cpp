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

#include "localsingleplayergamesaved.h"
#include "../../../gui/menu/windows/windowgamesettings/gamesettings.h"
#include "../../../gui/application.h"
#include "../../../gui/game/gamegui.h"
#include "../../../client.h"
#include "../../../server.h"
#include "../../../events.h"
#include "../../../player.h"
#include "../../../clientevents.h"
#include "../../../savegame.h"

//------------------------------------------------------------------------------
void cLocalSingleplayerGameSaved::start (cApplication& application)
{
	eventHandling = std::make_unique<cEventHandling> ();
	server = std::make_unique<cServer> (nullptr);
	client = std::make_unique<cClient> (server.get (), nullptr, *eventHandling);

	cSavegame savegame (saveGameNumber);
	if (savegame.load (*server) == false) return;

	auto staticMap = server->Map->staticMap;
	client->setMap (staticMap);

	const auto& serverPlayerList = server->PlayerList;
	if (serverPlayerList.empty ()) return;

	const int player = 0;

	// Following may be simplified according to serverGame::loadGame
	std::vector<sPlayer> clientPlayerList;

	// copy players for client
	for (size_t i = 0; i != serverPlayerList.size (); ++i)
	{
		const auto& p = *serverPlayerList[i];
		clientPlayerList.push_back (sPlayer (p.getName (), p.getColor (), p.getNr (), p.getSocketNum ()));
	}
	client->setPlayers (clientPlayerList, player);

	// in single player only the first player is important
	serverPlayerList[player]->setLocal ();
	sendRequestResync (*client, serverPlayerList[player]->getNr ());

	// TODO: move that in server
	for (size_t i = 0; i != serverPlayerList.size (); ++i)
	{
		sendHudSettings (*server, *serverPlayerList[i]);
		std::vector<sSavedReportMessage>& reportList = serverPlayerList[i]->savedReportsList;
		for (size_t j = 0; j != reportList.size (); ++j)
		{
			sendSavedReport (*server, reportList[j], serverPlayerList[i]->getNr ());
		}
		reportList.clear ();
	}

	// start game
	server->serverState = SERVER_STATE_INGAME;

	auto gameGui = std::make_shared<cNewGameGUI> (staticMap);

	gameGui->setDynamicMap (client->getMap ());
	gameGui->setPlayer (&client->getActivePlayer ());

	gameGui->connectToClient (*client);

	application.show (gameGui);

	application.setGame (shared_from_this ());

	signalConnectionManager.connect (gameGui->terminated, [&]()
	{
		application.setGame (nullptr);
	});
}

void cLocalSingleplayerGameSaved::setSaveGameNumber (int saveGameNumber_)
{
	saveGameNumber = saveGameNumber_;
}