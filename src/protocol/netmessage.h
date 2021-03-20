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

#ifndef protocol_netmessageH
#define protocol_netmessageH

#include <memory>

#include "game/data/freezemode.h"
#include "game/data/player/playerbasicdata.h"
#include "game/data/report/savedreport.h"
#include "game/logic/gametimer.h"
#include "ui/graphical/game/gameguistate.h"
#include "utility/color.h"
#include "utility/serialization/serialization.h"
#include "utility/serialization/textarchive.h"
#include "utility/serialization/binaryarchive.h"

class cSavedReport;
class cSocket;

// When changing this enum, also update function enumToString (eNetMessageType value)!
enum class eNetMessageType {
	/** TCP messages generated by the connection manager:
	    Note: do not change the numerical values of the first 3 message, to make sure the game
		version check is working across different maxr versions */
	TCP_HELLO = 0,           /** sent from host to provide game version of the server (1st stage of the initial handshake) */
	TCP_WANT_CONNECT = 1,    /** sent from client to provide credentials (2nd stage of the initial handshake) */
	TCP_CONNECTED = 2,       /** sent from host, when the connection is accepted (3rd stage of the initial handshake) */
	TCP_CONNECT_FAILED,      /** local event, when server is not reachable. Sent from host, if connection is declined by host */
	TCP_CLOSE,               /** local event (generated by connection manager) to indicate a closed connection */

	MULTIPLAYER_LOBBY,    /** messages for multiplayer lobby and game preparation menus */
	ACTION,               /** the set of actions a client (AI or player) can trigger to influence the game */
	GAMETIME_SYNC_SERVER, /** sync message from server to clients */
	GAMETIME_SYNC_CLIENT, /** sync message from client to server */
	RANDOM_SEED,          /** initialize the synchronized random generator of the models */
	FREEZE_MODES,         /** Is the game running? And if it isn't, why not? */
	REPORT,               /** chat messages and other reports for the player */
	GUI_SAVE_INFO,        /** saved reports and gui settings */
	REQUEST_GUI_SAVE_INFO,/** requests the clients to send their gui data for saving */
	RESYNC_MODEL,         /** transfer a copy of the complete modeldata to clients */
	REQUEST_RESYNC_MODEL, /** instructs the server to send a copy of the model */
	GAME_ALREADY_RUNNING, /** send by server, when a new connection is established, after the game has started */
	WANT_REJOIN_GAME,     /** send by a client to reconnect a disconnected player */
};
std::string enumToString (eNetMessageType value);

//------------------------------------------------------------------------------
class cNetMessage
{
public:

	static std::unique_ptr<cNetMessage> createFromBuffer (const unsigned char* data, int length);

	virtual ~cNetMessage() {}

	eNetMessageType getType() const { return type; }
	std::unique_ptr<cNetMessage> clone() const;

	virtual void serialize (cBinaryArchiveIn& archive) { serializeThis (archive); }
	virtual void serialize (cTextArchiveIn& archive) { serializeThis (archive); }

	cNetMessage& From (int player) { playerNr = player; return *this; }

	int playerNr;

protected:
	cNetMessage (eNetMessageType type) : playerNr (-1), type (type) {}
private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & type;
		archive & playerNr;
	}

	eNetMessageType type;
};

//------------------------------------------------------------------------------
template <eNetMessageType MsgType>
class cNetMessageT : public cNetMessage
{
public:
	cNetMessageT() : cNetMessage(MsgType) {}
};

/**
* Interface called each time a message should be handled.
*/
class INetMessageHandler
{
public:
	virtual ~INetMessageHandler() = default;
	/**
	 * potentially handle the message
	 * @return if message is handled
	 */
	virtual bool handleMessage (const cNetMessage&) = 0;
};

//------------------------------------------------------------------------------
class cNetMessageTcpHello : public cNetMessageT<eNetMessageType::TCP_HELLO>
{
public:
	cNetMessageTcpHello();
	cNetMessageTcpHello (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	std::string packageVersion;
	std::string packageRev;

private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & packageVersion;
		archive & packageRev;
	}
};

