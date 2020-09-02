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

#include "network.h"

#include "connectionmanager.h"
#include "game/data/player/playerbasicdata.h"
#include "utility/log.h"
#include "protocol/netmessage.h"
#include "maxrversion.h"
#include "utility/string/toString.h"
#include "protocol/lobbymessage.h"

#define HANDSHAKE_TIMEOUT_MS 3000
#define DISABLE_TIMEOUTS 0 // this can be used, when debugging the connection handshake
                           // and timeouts are not wanted

class cHandshakeTimeout
{
public:
	cHandshakeTimeout(const cSocket* socket, cConnectionManager* connectionManager) :
		connectionManager(connectionManager),
		socket(socket)
	{
		timer = SDL_AddTimer(HANDSHAKE_TIMEOUT_MS, callback, (void*)this);
	}

	~cHandshakeTimeout()
	{
		SDL_RemoveTimer(timer);
	}

	const cSocket* getSocket() const
	{
		return socket;
	}
private:
	static uint32_t callback(uint32_t intervall, void* arg)
	{
		cHandshakeTimeout* thisPtr = reinterpret_cast<cHandshakeTimeout*> (arg);
		thisPtr->connectionManager->handshakeTimeoutCallback(*thisPtr);

		return 0;
	}

	cConnectionManager* connectionManager = nullptr;
	SDL_TimerID timer;
	const cSocket* socket = nullptr;
};


//------------------------------------------------------------------------------
cConnectionManager::cConnectionManager() = default;

//------------------------------------------------------------------------------
cConnectionManager::~cConnectionManager() = default;

//------------------------------------------------------------------------------
int cConnectionManager::openServer(int port)
{
	assert(localServer != nullptr);

	cLockGuard<cMutex> tl(mutex);

	if (serverOpen) return -1;

	if (!network)
		network = std::make_unique<cNetwork>(*this, mutex);

	int result = network->openServer(port);
	if (result == 0)
		serverOpen = true;

	return result;
}

//------------------------------------------------------------------------------
void cConnectionManager::closeServer()
{
	cLockGuard<cMutex> tl(mutex);

	if (!network || !serverOpen) return;

	network->closeServer();
	serverOpen = false;
}

//------------------------------------------------------------------------------
bool cConnectionManager::isServerOpen() const
{
	return serverOpen;
}

//------------------------------------------------------------------------------
void cConnectionManager::acceptConnection(const cSocket* socket, int playerNr)
{
	assert(localServer != nullptr);

	cLockGuard<cMutex> tl(mutex);

	stopTimeout(socket);

	auto x = std::find_if(clientSockets.begin(), clientSockets.end(), [&](const std::pair<const cSocket*, int>& x) { return x.first == socket; });
	if (x == clientSockets.end())
	{
		//looks like the connection was disconnected during the handshake
		Log.write("ConnectionManager: accept called for unknown socket", cLog::eLOG_TYPE_NET_WARNING);
		localServer->pushMessage(std::make_unique<cNetMessageTcpClose>(playerNr));

		return;
	}

	Log.write("ConnectionManager: Accepted connection and assigned playerNr: " + toString(playerNr), cLog::eLOG_TYPE_NET_DEBUG);

	//assign playerNr to the socket
	x->second = playerNr;

	cNetMessageTcpConnected message(playerNr);

	cTextArchiveIn archive;
	archive << message;
	Log.write("ConnectionManager: --> " + archive.data(), cLog::eLOG_TYPE_NET_DEBUG);

	sendMessage(socket, message);
}

//------------------------------------------------------------------------------
void cConnectionManager::declineConnection(const cSocket* socket, const std::string& reason)
{
	assert(localServer != nullptr);

	cLockGuard<cMutex> tl(mutex);

	stopTimeout(socket);

	auto x = std::find_if(clientSockets.begin(), clientSockets.end(), [&](const std::pair<const cSocket*, int>& x) { return x.first == socket; });
	if (x == clientSockets.end())
	{
		//looks like the connection was disconnected during the handshake
		Log.write("ConnectionManager: decline called for unknown socket", cLog::eLOG_TYPE_NET_WARNING);
		return;
	}

	cNetMessageTcpConnectFailed message(reason);

	cTextArchiveIn archive;
	archive << message;
	Log.write("ConnectionManager: --> " + archive.data(), cLog::eLOG_TYPE_NET_DEBUG);

	sendMessage(socket, message);

	network->close(socket);
}

