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

#include "networkgame.h"

#include "game/logic/client.h"
#include "game/startup/initplayerdata.h"
#include "ui/widgets/application.h"

//------------------------------------------------------------------------------
cNetworkGame::~cNetworkGame()
{
	// if (server)
	// {
	//     server->stop();
	// }
}

//------------------------------------------------------------------------------
void cNetworkGame::run()
{
}

//------------------------------------------------------------------------------
void cNetworkGame::startNewGame (cApplication& application, std::shared_ptr<cClient> client, const sInitPlayerData& initPlayerData, cServer* server)
{
	client->initNewGame (initPlayerData);

	gameGuiController = std::make_unique<cGameGuiController> (application, client->getModel().getMap()->staticMap);
	gameGuiController->setSingleClient (client);
	gameGuiController->setServer (server);

	cGameGuiState playerGameGuiState;
	playerGameGuiState.mapPosition = initPlayerData.landingPosition;
	gameGuiController->addPlayerGameGuiState (client->getActivePlayer().getId(), std::move (playerGameGuiState));

	gameGuiController->start();

	resetTerminating();

	application.addRunnable (shared_from_this());

	signalConnectionManager.connect (gameGuiController->terminated, [this]() { exit(); });
}

//------------------------------------------------------------------------------
void cNetworkGame::start (cApplication& application, std::shared_ptr<cClient> client, cServer* server)
{
	gameGuiController = std::make_unique<cGameGuiController> (application, client->getModel().getMap()->staticMap);
	gameGuiController->setSingleClient (client);
	gameGuiController->setServer (server);
	gameGuiController->start();

	resetTerminating();

	application.addRunnable (shared_from_this());

	signalConnectionManager.connect (gameGuiController->terminated, [this]() { exit(); });
}