//------------------------------------------------------------------------------
class cNetMessageTcpWantConnect : public cNetMessageT<eNetMessageType::TCP_WANT_CONNECT>
{
public:
	cNetMessageTcpWantConnect();
	cNetMessageTcpWantConnect (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	std::string playerName;
	cRgbColor playerColor;
	bool ready;
	std::string packageVersion;
	std::string packageRev;

	const cSocket* socket;

private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & playerName;
		archive & playerColor;
		archive & ready;
		archive & packageVersion;
		archive & packageRev;
		// socket is not serialized
	}
};

//------------------------------------------------------------------------------
class cNetMessageTcpConnected : public cNetMessageT<eNetMessageType::TCP_CONNECTED>
{
public:
	cNetMessageTcpConnected (int playerNr);
	cNetMessageTcpConnected (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	int playerNr;
	std::string packageVersion;
	std::string packageRev;

private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & playerNr;
		archive & packageVersion;
		archive & packageRev;
	}
};

//------------------------------------------------------------------------------
class cNetMessageTcpConnectFailed : public cNetMessageT<eNetMessageType::TCP_CONNECT_FAILED>
{
public:
	cNetMessageTcpConnectFailed (const std::string& reason = "") :
		reason (reason)
	{}
	cNetMessageTcpConnectFailed (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	std::string reason;
private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & reason;
	}
};

//------------------------------------------------------------------------------
class cNetMessageTcpClose : public cNetMessageT<eNetMessageType::TCP_CLOSE>
{
public:
	cNetMessageTcpClose (int playerNr_)
	{
		playerNr = playerNr_;
	}
};

//------------------------------------------------------------------------------
class cNetMessageSyncServer : public cNetMessageT<eNetMessageType::GAMETIME_SYNC_SERVER>
{
public:
	cNetMessageSyncServer() = default;
	cNetMessageSyncServer (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	unsigned int gameTime;
	unsigned int checksum;
	unsigned int ping;

private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & gameTime;
		archive & checksum;
		archive & ping;
	}
};

//------------------------------------------------------------------------------
class cNetMessageSyncClient : public cNetMessageT<eNetMessageType::GAMETIME_SYNC_CLIENT>
{
public:
	cNetMessageSyncClient() = default;
	cNetMessageSyncClient (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	unsigned int gameTime;

	//send debug data to server
	bool crcOK;
	unsigned int timeBuffer;
	unsigned int ticksPerFrame;
	unsigned int queueSize;
	unsigned int eventCounter;

private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & gameTime;
		archive & crcOK;
		archive & timeBuffer;
		archive & ticksPerFrame;
		archive & queueSize;
		archive & eventCounter;
	}
};


//------------------------------------------------------------------------------
class cNetMessageRandomSeed : public cNetMessageT<eNetMessageType::RANDOM_SEED>
{
public:
	explicit cNetMessageRandomSeed (uint64_t seed) : seed (seed)	{}
	cNetMessageRandomSeed (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	uint64_t seed;
private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & seed;
	}
};

//------------------------------------------------------------------------------
class cNetMessageFreezeModes : public cNetMessageT<eNetMessageType::FREEZE_MODES>
{
public:
	cNetMessageFreezeModes (const cFreezeModes& freezeModes, std::map<int, ePlayerConnectionState> playerStates) :
		freezeModes (freezeModes),
		playerStates (playerStates)
	{}
	cNetMessageFreezeModes (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	cFreezeModes freezeModes;
	std::map<int, ePlayerConnectionState> playerStates;
private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & freezeModes;
		archive & playerStates;
	}
};

//------------------------------------------------------------------------------
class cNetMessageReport : public cNetMessageT<eNetMessageType::REPORT>
{
public:
	cNetMessageReport() = default;
	cNetMessageReport (std::unique_ptr<cSavedReport> report) :
		report (std::move (report))
	{}

	cNetMessageReport (cBinaryArchiveOut& archive)
	{
		loadThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); saveThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); saveThis (archive); }

	std::unique_ptr<cSavedReport> report;