//------------------------------------------------------------------------------
void cConnectionManager::connectToServer(const std::string& host, int port, const cPlayerBasicData& player)
{
	assert(localClient != nullptr);

	cLockGuard<cMutex> tl(mutex);

	if (!network)
		network = std::make_unique<cNetwork>(*this, mutex);

	Log.write("ConnectionManager: Connecting to " + host + ":" + toString(port), cLog::eLOG_TYPE_NET_DEBUG);

	network->connectToServer(host, port);

	//save credentials, until the connection is established
	connecting = true;
	connectingPlayerName = player.getName();
	connectingPlayerColor = player.getColor().getColor();
	connectingPlayerReady = player.isReady();
}

//------------------------------------------------------------------------------
bool cConnectionManager::isConnectedToServer() const
{
	if (localServer) return true;
	cLockGuard<cMutex> tl(mutex);

	return connecting || serverSocket != nullptr;
}

//------------------------------------------------------------------------------
void cConnectionManager::changePlayerNumber(int currentNr, int newNr)
{
	cLockGuard<cMutex> tl(mutex);

	if (localPlayer == currentNr)
	{
		localPlayer = newNr;
		return;
	}

	auto x = std::find_if(clientSockets.begin(), clientSockets.end(), [&](const std::pair<const cSocket*, int>& x) { return x.second == currentNr; });
	if (x == clientSockets.end())
	{
		Log.write("Connection Manager: Can't change playerNr. Unknown player " + toString(currentNr), cLog::eLOG_TYPE_NET_ERROR);
		return;
	}
	x->second = newNr;
}

//------------------------------------------------------------------------------
void cConnectionManager::setLocalClient(INetMessageReceiver* client, int playerNr)
{
	cLockGuard<cMutex> tl(mutex);

	//move pending messages in the event queue to new receiver
	if (localClient && client)
	{
		while (auto message = localClient->popMessage())
		{
			client->pushMessage(std::move(message));
		}
	}

	localPlayer = playerNr;
	localClient = client;
}

//------------------------------------------------------------------------------
void cConnectionManager::setLocalServer(INetMessageReceiver* server)
{
	cLockGuard<cMutex> tl(mutex);

	//move pending messages in the event queue to new receiver
	if (localServer && server)
	{
		while (auto message = localServer->popMessage())
		{
			server->pushMessage(std::move(message));
		}
	}

	localServer = server;
}

//------------------------------------------------------------------------------
void cConnectionManager::setLocalClients(std::vector<INetMessageReceiver*>&& clients)
{
	localClients = std::move(clients);
}

//------------------------------------------------------------------------------
void cConnectionManager::sendToServer(const cNetMessage2& message)
{
	cLockGuard<cMutex> tl(mutex);

	if (localServer)
	{
		localServer->pushMessage(message.clone());
	}
	else if (serverSocket)
	{
		sendMessage(serverSocket, message);
	}
	else
	{
		Log.write("Connection Manager: Can't send message. No local server and no connection to server", cLog::eLOG_TYPE_NET_ERROR);
	}
}

//------------------------------------------------------------------------------
void cConnectionManager::sendToPlayer(const cNetMessage2& message, int playerNr)
{
	cLockGuard<cMutex> tl(mutex);

	if (playerNr == localPlayer)
	{
		localClient->pushMessage(message.clone());
		return;
	}
	else if (static_cast<std::size_t>(playerNr) < localClients.size())
	{
		localClients[playerNr]->pushMessage(message.clone());
	}
	else
	{
		auto x = std::find_if(clientSockets.begin(), clientSockets.end(), [&](const std::pair<const cSocket*, int>& x) { return x.second == playerNr; });
		if (x == clientSockets.end())
		{
			Log.write("Connection Manager: Can't send message. No connection to player " + toString(playerNr), cLog::eLOG_TYPE_NET_ERROR);
			return;
		}

		sendMessage(x->first, message);
	}
}

//------------------------------------------------------------------------------
void cConnectionManager::sendToPlayers(const cNetMessage2& message)
{
	cLockGuard<cMutex> tl(mutex);

	if (localPlayer != -1)
	{
		localClient->pushMessage(message.clone());
	}
	for (auto& client : localClients)
	{
		client->pushMessage(message.clone());
	}

	// serialize...
	std::vector<unsigned char> buffer;
	cBinaryArchiveIn archive(buffer);
	archive << message;

	for (const auto& client : clientSockets)
	{
		network->sendMessage(client.first, buffer.size(), buffer.data());
	}
}

