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

#ifndef game_servergameH
#define game_servergameH

#include "game/logic/server.h"
#include "game/startup/lobbyserver.h"
#include "utility/signal/signalconnectionmanager.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <SDL.h>

//------------------------------------------------------------------------
/** cServerGame handles all server side tasks of one multiplayer game in a thread.
 */
class cServerGame
{
public:
	cServerGame();
	~cServerGame();

	eOpenServerResult startServer(int port);
	int getPort() const { return port; }

	void runInThread();

	void prepareGameData();
	void loadGame (int saveGameNumber);
	void saveGame (int saveGameNumber);

	// retrieve state
	std::string getGameState() const;

	std::function<std::string()> getGamesString;
	std::function<std::string()> getAvailableMapsString;

	void handleChatCommand (int fromPlayer, const std::vector<std::string>& tokens);
private:
	friend int serverGameThreadFunction (void* data);
	void run();

private:
	cSignalConnectionManager signalConnectionManager;
	cLobbyServer lobbyServer;
	cServer* server = nullptr;

	int port = 0;

	bool shouldSave = false;
	int saveGameNumber = -1;

	SDL_Thread* thread = nullptr;
	std::atomic<bool> canceled{false};
};

#endif