private:
	template <typename T>
	void loadThis (T& archive)
	{
		report = cSavedReport::createFrom (archive);
	}
	template <typename T>
	void saveThis (T& archive)
	{
		archive << *report;
	}
};

//------------------------------------------------------------------------------
class cNetMessageGUISaveInfo : public cNetMessageT<eNetMessageType::GUI_SAVE_INFO>
{
public:
	cNetMessageGUISaveInfo (int savingID) :
		reports (nullptr),
		savingID (savingID)
	{}
	cNetMessageGUISaveInfo (cBinaryArchiveOut& archive)
	{
		loadThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); saveThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); saveThis (archive); }

	std::shared_ptr<std::vector<std::unique_ptr<cSavedReport>>> reports;
	cGameGuiState guiState;
	std::array<std::pair<bool, cPosition>, 4> savedPositions;
	std::vector<unsigned int> doneList;
	int savingID;
private:
	template <typename T>
	void loadThis (T& archive)
	{
		if (reports == nullptr)
			reports = std::make_shared<std::vector<std::unique_ptr<cSavedReport>>>();

		int size;
		archive >> size;
		reports->resize (size);
		for (auto& report : *reports)
		{
			report = cSavedReport::createFrom (archive);
		}
		archive >> guiState;
		archive >> savingID;
		archive >> savedPositions;
		archive >> doneList;
	}
	template <typename T>
	void saveThis (T& archive)
	{
		if (reports == nullptr)
			reports = std::make_shared<std::vector<std::unique_ptr<cSavedReport>>>();

		archive << (int)reports->size();
		for (auto& report : *reports)
		{
			archive << *report;
		}
		archive << guiState;
		archive << savingID;
		archive << savedPositions;
		archive << doneList;
	}
};

//------------------------------------------------------------------------------
class cNetMessageRequestGUISaveInfo : public cNetMessageT<eNetMessageType::REQUEST_GUI_SAVE_INFO>
{
public:
	cNetMessageRequestGUISaveInfo (int savingID) :
		savingID (savingID)
	{}
	cNetMessageRequestGUISaveInfo (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	int savingID;
private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & savingID;
	}
};

//------------------------------------------------------------------------------
class cNetMessageResyncModel : public cNetMessageT<eNetMessageType::RESYNC_MODEL>
{
public:
	cNetMessageResyncModel (const cModel& model);
	cNetMessageResyncModel (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}
	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	void apply (cModel& model) const;
private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & data;
	}

	std::vector<uint8_t> data;
};

//------------------------------------------------------------------------------
class cNetMessageRequestResync : public cNetMessageT<eNetMessageType::REQUEST_RESYNC_MODEL>
{
public:
	cNetMessageRequestResync (int playerToSync = -1, int saveNumberForGuiInfo = -1) :
		playerToSync (playerToSync),
		saveNumberForGuiInfo (saveNumberForGuiInfo)
	{}
	cNetMessageRequestResync (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	int playerToSync;         // playerNr who will receive the data. -1 for all connected players
	int saveNumberForGuiInfo; // number of save game file, from which gui info will be loaded. -1 disables loading gui data

private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & playerToSync;
		archive & saveNumberForGuiInfo;
	}
};

//------------------------------------------------------------------------------
class cNetMessageGameAlreadyRunning : public cNetMessageT<eNetMessageType::GAME_ALREADY_RUNNING>
{
public:
	cNetMessageGameAlreadyRunning (const cModel& model);
	cNetMessageGameAlreadyRunning (cBinaryArchiveOut& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cNetMessage::serialize (archive); serializeThis (archive); }

	std::string mapName;
	uint32_t mapCrc;
	std::vector<cPlayerBasicData> playerList;

private:
	template <typename T>
	void serializeThis (T& archive)
	{
		archive & mapName;
		archive & mapCrc;
		archive & playerList;
	}
};

//------------------------------------------------------------------------------
class cNetMessageWantRejoinGame : public cNetMessageT<eNetMessageType::WANT_REJOIN_GAME>
{
public:
	cNetMessageWantRejoinGame() = default;
	cNetMessageWantRejoinGame (cBinaryArchiveOut& archive)
	{}
};

#endif