//------------------------------------------------------------------------------
void cConnectionManager::disconnect(int player)
{
	cLockGuard<cMutex> tl(mutex);

	auto x = std::find_if(clientSockets.begin(), clientSockets.end(), [&](const std::pair<const cSocket*, int>& x) { return x.second == player; });
	if (x == clientSockets.end())
	{
		Log.write("ConnectionManager: Can't disconnect player. No connection to player " + toString(player), cLog::eLOG_TYPE_NET_ERROR);
		return;
	}

	network->close(x->first);
}

//------------------------------------------------------------------------------
void cConnectionManager::disconnectAll()
{
	cLockGuard<cMutex> tl(mutex);

	if (serverSocket)
	{
		network->close(serverSocket);
	}

	while (clientSockets.size() > 0)
	{
		//erease in loop
		network->close(clientSockets[0].first);
	}
}

//------------------------------------------------------------------------------
void cConnectionManager::connectionClosed(const cSocket* socket)
{
	stopTimeout(socket);

	if (socket == serverSocket)
	{
		if (localClient)
		{
			localClient->pushMessage(std::make_unique<cNetMessageTcpClose>(-1));
		}
		serverSocket = nullptr;
	}
	else
	{
		auto x = std::find_if(clientSockets.begin(), clientSockets.end(), [&](const std::pair<const cSocket*, int>& x) { return x.first == socket; });
		if (x == clientSockets.end())
		{
			Log.write("ConnectionManager: An unknown connection was closed", cLog::eLOG_TYPE_NET_ERROR);
			return;
		}

		int playerNr = x->second;
		if (playerNr != -1 && localServer) //is a player associated with the socket?
		{
			localServer->pushMessage(std::make_unique<cNetMessageTcpClose>(playerNr));
		}

		clientSockets.erase(x);
	}
}

//------------------------------------------------------------------------------
void cConnectionManager::incomingConnection(const cSocket* socket)
{
	startTimeout(socket);

	std::pair<const cSocket*, int> connection;
	connection.first = socket;
	connection.second = -1;
	clientSockets.push_back(connection);

	cNetMessageTcpHello message;

	cTextArchiveIn archive;
	archive << message;
	Log.write("ConnectionManager: --> " + archive.data(), cLog::eLOG_TYPE_NET_DEBUG);

	sendMessage(socket, message);
}

//------------------------------------------------------------------------------
int cConnectionManager::sendMessage(const cSocket* socket, const cNetMessage2& message)
{
	// serialize...
	std::vector<unsigned char> buffer;
	cBinaryArchiveIn archive(buffer);
	archive << message;

	return network->sendMessage(socket, buffer.size(), buffer.data());
}

//------------------------------------------------------------------------------
void cConnectionManager::messageReceived(const cSocket* socket, unsigned char* data, int length)
{
	std::unique_ptr<cNetMessage2> message;
	try
	{
		message = cNetMessage2::createFromBuffer(data, length);
	}
	catch (std::runtime_error& e)
	{
		Log.write((std::string)"ConnectionManager: Can't deserialize net message: " + e.what(), cLog::eLOG_TYPE_NET_ERROR);
		return;
	}

	//compare the sender playerNr of the message with the playerNr that is expected behind the socket
	int playerOnSocket = -1;
	auto x = std::find_if(clientSockets.begin(), clientSockets.end(), [&](const std::pair<const cSocket*, int>& x) { return x.first == socket; });
	if (x != clientSockets.end())
	{
		playerOnSocket = x->second;
		if (message->playerNr != playerOnSocket)
		{
			Log.write("ConnectionManager: Discarding message with wrong sender id", cLog::eLOG_TYPE_NET_WARNING);
			return;
		}
	}

	// handle messages for the connection handshake
	if (handeConnectionHandshake(message, socket, playerOnSocket))
	{
		return;
	}

	if (localServer)
	{
		localServer->pushMessage(std::move(message));
	}
	else if (localClient)
	{
		localClient->pushMessage(std::move(message));
	}
	else
	{
		Log.write("ConnectionManager: Cannot handle message. No message receiver.", cLog::eLOG_TYPE_NET_ERROR);
	}
}

bool cConnectionManager::handeConnectionHandshake(const std::unique_ptr<cNetMessage2> &message, const cSocket* socket, int playerOnSocket)
{
	switch (message->getType())
	{
	case eNetMessageType::TCP_HELLO:
	{
		cTextArchiveIn archive;
		archive << *message;
		Log.write("ConnectionManager: <-- " + archive.data(), cLog::eLOG_TYPE_NET_DEBUG);

		if (localServer)
		{
			// server shouldn't get this message
			return true;
		}

		//check compatible game version
		const auto& msgTcpHello = static_cast<cNetMessageTcpHello&>(*message);
		if (msgTcpHello.packageVersion != PACKAGE_VERSION)
		{
			localClient->pushMessage(std::make_unique<cMuMsgChat>("Text~Multiplayer~Gameversion_Error", true, msgTcpHello.packageVersion));
			localClient->pushMessage(std::make_unique<cMuMsgChat>("Text~Multiplayer~Gameversion_Own", true, PACKAGE_VERSION));
			network->close(socket);
			return true;
		}


		cNetMessageTcpWantConnect response;
		response.playerName = connectingPlayerName;
		response.playerColor = connectingPlayerColor;
		response.ready = connectingPlayerReady;

		cTextArchiveIn archiveResponse;
		archiveResponse << response;
		Log.write("ConnectionManager: --> " + archiveResponse.data(), cLog::eLOG_TYPE_NET_DEBUG);

		sendMessage(socket, response);
		return true;
	}
	case eNetMessageType::TCP_WANT_CONNECT:
	{
		cTextArchiveIn archive;
		archive << *message;
		Log.write("ConnectionManager: <-- " + archive.data(), cLog::eLOG_TYPE_NET_DEBUG);

		if (!localServer)
		{
			// clients shouldn't get this message
			return true;
		}

		if (playerOnSocket != -1)
		{
			Log.write("ConnectionManager: Received TCP_WANT_CONNECT from alreay connected player", cLog::eLOG_TYPE_NET_ERROR);
			return true;
		}

		auto& msgTcpWantConnect = static_cast<cNetMessageTcpWantConnect&>(*message);
		msgTcpWantConnect.socket = socket;

		//check compatible game version
		if (msgTcpWantConnect.packageVersion != PACKAGE_VERSION)
		{
			sendMessage(socket, cMuMsgChat("Text~Multiplayer~Gameversion_Error", true, PACKAGE_VERSION));
			sendMessage(socket, cMuMsgChat("Text~Multiplayer~Gameversion_Own", true, msgTcpWantConnect.packageVersion));
			network->close(socket);
			return true;
		}
		break;
	}
	case eNetMessageType::TCP_CONNECTED:
	{
		if (localServer)
		{
			// server shouldn't get this message
			return true;
		}
		cTextArchiveIn archive;
		archive << *message;
		Log.write("ConnectionManager: <-- " + archive.data(), cLog::eLOG_TYPE_NET_DEBUG);

		stopTimeout(socket);

		//TODO: set localPlayerNr?
		break;
	}
	default:
		break;
	}

	return false;
}

//------------------------------------------------------------------------------
bool cConnectionManager::isPlayerConnected(int playerNr) const
{
	if (playerNr == localPlayer || static_cast<std::size_t>(playerNr) < localClients.size())
	{
		return true;
	}

	auto it = std::find_if(clientSockets.begin(), clientSockets.end(), [&](const std::pair<const cSocket*, int>& p) { return p.second == playerNr; });
	return it != clientSockets.end();
}

//------------------------------------------------------------------------------
void cConnectionManager::connectionResult(const cSocket* socket)
{
	assert(localClient);
	if (socket != nullptr)
	{
		startTimeout(socket);
	}

	connecting = false;
	serverSocket = socket;

	if (socket == nullptr)
	{
		Log.write("ConnectionManager: Connect to server failed", cLog::eLOG_TYPE_NET_WARNING);
		auto message = std::make_unique<cNetMessageTcpConnectFailed>();
		localClient->pushMessage(std::move(message));
	}
}

//------------------------------------------------------------------------------
void cConnectionManager::startTimeout(const cSocket* socket)
{
#if DISABLE_TIMEOUTS == 0
	timeouts.push_back(std::make_unique<cHandshakeTimeout>(socket, this));
#endif
}

//------------------------------------------------------------------------------
void cConnectionManager::stopTimeout(const cSocket* socket)
{
	auto t = std::find_if(timeouts.begin(), timeouts.end(), [&](const auto& timer) { return timer->getSocket() == socket; });
	if (t != timeouts.end())
	{
		timeouts.erase(t);
	}
}

//------------------------------------------------------------------------------
void cConnectionManager::handshakeTimeoutCallback(cHandshakeTimeout& timer)
{
	cLockGuard<cMutex> tl(mutex);

	Log.write("ConnectionManager: Handshake timed out", cLog::eLOG_TYPE_NET_WARNING);

	auto it = std::find_if(timeouts.begin(), timeouts.end(), [&](const auto& timeout){ return timeout.get() == &timer;});
	if (it != timeouts.end())
	{
		network->close(timer.getSocket());
		timeouts.erase(it);
	}
}
