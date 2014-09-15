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
#include <cmath>
#include <sstream>

#include "game/logic/client.h"

#include "game/logic/attackjobs.h"
#include "game/logic/automjobs.h"
#include "game/data/units/building.h"
#include "game/logic/casualtiestracker.h"
#include "game/logic/clientevents.h"
#include "utility/listhelpers.h"
#include "game/logic/fxeffects.h"
#include "game/logic/gametimer.h"
#include "game/logic/jobs.h"
#include "game/logic/automjobs.h"
#include "utility/log.h"
#include "main.h"
#include "netmessage.h"
#include "game/data/player/player.h"
#include "game/logic/server.h"
#include "game/logic/serverevents.h"
#include "settings.h"
#include "game/data/units/vehicle.h"
#include "video.h"
#include "ui/graphical/menu/windows/windowgamesettings/gamesettings.h"
#include "ui/graphical/game/gameguistate.h"
#include "game/data/report/savedreportchat.h"
#include "game/data/report/savedreportsimple.h"
#include "game/data/report/unit/savedreportdisabled.h"
#include "game/data/report/unit/savedreportpathinterrupted.h"
#include "game/data/report/unit/savedreportcapturedbyenemy.h"
#include "game/data/report/unit/savedreportdetected.h"
#include "game/data/report/unit/savedreportpathinterrupted.h"
#include "game/data/report/special/savedreportplayerendedturn.h"
#include "game/data/report/special/savedreportplayerdefeated.h"
#include "game/data/report/special/savedreportplayerleft.h"
#include "game/data/report/special/savedreportupgraded.h"
#include "game/logic/turnclock.h"
#include "game/logic/turntimeclock.h"

using namespace std;

//------------------------------------------------------------------------
// cClient implementation
//------------------------------------------------------------------------

//------------------------------------------------------------------------
cClient::cClient (cServer* server_, std::shared_ptr<cTCP> network_) :
	server (server_),
	network (std::move (network_)),
	gameTimer (std::make_shared<cGameTimerClient> ()),
	ActivePlayer (NULL),
	turnClock (std::make_shared<cTurnClock> (1)),
	turnTimeClock (std::make_shared<cTurnTimeClock> (gameTimer)),
	gameSettings (std::make_shared<cGameSettings> ()),
	casualtiesTracker (std::make_shared<cCasualtiesTracker> ()),
	effectsList (new cFxContainer)
{
	assert (server != nullptr || network != nullptr);

	gameTimer->setClient (this);
	if (server) server->addLocalClient (*this);
	else network->setMessageReceiver (this);
	bDefeated = false;
	bWantToEnd = false;

	gameTimer->start ();

	signalConnectionManager.connect (gameSettings->turnEndDeadlineChanged, [this]()
	{
		if (turnEndDeadline)
		{
			turnEndDeadline->changeDeadline (gameSettings->getTurnEndDeadline ());
		}
	});

	signalConnectionManager.connect (gameSettings->turnEndDeadlineActiveChanged, [this]()
	{
		if (!gameSettings->isTurnEndDeadlineActive () && turnEndDeadline)
		{
			turnTimeClock->removeDeadline (turnEndDeadline);
			turnEndDeadline = nullptr;
		}
	});

	signalConnectionManager.connect (gameSettings->turnLimitChanged, [this]()
	{
		if (turnLimitDeadline)
		{
			turnLimitDeadline->changeDeadline (gameSettings->getTurnLimit ());
		}
	});

	signalConnectionManager.connect (gameSettings->turnLimitActiveChanged, [this]()
	{
		if (!gameSettings->isTurnLimitActive () && turnLimitDeadline)
		{
			turnTimeClock->removeDeadline (turnLimitDeadline);
			turnLimitDeadline = nullptr;
		}
		else if (gameSettings->isTurnLimitActive () && !turnLimitDeadline)
		{
			turnLimitDeadline = turnTimeClock->startNewDeadlineFrom (turnTimeClock->getStartGameTime (), gameSettings->getTurnLimit ());
		}
	});
}

cClient::~cClient()
{
	if (network != nullptr) network->setMessageReceiver (nullptr);

	gameTimer->stop();

	for (unsigned int i = 0; i < attackJobs.size(); i++)
	{
		delete attackJobs[i];
	}
	neutralBuildings.clear ();

	// since currently the vehicles do own movejobs and other stuff that
	// have non owning references to the client, we delete all units in the players
	// and hopefully eliminate all those references by doing so...
	for (size_t i = 0; i < playerList.size (); ++i)
	{
		playerList[i]->removeAllUnits ();
	}
}

void cClient::setMap (std::shared_ptr<cStaticMap> staticMap)
{
	Map = std::make_shared<cMap>(staticMap);
	initPlayersWithMap();
}

void cClient::setGameSettings (const cGameSettings& gameSettings_)
{
	*gameSettings = gameSettings_;
}

class LessByNr
{
public:
	bool operator() (const cPlayer* lhs, const cPlayer* rhs) const
	{
		return lhs->getNr() < rhs->getNr();
	}
};

void cClient::setPlayers (const std::vector<cPlayerBasicData>& splayers, size_t activePlayerIndex)
{
	assert (activePlayerIndex < splayers.size ());

	for (size_t i = 0, size = splayers.size(); i != size; ++i)
	{
		playerList.push_back(std::make_shared<cPlayer>(splayers[i]));
	}

	ActivePlayer = playerList[activePlayerIndex].get();

	//std::sort(PlayerList.begin(), PlayerList.end(), LessByNr());

	initPlayersWithMap();
}

void cClient::initPlayersWithMap()
{
	if (playerList.empty() || Map == NULL) return;

	for (size_t i = 0; i != playerList.size (); ++i)
	{
		playerList[i]->initMaps (*Map);
	}

	// generate subbase for enemy players
	for (size_t i = 0; i != playerList.size (); ++i)
	{
		if (playerList[i].get() == ActivePlayer) continue;
		playerList[i]->base.SubBases.push_back (new sSubBase (playerList[i].get()));
	}
}

/*virtual*/ void cClient::pushEvent (std::unique_ptr<cNetMessage> message)
{
	if (message->iType == NET_GAME_TIME_SERVER)
	{
		// this is a preview for the client to know
		// how many sync messages are in queue
		// used to detect a growing lag behind the server time
		message->popInt32();
		unsigned int receivedTime = message->popInt32();
		message->rewind();

		gameTimer->setReceivedTime (receivedTime);
	}
	eventQueue.push (std::move(message));
}

void cClient::sendNetMessage (cNetMessage* message_) const
{
	// TODO: sendNetMessage should take a unique_ptr directly!
	std::unique_ptr<cNetMessage> message (message_);

	message->iPlayerNr = ActivePlayer->getNr();

	if (message->iType != NET_GAME_TIME_CLIENT)
	{
		Log.write ("Client: " + getActivePlayer ().getName () + " --> "
				   + message->getTypeAsString ()
				   + ", gameTime:" + iToStr (this->gameTimer->gameTime)
				   + ", Hexdump: " + message->getHexDump (), cLog::eLOG_TYPE_NET_DEBUG);
	}

	if (server)
	{
		// push an event to the local server in singleplayer, HotSeat or
		// if this machine is the host
		server->pushEvent (std::move(message));
	}
	else // else send it over the net
	{
		//the client is only connected to one socket
		//so network->send() only sends to the server
		network->send (message->iLength, message->serialize());
	}
}

bool cClient::addMoveJob (cVehicle& vehicle, const cPosition& destination, const std::vector<cVehicle*>* group)
{
	sWaypoint* path = cClientMoveJob::calcPath (*getMap(), vehicle.getPosition(), destination, vehicle, group);
	if (path)
	{
		sendMoveJob (*this, path, vehicle.iID);
		Log.write (" Client: Added new movejob: VehicleID: " + iToStr (vehicle.iID) + ", SrcX: " + iToStr (vehicle.getPosition().x()) + ", SrcY: " + iToStr (vehicle.getPosition().y()) + ", DestX: " + iToStr (destination.x()) + ", DestY: " + iToStr (destination.y()), cLog::eLOG_TYPE_NET_DEBUG);
		return true;
	}
	else
	{
		moveJobBlocked (vehicle);
		return false;
	}
}

void cClient::startGroupMove(const std::vector<cVehicle*>& group_, const cPosition& mainDestination)
{
	const auto& mainPosition = group_[0]->getPosition();

	// copy the selected-units-list
	std::vector<cVehicle*> group = group_;

	// go trough all vehicles in the list
	while (group.size())
	{
		// we will start moving the vehicles in the list with the vehicle
		// that is the closest to the destination.
		// this will avoid that the units will crash into each other
		// because the one infront of them has started
		// his move and the next field is free.
		// TODO: sort group by ditance, then proceed.
		int shortestWaySquareLength = 0x7FFFFFFF;
		int shortestWayVehNum = 0;
		for (unsigned int i = 0; i < group.size(); i++)
		{
			cVehicle* vehicle = group[i];
			const auto delta = vehicle->getPosition() + mainDestination + vehicle->getPosition() - mainPosition;
			const int waySquareLength = delta.l2NormSquared();

			if (waySquareLength < shortestWaySquareLength)
			{
				shortestWaySquareLength = waySquareLength;
				shortestWayVehNum = i;
			}
		}
		cVehicle& vehicle = *group[shortestWayVehNum];
		// add the movejob to the destination of the unit.
		// the formation of the vehicle group will stay as destination formation.
		auto destination = mainDestination + vehicle.getPosition() - mainPosition;
		addMoveJob(vehicle, destination, &group_);
		// delete the unit from the copyed list
		group.erase (group.begin() + shortestWayVehNum);
	}
}

void cClient::runFx()
{
	effectsList->run ();
}

void cClient::addFx (std::shared_ptr<cFx> fx)
{
	effectsList->push_back (fx);
	addedEffect (fx);
}

void cClient::HandleNetMessage_TCP_CLOSE (cNetMessage& message)
{
	assert (message.iType == TCP_CLOSE);

	network->close (message.popInt16());
	//TODO: gameGUI
	//gameGUI->onLostConnection();
}

void cClient::HandleNetMessage_GAME_EV_PLAYER_CLANS (cNetMessage& message)
{
	assert (message.iType == GAME_EV_PLAYER_CLANS);

	for (unsigned int i = 0; i < getPlayerList().size(); i++)
	{
		const int playerNr = message.popChar();
		const int clan = message.popChar();

		cPlayer* player = getPlayerFromNumber (playerNr);
		player->setClan (clan);
	}
}

void cClient::HandleNetMessage_GAME_EV_ADD_BUILDING (cNetMessage& message)
{
	assert (message.iType == GAME_EV_ADD_BUILDING);

	/*const bool Init =*/ message.popBool();
	cPlayer* Player = getPlayerFromNumber (message.popInt16());
	if (!Player)
	{
		Log.write ("Player not found", cLog::eLOG_TYPE_NET_ERROR);
		return;
	}
	const sID UnitID = message.popID();
	const auto position = message.popPosition();
	unsigned int ID = message.popInt16();
	auto& addedBuilding = Player->addNewBuilding(position, UnitID, ID);

	addUnit (position, addedBuilding);

	Player->base.addBuilding (&addedBuilding, NULL);
}

void cClient::HandleNetMessage_GAME_EV_ADD_VEHICLE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_ADD_VEHICLE);

	/*const bool Init =*/ message.popBool();
	cPlayer* Player = getPlayerFromNumber (message.popInt16());
	if (!Player)
	{
		Log.write ("Player not found", cLog::eLOG_TYPE_NET_ERROR);
		return;
	}
	const sID UnitID = message.popID ();
	const auto position = message.popPosition ();
	const unsigned int ID = message.popInt16();
	const bool addToMap = message.popBool();

	auto& addedVehicle = Player->addNewVehicle(position, UnitID, ID);
	addUnit (position, addedVehicle, addToMap);
}

void cClient::HandleNetMessage_GAME_EV_DEL_BUILDING (cNetMessage& message)
{
	assert (message.iType == GAME_EV_DEL_BUILDING);

	cBuilding* Building = getBuildingFromID (message.popInt16());

	if (Building)
	{
		deleteUnit (Building);
	}
}

void cClient::HandleNetMessage_GAME_EV_DEL_VEHICLE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_DEL_VEHICLE);

	cVehicle* Vehicle = getVehicleFromID (message.popInt16());

	if (Vehicle) deleteUnit (Vehicle);
}

void cClient::HandleNetMessage_GAME_EV_ADD_ENEM_VEHICLE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_ADD_ENEM_VEHICLE);

	cPlayer* Player = getPlayerFromNumber (message.popInt16());
	if (!Player)
	{
		Log.write ("Player not found", cLog::eLOG_TYPE_NET_ERROR);
		return;
	}
	const sID UnitID = message.popID ();
	const auto position = message.popPosition ();
	const int dir = message.popInt16();
	const int ID = message.popInt16();
	const int version = message.popInt16();
	auto& addedVehicle = Player->addNewVehicle (position, UnitID, ID);

	addedVehicle.dir = dir;
	addedVehicle.data.setVersion (version);
	addUnit (position, addedVehicle);
}

void cClient::HandleNetMessage_GAME_EV_ADD_ENEM_BUILDING (cNetMessage& message)
{
	assert (message.iType == GAME_EV_ADD_ENEM_BUILDING);

	cPlayer* Player = getPlayerFromNumber (message.popInt16());
	if (!Player)
	{
		Log.write ("Player not found", cLog::eLOG_TYPE_NET_ERROR);
		return;
	}
	const sID UnitID = message.popID ();
	const auto position = message.popPosition ();
	const int ID = message.popInt16();
	const int version = message.popInt16();
	auto& addedBuilding = Player->addNewBuilding (position, UnitID, ID);

	addedBuilding.data.setVersion (version);
	addUnit (position, addedBuilding);

	if (addedBuilding.data.connectsToBase)
	{
		Player->base.SubBases[0]->buildings.push_back (&addedBuilding);
		addedBuilding.SubBase = Player->base.SubBases[0];

		addedBuilding.updateNeighbours (*getMap ());
	}
}

void cClient::HandleNetMessage_GAME_EV_WAIT_FOR (cNetMessage& message)
{
	assert (message.iType == GAME_EV_WAIT_FOR);

	const int nextPlayerNum = message.popInt16();

	if (nextPlayerNum != ActivePlayer->getNr())
	{
		enableFreezeMode (FREEZE_WAIT_FOR_OTHERS, nextPlayerNum);
	}
}

void cClient::HandleNetMessage_GAME_EV_MAKE_TURNEND (cNetMessage& message)
{
	assert (message.iType == GAME_EV_MAKE_TURNEND);

	turnClock->increaseTurn ();
	bWantToEnd = false;
	ActivePlayer->clearDone();
	Log.write ("######### Round " + iToStr (turnClock->getTurn()) + " ###########", cLog::eLOG_TYPE_NET_DEBUG);
	for (unsigned int i = 0; i < getPlayerList().size(); i++)
	{
		getPlayerList() [i]->setHasFinishedTurn(false);
	}
	finishedTurnEndProcess ();
}

void cClient::HandleNetMessage_GAME_EV_FINISHED_TURN (cNetMessage& message)
{
	assert (message.iType == GAME_EV_FINISHED_TURN);

	const int playerNumber = message.popInt16 ();
	cPlayer* player = getPlayerFromNumber (playerNumber);

	const bool hasNextPlayerInfo = message.popBool ();
	int nextPlayerNumber = -1;
	if (hasNextPlayerInfo)
	{
		nextPlayerNumber = message.popInt16 ();
	}

	if (player == nullptr)
	{
		Log.write (" Client: Player with nr " + iToStr (playerNumber) + " has finished turn, but can't find him", cLog::eLOG_TYPE_NET_WARNING);
		return;
	}

	if (playerNumber != ActivePlayer->getNr())
	{
		ActivePlayer->addSavedReport (std::make_unique<cSavedReportPlayerEndedTurn> (*player));
	}

	player->setHasFinishedTurn (true);

	playerFinishedTurn (playerNumber, nextPlayerNumber);
}

void cClient::HandleNetMessage_GAME_EV_TURN_START_TIME (cNetMessage& message)
{
	assert (message.iType == GAME_EV_TURN_START_TIME);

	const auto time = message.popInt32 ();

	turnTimeClock->restartFrom (time);
	turnTimeClock->clearAllDeadlines ();

	if (gameSettings->isTurnLimitActive ())
	{
		turnLimitDeadline = turnTimeClock->startNewDeadlineFromNow (gameSettings->getTurnLimit ());
	}
}

void cClient::HandleNetMessage_GAME_EV_TURN_END_DEADLINE_START_TIME (cNetMessage& message)
{
	assert (message.iType == GAME_EV_TURN_END_DEADLINE_START_TIME);

	const auto time = message.popInt32 ();

	turnEndDeadline = turnTimeClock->startNewDeadlineFrom (time, gameSettings->getTurnEndDeadline ());
}

void cClient::HandleNetMessage_GAME_EV_UNIT_DATA (cNetMessage& message)
{
	assert (message.iType == GAME_EV_UNIT_DATA);

	cPlayer* Player = getPlayerFromNumber (message.popInt16());
	(void) Player;  // TODO use me
	const int iID = message.popInt16();
	const bool bVehicle = message.popBool ();
	const auto position = message.popPosition ();

	Log.write (" Client: Received Unit Data: Vehicle: " + iToStr ((int) bVehicle) + ", ID: " + iToStr (iID) + ", XPos: " + iToStr (position.x()) + ", YPos: " + iToStr (position.y()), cLog::eLOG_TYPE_NET_DEBUG);
	cVehicle* Vehicle = NULL;
	sUnitData* Data = NULL;
	// unit is a vehicle
	if (bVehicle)
	{
		bool bBig = message.popBool();
		Vehicle = getVehicleFromID (iID);

		if (!Vehicle)
		{
			Log.write (" Client: Unknown vehicle with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
			// TODO: Request sync of vehicle
			return;
		}
		if (Vehicle->getPosition() != position || Vehicle->data.isBig != bBig)
		{
			// if the vehicle is moving it is normal that
			// the positions may not be the same,
			// when the vehicle was building it is also normal that
			// the position should be changed
			// so the log message will just be a debug one
			int iLogType = cLog::eLOG_TYPE_NET_WARNING;
			if (Vehicle->isUnitBuildingABuilding () || Vehicle->isUnitClearing () || Vehicle->isUnitMoving ()) iLogType = cLog::eLOG_TYPE_NET_DEBUG;
			Log.write (" Client: Vehicle identificated by ID (" + iToStr (iID) + ") but has wrong position [IS: X" + iToStr (Vehicle->getPosition().x()) + " Y" + iToStr (Vehicle->getPosition().y()) + "; SHOULD: X" + iToStr (position.x()) + " Y" + iToStr (position.y()) + "]", iLogType);

			// set to server position if vehicle is not moving
			if (!Vehicle->MoveJobActive)
			{
				getMap()->moveVehicle (*Vehicle, position);
				if (bBig) getMap()->moveVehicleBig (*Vehicle, position);
				Vehicle->getOwner ()->doScan ();
			}
		}

		if (message.popBool()) Vehicle->changeName (message.popString());

		Vehicle->setIsBeeinAttacked(message.popBool());
		const bool bWasDisabled = Vehicle->isDisabled();
		Vehicle->setDisabledTurns(message.popInt16 ());
		Vehicle->setCommandoRank(message.popInt16());
		Vehicle->setClearing(message.popBool());
		Vehicle->setBuildingABuilding(message.popBool());
		Vehicle->setBuildTurns(message.popInt16());
		Vehicle->setClearingTurns(message.popInt16());
		Vehicle->setSentryActive(message.popBool());
		Vehicle->setManualFireActive(message.popBool());

		if (Vehicle->isDisabled () != bWasDisabled && Vehicle->getOwner () == ActivePlayer)
		{
			if (Vehicle->isDisabled())
			{
				ActivePlayer->addSavedReport (std::make_unique<cSavedReportDisabled>(*Vehicle));
				unitDisabled (*Vehicle);
			}
			Vehicle->getOwner ()->doScan ();
		}
		Data = &Vehicle->data;
	}
	else
	{
		cBuilding* Building = getBuildingFromID (iID);
		if (!Building)
		{
			Log.write (" Client: Unknown building with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
			// TODO: Request sync of building
			return;
		}

		if (message.popBool()) Building->changeName (message.popString());

		const bool bWasDisabled = Building->isDisabled();
		Building->setDisabledTurns (message.popInt16 ());
		Building->setResearchArea ((cResearch::ResearchArea)message.popInt16());
		Building->setWorking(message.popBool());
		Building->setSentryActive( message.popBool ());
		Building->setManualFireActive( message.popBool ());
		Building->points = message.popInt16();

		if (Building->isDisabled () != bWasDisabled && Building->getOwner () == ActivePlayer)
		{
			if (Building->isDisabled())
			{
				ActivePlayer->addSavedReport (std::make_unique<cSavedReportDisabled> (*Building));
				unitDisabled (*Building);
			}
			Building->getOwner ()->doScan ();
		}
		Data = &Building->data;
	}

	Data->buildCosts = message.popInt16();
	Data->setAmmo(message.popInt16());
	Data->ammoMax = message.popInt16();
	Data->setStoredResources(message.popInt16());
	Data->storageResMax = message.popInt16();
	Data->setStoredUnits(message.popInt16());
	Data->storageUnitsMax = message.popInt16();
	Data->setDamage(message.popInt16());
	Data->setShots(message.popInt16());
	Data->shotsMax = message.popInt16();
	Data->setRange(message.popInt16());
	Data->setScan(message.popInt16());
	Data->setArmor(message.popInt16());
	Data->setHitpoints(message.popInt16());
	Data->hitpointsMax = message.popInt16();
	Data->setVersion(message.popInt16());

	if (bVehicle)
	{
		if (Data->canPlaceMines)
		{
			if (Data->getStoredResources () <= 0) Vehicle->setLayMines (false);
			if (Data->getStoredResources () >= Data->storageResMax) Vehicle->setClearMines (false);
		}
		Data->speedCur = message.popInt16();
		Data->speedMax = message.popInt16();

		Vehicle->setFlightHeight(message.popInt16 ());
	}
}

void cClient::HandleNetMessage_GAME_EV_SPECIFIC_UNIT_DATA (cNetMessage& message)
{
	assert (message.iType == GAME_EV_SPECIFIC_UNIT_DATA);

	cVehicle* Vehicle = getVehicleFromID (message.popInt16());
	if (!Vehicle) return;
	Vehicle->dir = message.popInt16();
	Vehicle->setBuildingType (message.popID ());
	Vehicle->BuildPath = message.popBool();
	Vehicle->bandPosition = message.popPosition();
}

void cClient::HandleNetMessage_GAME_EV_DO_START_WORK (cNetMessage& message)
{
	assert (message.iType == GAME_EV_DO_START_WORK);

	const int iID = message.popInt32();

	cBuilding* building = getBuildingFromID (iID);
	if (building == NULL)
	{
		Log.write (" Client: Can't start work of building: Unknown building with id: " + iToStr (iID), cLog::eLOG_TYPE_NET_ERROR);
		// TODO: Request sync of building
		return;
	}
	building->clientStartWork ();
}

void cClient::HandleNetMessage_GAME_EV_DO_STOP_WORK (cNetMessage& message)
{
	assert (message.iType == GAME_EV_DO_STOP_WORK);

	const int iID = message.popInt32();
	cBuilding* building = getBuildingFromID (iID);
	if (building == NULL)
	{
		Log.write (" Client: Can't stop work of building: Unknown building with id: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
		// TODO: Request sync of building
		return;
	}
	building->clientStopWork ();
}

void cClient::HandleNetMessage_GAME_EV_MOVE_JOB_SERVER (cNetMessage& message)
{
	assert (message.iType == GAME_EV_MOVE_JOB_SERVER);

	const int iVehicleID = message.popInt32();
	const auto srcPosition = message.popPosition ();
	const auto destPosition = message.popPosition ();
	const int iSavedSpeed = message.popInt16();
	cVehicle* Vehicle = getVehicleFromID (iVehicleID);

	if (Vehicle == NULL)
	{
		Log.write (" Client: Can't find vehicle with id " + iToStr (iVehicleID) + " for movejob from " + iToStr (srcPosition.x()) + "x" + iToStr (srcPosition.y()) + " to " + iToStr (destPosition.x()) + "x" + iToStr (destPosition.y()), cLog::eLOG_TYPE_NET_WARNING);
		// TODO: request sync of vehicle
		return;
	}

	cClientMoveJob* MoveJob = new cClientMoveJob(*this, srcPosition, destPosition, Vehicle);
	MoveJob->iSavedSpeed = iSavedSpeed;
	if (!MoveJob->generateFromMessage (message)) return;
	Log.write (" Client: Added received movejob at time " + iToStr (gameTimer->gameTime), cLog::eLOG_TYPE_NET_DEBUG);
}

void cClient::HandleNetMessage_GAME_EV_NEXT_MOVE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_NEXT_MOVE);

	const int iID = message.popInt16();
	const int iType = message.popChar();
	int iSavedSpeed = -1;
	if (iType == MJOB_STOP) iSavedSpeed = message.popChar();

	Log.write (" Client: Received information for next move: ID: " + iToStr (iID) + ", Type: " + iToStr (iType) + ", Time: " + iToStr (gameTimer->gameTime), cLog::eLOG_TYPE_NET_DEBUG);

	cVehicle* Vehicle = getVehicleFromID (iID);
	if (Vehicle && Vehicle->getClientMoveJob ())
	{
		Vehicle->getClientMoveJob ()->handleNextMove (iType, iSavedSpeed);
	}
	else
	{
		if (Vehicle == NULL) Log.write (" Client: Can't find vehicle with ID " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
		else Log.write (" Client: Vehicle with ID " + iToStr (iID) + "has no movejob", cLog::eLOG_TYPE_NET_WARNING);
		// TODO: request sync of vehicle
	}
}

void cClient::HandleNetMessage_GAME_EV_ATTACKJOB_LOCK_TARGET (cNetMessage& message)
{
	assert (message.iType == GAME_EV_ATTACKJOB_LOCK_TARGET);

	cClientAttackJob::lockTarget (*this, message);
}

void cClient::HandleNetMessage_GAME_EV_ATTACKJOB_FIRE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_ATTACKJOB_FIRE);

	cClientAttackJob* job = new cClientAttackJob (this, message);
	attackJobs.push_back (job);
}

void cClient::HandleNetMessage_GAME_EV_ATTACKJOB_IMPACT (cNetMessage& message)
{
	assert (message.iType == GAME_EV_ATTACKJOB_IMPACT);

	const int id = message.popInt16();
	const int remainingHP = message.popInt16();
	const auto position = message.popPosition();
	cClientAttackJob::makeImpact (*this, position, remainingHP, id);
}

void cClient::HandleNetMessage_GAME_EV_RESOURCES (cNetMessage& message)
{
	assert (message.iType == GAME_EV_RESOURCES);

	const int iCount = message.popInt16();
	for (int i = 0; i < iCount; i++)
	{
		const auto position = message.popPosition();

		ActivePlayer->exploreResource (position);

		sResources& res = getMap ()->getResource (position);
		res.typ = (unsigned char) message.popInt16();
		res.value = (unsigned char) message.popInt16();
	}
}

void cClient::HandleNetMessage_GAME_EV_BUILD_ANSWER (cNetMessage& message)
{
	assert (message.iType == GAME_EV_BUILD_ANSWER);

	const bool bOK = message.popBool();
	const int iID = message.popInt16();
	cVehicle* Vehicle = getVehicleFromID (iID);
	if (Vehicle == NULL)
	{
		Log.write (" Client: Vehicle can't start building: Unknown vehicle with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
		// TODO: Request sync of vehicle
		return;
	}

	if (!bOK)
	{
		if (Vehicle->getOwner () == ActivePlayer)
		{
			if (!Vehicle->BuildPath)
			{
				ActivePlayer->addSavedReport (std::make_unique<cSavedReportSimple> (eSavedReportType::ProducingError));
			}
			else if (Vehicle->bandPosition != Vehicle->getPosition())
			{
				ActivePlayer->addSavedReport (std::make_unique<cSavedReportPathInterrupted> (*Vehicle));
			}
		}
		Vehicle->setBuildTurns(0);
		Vehicle->BuildPath = false;
		Vehicle->bandPosition = cPosition(0,0);
		return;
	}

	if (Vehicle->isUnitBuildingABuilding ()) Log.write (" Client: Vehicle is already building", cLog::eLOG_TYPE_NET_ERROR);

	const auto buildPosition = message.popPosition ();
	const bool buildBig = message.popBool();
	const auto oldPosition = Vehicle->getPosition();

	if (buildBig)
	{
		getMap()->moveVehicleBig(*Vehicle, buildPosition);
		Vehicle->getOwner ()->doScan ();
	}
	else
	{
		getMap()->moveVehicle(*Vehicle, buildPosition);
		Vehicle->getOwner ()->doScan ();
	}

	if (Vehicle->getOwner () == ActivePlayer)
	{
		Vehicle->setBuildingType (message.popID());
		Vehicle->setBuildTurns (message.popInt16 ());
		Vehicle->BuildPath = message.popBool();
		Vehicle->bandPosition = message.popPosition();
	}

	Vehicle->setBuildingABuilding(true);
	addJob (new cStartBuildJob (*Vehicle, oldPosition, buildBig));

	if (Vehicle->getClientMoveJob ()) Vehicle->getClientMoveJob ()->release ();
}


void cClient::HandleNetMessage_GAME_EV_STOP_BUILD (cNetMessage& message)
{
	assert (message.iType == GAME_EV_STOP_BUILD);

	const int iID = message.popInt16();
	cVehicle* Vehicle = getVehicleFromID (iID);
	if (Vehicle == NULL)
	{
		Log.write (" Client: Can't stop building: Unknown vehicle with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
		// TODO: Request sync of vehicle
		return;
	}

	const auto newPosition = message.popPosition();

	if (Vehicle->data.isBig)
	{
		getMap()->moveVehicle (*Vehicle, newPosition);
		Vehicle->getOwner ()->doScan ();
	}

	Vehicle->setBuildingABuilding(false);
	Vehicle->BuildPath = false;
}

void cClient::HandleNetMessage_GAME_EV_SUBBASE_VALUES (cNetMessage& message)
{
	assert (message.iType == GAME_EV_SUBBASE_VALUES);

	const int iID = message.popInt16();
	sSubBase* subBase = getSubBaseFromID (iID);
	if (subBase == NULL)
	{
		Log.write (" Client: Can't add subbase values: Unknown subbase with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
		// TODO: Request sync of subbases
		return;
	}

	subBase->popFrom (message);

	// temporary debug check
	if (subBase->isDitributionMaximized() == false)
	{
		Log.write (" Server: Mine distribution values are not a maximum", cLog::eLOG_TYPE_NET_WARNING);
	}
}

void cClient::HandleNetMessage_GAME_EV_BUILDLIST (cNetMessage& message)
{
	assert (message.iType == GAME_EV_BUILDLIST);

	const int iID = message.popInt16();
	cBuilding* Building = getBuildingFromID (iID);
	if (Building == NULL)
	{
		Log.write (" Client: Can't set buildlist: Unknown building with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
		// TODO: Request sync of building
		return;
	}

	std::vector<cBuildListItem> newBuildList;
	const int iCount = message.popInt16();
	for (int i = 0; i < iCount; i++)
	{
		cBuildListItem buildListItem;
		buildListItem.setType (message.popID ());
		buildListItem.setRemainingMetal (message.popInt16 ());
		newBuildList.push_back (std::move (buildListItem));
	}
	Building->setBuildList (std::move (newBuildList));

	Building->MetalPerRound = message.popInt16();
	Building->BuildSpeed = message.popInt16();
	Building->RepeatBuild = message.popBool();
}

void cClient::HandleNetMessage_GAME_EV_MINE_PRODUCE_VALUES (cNetMessage& message)
{
	assert (message.iType == GAME_EV_MINE_PRODUCE_VALUES);

	const int iID = message.popInt16();
	cBuilding* Building = getBuildingFromID (iID);
	if (Building == NULL)
	{
		Log.write (" Client: Can't set produce values of building: Unknown building with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
		// TODO: Request sync of building
		return;
	}

	Building->MaxMetalProd = message.popInt16();
	Building->MaxOilProd = message.popInt16();
	Building->MaxGoldProd = message.popInt16();
}

void cClient::HandleNetMessage_GAME_EV_MARK_LOG (cNetMessage& message)
{
	assert (message.iType == GAME_EV_MARK_LOG);

	Log.write ("=============================================================================================", cLog::eLOG_TYPE_NET_DEBUG);
	Log.write (message.popString(), cLog::eLOG_TYPE_NET_DEBUG);
	Log.write ("=============================================================================================", cLog::eLOG_TYPE_NET_DEBUG);
}

void cClient::HandleNetMessage_GAME_EV_SUPPLY (cNetMessage& message)
{
	assert (message.iType == GAME_EV_SUPPLY);

	const int iType = message.popChar();
	cUnit* DestUnit = nullptr;
	if (message.popBool())
	{
		const int iID = message.popInt16();
		cVehicle* DestVehicle = getVehicleFromID (iID);
		DestUnit = DestVehicle;
		if (!DestVehicle)
		{
			Log.write (" Client: Can't supply vehicle: Unknown vehicle with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
			// TODO: Request sync of vehicle
			return;
		}
		if (iType == SUPPLY_TYPE_REARM) DestVehicle->data.setAmmo(message.popInt16());
		else DestVehicle->data.setHitpoints(message.popInt16());

		//if (DestVehicle->isUnitLoaded ())
		//{
		//	// get the building which has loaded the unit
		//	cBuilding* Building = DestVehicle->owner->BuildingList;
		//	for (; Building; Building = Building->next)
		//	{
		//		if (Contains (Building->storedUnits, DestVehicle)) break;
		//	}
		//}
	}
	else
	{
		const int iID = message.popInt16();
		cBuilding* DestBuilding = getBuildingFromID (iID);
		DestUnit = DestBuilding;
		if (!DestBuilding)
		{
			Log.write (" Client: Can't supply building: Unknown building with ID: " + iToStr (iID), cLog::eLOG_TYPE_NET_WARNING);
			// TODO: Request sync of building
			return;
		}
		if (iType == SUPPLY_TYPE_REARM) DestBuilding->data.setAmmo(message.popInt16());
		else DestBuilding->data.setHitpoints(message.popInt16());
	}
	assert (DestUnit != nullptr);
	if (iType == SUPPLY_TYPE_REARM)
	{
		unitSuppliedWithAmmo (*DestUnit);
	}
	else
	{
		unitRepaired (*DestUnit);
	}
}

void cClient::HandleNetMessage_GAME_EV_ADD_RUBBLE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_ADD_RUBBLE);

	const bool big = message.popBool();
	const int typ = message.popInt16();
	const int value = message.popInt16();
	const unsigned int ID = message.popInt16();
	auto rubble = std::make_shared<cBuilding> (nullptr, nullptr, ID);

	rubble->data.isBig = big;
	rubble->RubbleTyp = typ;
	rubble->RubbleValue = value;
	const auto position = message.popPosition ();

	rubble->setPosition (position);

	getMap()->addBuilding (*rubble, rubble->getPosition());

	auto result = neutralBuildings.insert (std::move (rubble));
	assert (result.second);
}

void cClient::HandleNetMessage_GAME_EV_DETECTION_STATE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_DETECTION_STATE);

	const int id = message.popInt32();
	cVehicle* vehicle = getVehicleFromID (id);
	if (vehicle == NULL)
	{
		Log.write (" Client: Vehicle (ID: " + iToStr (id) + ") not found", cLog::eLOG_TYPE_NET_ERROR);
		return;
	}
	const bool detected = message.popBool();
	if (detected)
	{
		//mark vehicle as detected with size of detectedByPlayerList > 0
		vehicle->detectedByPlayerList.push_back (NULL);
	}
	else
	{
		vehicle->detectedByPlayerList.clear();
	}
}

void cClient::HandleNetMessage_GAME_EV_CLEAR_ANSWER (cNetMessage& message)
{
	assert (message.iType == GAME_EV_CLEAR_ANSWER);

	switch (message.popInt16())
	{
		case 0:
		{
			const int id = message.popInt16();
			cVehicle* Vehicle = getVehicleFromID (id);
			if (Vehicle == NULL)
			{
				Log.write ("Client: Can not find vehicle with id " + iToStr (id) + " for clearing", LOG_TYPE_NET_WARNING);
				break;
			}
			const auto orgiginalPosition = Vehicle->getPosition();

			Vehicle->setClearingTurns(message.popInt16());
			const auto bigPosition = message.popPosition ();
			if (bigPosition.x () >= 0 && bigPosition.y() >= 0)
			{
				getMap()->moveVehicleBig (*Vehicle, bigPosition);
				Vehicle->getOwner ()->doScan ();
			}
			Vehicle->setClearing(true);
			addJob (new cStartBuildJob (*Vehicle, orgiginalPosition, (bigPosition.x () >= 0 && bigPosition.y () >= 0)));
		}
		break;
		case 1:
			// TODO: add blocked message
			// gameGUI->addMessage ("blocked");
			break;
		case 2:
			Log.write ("Client: warning on start of clearing", LOG_TYPE_NET_WARNING);
			break;
		default:
			break;
	}
}

void cClient::HandleNetMessage_GAME_EV_STOP_CLEARING (cNetMessage& message)
{
	assert (message.iType == GAME_EV_STOP_CLEARING);

	const int id = message.popInt16();
	cVehicle* Vehicle = getVehicleFromID (id);
	if (Vehicle == NULL)
	{
		Log.write ("Client: Can not find vehicle with id " + iToStr (id) + " for stop clearing", LOG_TYPE_NET_WARNING);
		return;
	}

	const auto bigPosition = message.popPosition ();
	if (bigPosition.x () >= 0 && bigPosition.y () >= 0)
	{
		getMap()->moveVehicle(*Vehicle, bigPosition);
		Vehicle->getOwner ()->doScan ();
	}
	Vehicle->setClearing(false);
	Vehicle->setClearingTurns(0);
}

void cClient::HandleNetMessage_GAME_EV_NOFOG (cNetMessage& message)
{
	assert (message.iType == GAME_EV_NOFOG);

	ActivePlayer->revealMap();
}

void cClient::HandleNetMessage_GAME_EV_DEFEATED (cNetMessage& message)
{
	assert (message.iType == GAME_EV_DEFEATED);

	const int iTmp = message.popInt16();
	cPlayer* Player = getPlayerFromNumber (iTmp);
	if (Player == NULL)
	{
		Log.write ("Client: Cannot find defeated player!", LOG_TYPE_NET_WARNING);
		return;
	}
	Player->isDefeated = true;
	ActivePlayer->addSavedReport (std::make_unique<cSavedReportPlayerDefeated> (*Player));
#if 0
	for (unsigned int i = 0; i < getPlayerList()->size(); i++)
	{
		if (Player == (*getPlayerList()) [i])
		{
			Hud.ExtraPlayers (Player->getName() + " (d)", Player->getColor(), i, Player->bFinishedTurn, false);
			return;
		}
	}
#endif
}

void cClient::HandleNetMessage_GAME_EV_FREEZE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_FREEZE);

	const eFreezeMode mode = (eFreezeMode) message.popInt16();
	const int playerNumber = message.popInt16();
	enableFreezeMode (mode, playerNumber);
}

void cClient::HandleNetMessage_GAME_EV_UNFREEZE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_UNFREEZE);

	eFreezeMode mode = (eFreezeMode) message.popInt16();
	disableFreezeMode (mode);
}

void cClient::HandleNetMessage_GAME_EV_DEL_PLAYER (cNetMessage& message)
{
	assert (message.iType == GAME_EV_DEL_PLAYER);

	cPlayer* Player = getPlayerFromNumber (message.popInt16());
	if (Player == ActivePlayer)
	{
		Log.write ("Client: Cannot delete own player!", LOG_TYPE_NET_WARNING);
		return;
	}
	if (Player->hasUnits())
	{
		Log.write ("Client: Player to be deleted has some units left !", LOG_TYPE_NET_ERROR);
	}
	ActivePlayer->addSavedReport (std::make_unique<cSavedReportPlayerLeft> (*Player));

	deletePlayer (*Player);
}

void cClient::HandleNetMessage_GAME_EV_TURN (cNetMessage& message)
{
	assert (message.iType == GAME_EV_TURN);

	turnClock->setTurn (message.popInt16 ());
}

void cClient::HandleNetMessage_GAME_EV_HUD_SETTINGS (cNetMessage& message)
{
	cGameGuiState state;
	state.popFrom (message);

	gameGuiStateReceived (state);
}

void cClient::HandleNetMessage_GAME_EV_STORE_UNIT (cNetMessage& message)
{
	assert (message.iType == GAME_EV_STORE_UNIT);

	cVehicle* storedVehicle = getVehicleFromID (message.popInt16());
	if (!storedVehicle) return;

	if (message.popBool())
	{
		cVehicle* storingVehicle = getVehicleFromID (message.popInt16());
		if (!storingVehicle) return;
		storingVehicle->storeVehicle (*storedVehicle, *getMap());
		unitStored (*storingVehicle, *storedVehicle);
	}
	else
	{
		cBuilding* storingBuilding = getBuildingFromID (message.popInt16());
		if (!storingBuilding) return;
		storingBuilding->storeVehicle (*storedVehicle, *getMap ());
		unitStored (*storingBuilding, *storedVehicle);
	}
}

void cClient::HandleNetMessage_GAME_EV_EXIT_UNIT (cNetMessage& message)
{
	assert (message.iType == GAME_EV_EXIT_UNIT);

	cVehicle* StoredVehicle = getVehicleFromID (message.popInt16());
	if (!StoredVehicle) return;

	if (message.popBool())
	{
		cVehicle* StoringVehicle = getVehicleFromID (message.popInt16());
		if (!StoringVehicle) return;

		const auto position = message.popPosition ();
		StoringVehicle->exitVehicleTo (*StoredVehicle, position, *getMap ());
		unitActivated (*StoringVehicle, *StoredVehicle);
	}
	else
	{
		cBuilding* StoringBuilding = getBuildingFromID (message.popInt16());
		if (!StoringBuilding) return;

		const auto position = message.popPosition ();
		StoringBuilding->exitVehicleTo (*StoredVehicle, position, *getMap ());
		unitActivated (*StoringBuilding, *StoredVehicle);
	}
}

void cClient::HandleNetMessage_GAME_EV_DELETE_EVERYTHING (cNetMessage& message)
{
	assert (message.iType == GAME_EV_DELETE_EVERYTHING);

	for (unsigned int i = 0; i < getPlayerList().size(); i++)
	{
		cPlayer& player = *getPlayerList() [i];

		player.removeAllUnits ();
	}

	//delete subbases
	// TODO: check that each subbase is deleted
	ActivePlayer->base.SubBases.clear();

	for (auto i = neutralBuildings.begin (); i != neutralBuildings.end (); ++i)
	{
		getMap()->deleteBuilding (**i);
	}
	neutralBuildings.clear ();

	//delete attack jobs
	for (size_t i = 0; i != attackJobs.size(); ++i)
	{
		delete attackJobs[i];
	}
	attackJobs.clear();

	//TODO: delete fx effects???

	// delete all eventually remaining pointers on the map, to prevent crashes after a resync.
	// Normally there shouldn't be any pointers left after deleting all units, but a resync is not
	// executed in normal situations and there are situations, when this happens.
	getMap()->reset();
}

void cClient::HandleNetMessage_GAME_EV_UNIT_UPGRADE_VALUES (cNetMessage& message)
{
	assert (message.iType == GAME_EV_UNIT_UPGRADE_VALUES);

	const sID ID = message.popID();
	sUnitData* Data = ActivePlayer->getUnitDataCurrentVersion (ID);
	if (Data == NULL) return;

	Data->setVersion(message.popInt16());
	Data->setScan(message.popInt16());
	Data->setRange(message.popInt16());
	Data->setDamage(message.popInt16());
	Data->buildCosts = message.popInt16();
	Data->setArmor(message.popInt16());
	Data->speedMax = message.popInt16();
	Data->shotsMax = message.popInt16();
	Data->ammoMax = message.popInt16();
	Data->hitpointsMax = message.popInt16();
}

void cClient::HandleNetMessage_GAME_EV_CREDITS_CHANGED (cNetMessage& message)
{
	assert (message.iType == GAME_EV_CREDITS_CHANGED);

	ActivePlayer->setCredits(message.popInt16());
}

void cClient::HandleNetMessage_GAME_EV_UPGRADED_BUILDINGS (cNetMessage& message)
{
	assert (message.iType == GAME_EV_UPGRADED_BUILDINGS);

	const int buildingsInMsg = message.popInt16();
	const int totalCosts = message.popInt16();
	if (buildingsInMsg <= 0) return;

	const sUnitData* unitData = nullptr;
	bool scanNecessary = false;
	for (int i = 0; i < buildingsInMsg; i++)
	{
		const int buildingID = message.popInt32();
		cBuilding* building = getBuildingFromID (buildingID);
		if (!building)
		{
			Log.write (" Client: Unknown building with ID: " + iToStr (buildingID), cLog::eLOG_TYPE_NET_ERROR);
			break;
		}
		const sUnitData& upgraded = *ActivePlayer->getUnitDataCurrentVersion (building->data.ID);
		if (building->data.getScan () < upgraded.getScan ())
			scanNecessary = true; // Scan range was upgraded. So trigger a scan.
		building->upgradeToCurrentVersion();
		if (i == 0)
		{
			unitData = &building->data;
		}
	}
	assert (unitData != nullptr);
	ActivePlayer->addSavedReport (std::make_unique<cSavedReportUpgraded> (unitData->ID, buildingsInMsg, totalCosts));
	if (scanNecessary)
		ActivePlayer->doScan();
}

void cClient::HandleNetMessage_GAME_EV_UPGRADED_VEHICLES (cNetMessage& message)
{
	assert (message.iType == GAME_EV_UPGRADED_VEHICLES);

	const int vehiclesInMsg = message.popInt16();
	const int totalCosts = message.popInt16();
	/*const unsigned int storingBuildingID =*/ message.popInt32();
	if (vehiclesInMsg <= 0) return;

	const sUnitData* unitData = nullptr;
	for (int i = 0; i < vehiclesInMsg; i++)
	{
		const int vehicleID = message.popInt32();
		cVehicle* vehicle = getVehicleFromID (vehicleID);
		if (!vehicle)
		{
			Log.write (" Client: Unknown vehicle with ID: " + iToStr (vehicleID), cLog::eLOG_TYPE_NET_ERROR);
			break;
		}
		vehicle->upgradeToCurrentVersion();
		if (i == 0)
		{
			unitData = &vehicle->data;
		}
	}
	assert (unitData != nullptr);
	ActivePlayer->addSavedReport (std::make_unique<cSavedReportUpgraded> (unitData->ID, vehiclesInMsg, totalCosts));
}

void cClient::HandleNetMessage_GAME_EV_RESEARCH_SETTINGS (cNetMessage& message)
{
	assert (message.iType == GAME_EV_RESEARCH_SETTINGS);

	const int buildingsInMsg = message.popInt16();
	for (int i = 0; i < buildingsInMsg; ++i)
	{
		const int buildingID = message.popInt32();
		const cResearch::ResearchArea newArea = (cResearch::ResearchArea)message.popChar ();
		cBuilding* building = getBuildingFromID (buildingID);
		if (building && building->data.canResearch && 0 <= newArea && newArea <= cResearch::kNrResearchAreas)
			building->setResearchArea(newArea);
	}
	// now update the research center count for the areas
	ActivePlayer->refreshResearchCentersWorkingOnArea();
}

void cClient::HandleNetMessage_GAME_EV_RESEARCH_LEVEL (cNetMessage& message)
{
	assert (message.iType == GAME_EV_RESEARCH_LEVEL);

	for (int area = cResearch::kNrResearchAreas - 1; area >= 0; area--)
	{
		const int newCurPoints = message.popInt16 ();
		const int newLevel = message.popInt16 ();
		ActivePlayer->getResearchState ().setCurResearchLevel (newLevel, area);
		ActivePlayer->getResearchState ().setCurResearchPoints (newCurPoints, area);
	}
}

void cClient::HandleNetMessage_GAME_EV_FINISHED_RESEARCH_AREAS (cNetMessage& message)
{
	assert (message.iType == GAME_EV_FINISHED_RESEARCH_AREAS);

	std::vector<int> areas (message.popInt32());
	for (size_t i = 0; i < areas.size (); ++i)
	{
		areas[i] = message.popInt32 ();
	}
	ActivePlayer->setCurrentTurnResearchAreasFinished (std::move (areas));
}

void cClient::HandleNetMessage_GAME_EV_REFRESH_RESEARCH_COUNT (cNetMessage& message)
{
	assert (message.iType == GAME_EV_REFRESH_RESEARCH_COUNT);

	ActivePlayer->refreshResearchCentersWorkingOnArea();
}

void cClient::HandleNetMessage_GAME_EV_SET_AUTOMOVE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_SET_AUTOMOVE);

	cVehicle* Vehicle = getVehicleFromID (message.popInt16());
	if (Vehicle)
	{
		Vehicle->startAutoMoveJob (*this);
	}
}

void cClient::HandleNetMessage_GAME_EV_COMMANDO_ANSWER (cNetMessage& message)
{
	assert (message.iType == GAME_EV_COMMANDO_ANSWER);

	bool success = message.popBool ();
	bool steal = false;
	if (success) steal = message.popBool ();
	cVehicle* vehicle = getVehicleFromID (message.popInt16 ());

	if (vehicle)
	{
		if (success)
		{
			if (steal) unitHasStolenSuccessfully (*vehicle);
			else unitHasDisabledSuccessfully (*vehicle);
		}
		else
		{
			unitStealDisableFailed (*vehicle);
		}
	}
}

void cClient::HandleNetMessage_GAME_EV_REQ_SAVE_INFO (cNetMessage& message)
{
	assert (message.iType == GAME_EV_REQ_SAVE_INFO);

	const int saveingID = message.popInt16();

	additionalSaveInfoRequested (saveingID);

	const auto& savedReports = ActivePlayer->savedReportsList;
	for (size_t i = std::max<int> (0, savedReports.size() - 50); i != savedReports.size(); ++i)
	{
		sendSaveReportInfo (*this, *savedReports[i], ActivePlayer->getNr(), saveingID);
	}
	sendFinishedSendSaveInfo (*this, ActivePlayer->getNr(), saveingID);
}

void cClient::HandleNetMessage_GAME_EV_SAVED_REPORT (cNetMessage& message)
{
	assert (message.iType == GAME_EV_SAVED_REPORT);

	ActivePlayer->addSavedReport (cSavedReport::createFrom(message));
}

void cClient::HandleNetMessage_GAME_EV_CASUALTIES_REPORT (cNetMessage& message)
{
	assert (message.iType == GAME_EV_CASUALTIES_REPORT);

	casualtiesTracker->updateCasualtiesFromNetMessage (&message);
}

void cClient::HandleNetMessage_GAME_EV_SCORE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_SCORE);

	const int pn = message.popInt16();
	const int turn = message.popInt16();
	const int n = message.popInt16();

	getPlayerFromNumber (pn)->setScore (n, turn);
}

void cClient::HandleNetMessage_GAME_EV_NUM_ECOS (cNetMessage& message)
{
	assert (message.iType == GAME_EV_NUM_ECOS);

	const int pn = message.popInt16();
	const int n = message.popInt16();

	getPlayerFromNumber (pn)->numEcos = n;
}

void cClient::HandleNetMessage_GAME_EV_UNIT_SCORE (cNetMessage& message)
{
	assert (message.iType == GAME_EV_UNIT_SCORE);

	cBuilding* b = getBuildingFromID (message.popInt16());
	b->points = message.popInt16();
}

void cClient::HandleNetMessage_GAME_EV_GAME_SETTINGS (cNetMessage& message)
{
	assert (message.iType == GAME_EV_GAME_SETTINGS);

	gameSettings->popFrom (message);
}

void cClient::HandleNetMessage_GAME_EV_SELFDESTROY (cNetMessage& message)
{
	assert (message.iType == GAME_EV_SELFDESTROY);

	cBuilding* building = getBuildingFromID (message.popInt16());
	if (!building) return;

	destroyUnit (*building);
}

void cClient::HandleNetMessage_GAME_EV_END_MOVE_ACTION_SERVER (cNetMessage& message)
{
	assert (message.iType == GAME_EV_END_MOVE_ACTION_SERVER);

	cVehicle* vehicle = getVehicleFromID (message.popInt32());
	if (!vehicle || !vehicle->getClientMoveJob ()) return;

	const int destID = message.popInt32();
	eEndMoveActionType type = (eEndMoveActionType) message.popChar();
	vehicle->getClientMoveJob ()->endMoveAction = new cEndMoveAction (vehicle, destID, type);
}

void cClient::HandleNetMessage_GAME_EV_SET_GAME_TIME (cNetMessage& message)
{
	assert (message.iType == GAME_EV_SET_GAME_TIME);

	const unsigned int newGameTime = message.popInt32();
	gameTimer->gameTime = newGameTime;

	//confirm new time to the server
	cNetMessage* response = new cNetMessage (NET_GAME_TIME_CLIENT);
	response->pushInt32 (gameTimer->gameTime);
	sendNetMessage (response);
}

void cClient::HandleNetMessage_GAME_EV_REVEAL_MAP (cNetMessage& message)
{
	assert (message.iType == GAME_EV_REVEAL_MAP);

	ActivePlayer->revealMap ();
}

void cClient::handleNetMessages ()
{
	if (gameTimer->nextMsgIsNextGameTime) return;

	std::unique_ptr<cNetMessage> message;
	while (eventQueue.try_pop(message))
	{
		handleNetMessage (*message);
		if (gameTimer->nextMsgIsNextGameTime) break;
	}
}

int cClient::handleNetMessage (cNetMessage& message)
{
	if (message.iType != NET_GAME_TIME_SERVER)
	{
		Log.write ("Client: " + getActivePlayer ().getName () + " <-- "
				   + message.getTypeAsString ()
				   + ", gameTime:" + iToStr (this->gameTimer->gameTime)
				   + ", Hexdump: " + message.getHexDump (), cLog::eLOG_TYPE_NET_DEBUG);
	}

	switch (message.iType)
	{
		case TCP_ACCEPT:
			//should not happen
			break;
		case TCP_CLOSE: HandleNetMessage_TCP_CLOSE (message); break;
		case GAME_EV_PLAYER_CLANS: HandleNetMessage_GAME_EV_PLAYER_CLANS (message); break;
		case GAME_EV_ADD_BUILDING: HandleNetMessage_GAME_EV_ADD_BUILDING (message); break;
		case GAME_EV_ADD_VEHICLE: HandleNetMessage_GAME_EV_ADD_VEHICLE (message); break;
		case GAME_EV_DEL_BUILDING: HandleNetMessage_GAME_EV_DEL_BUILDING (message); break;
		case GAME_EV_DEL_VEHICLE: HandleNetMessage_GAME_EV_DEL_VEHICLE (message); break;
		case GAME_EV_ADD_ENEM_VEHICLE: HandleNetMessage_GAME_EV_ADD_ENEM_VEHICLE (message); break;
		case GAME_EV_ADD_ENEM_BUILDING: HandleNetMessage_GAME_EV_ADD_ENEM_BUILDING (message); break;
		case GAME_EV_WAIT_FOR: HandleNetMessage_GAME_EV_WAIT_FOR (message); break;
		case GAME_EV_MAKE_TURNEND: HandleNetMessage_GAME_EV_MAKE_TURNEND (message); break;
		case GAME_EV_FINISHED_TURN: HandleNetMessage_GAME_EV_FINISHED_TURN (message); break;
		case GAME_EV_TURN_START_TIME: HandleNetMessage_GAME_EV_TURN_START_TIME (message); break;
		case GAME_EV_TURN_END_DEADLINE_START_TIME: HandleNetMessage_GAME_EV_TURN_END_DEADLINE_START_TIME (message); break;
		case GAME_EV_UNIT_DATA: HandleNetMessage_GAME_EV_UNIT_DATA (message); break;
		case GAME_EV_SPECIFIC_UNIT_DATA: HandleNetMessage_GAME_EV_SPECIFIC_UNIT_DATA (message); break;
		case GAME_EV_DO_START_WORK: HandleNetMessage_GAME_EV_DO_START_WORK (message); break;
		case GAME_EV_DO_STOP_WORK: HandleNetMessage_GAME_EV_DO_STOP_WORK (message); break;
		case GAME_EV_MOVE_JOB_SERVER: HandleNetMessage_GAME_EV_MOVE_JOB_SERVER (message); break;
		case GAME_EV_NEXT_MOVE: HandleNetMessage_GAME_EV_NEXT_MOVE (message); break;
		case GAME_EV_ATTACKJOB_LOCK_TARGET: HandleNetMessage_GAME_EV_ATTACKJOB_LOCK_TARGET (message); break;
		case GAME_EV_ATTACKJOB_FIRE: HandleNetMessage_GAME_EV_ATTACKJOB_FIRE (message); break;
		case GAME_EV_ATTACKJOB_IMPACT: HandleNetMessage_GAME_EV_ATTACKJOB_IMPACT (message); break;
		case GAME_EV_RESOURCES: HandleNetMessage_GAME_EV_RESOURCES (message); break;
		case GAME_EV_BUILD_ANSWER: HandleNetMessage_GAME_EV_BUILD_ANSWER (message); break;
		case GAME_EV_STOP_BUILD: HandleNetMessage_GAME_EV_STOP_BUILD (message); break;
		case GAME_EV_SUBBASE_VALUES: HandleNetMessage_GAME_EV_SUBBASE_VALUES (message); break;
		case GAME_EV_BUILDLIST: HandleNetMessage_GAME_EV_BUILDLIST (message); break;
		case GAME_EV_MINE_PRODUCE_VALUES: HandleNetMessage_GAME_EV_MINE_PRODUCE_VALUES (message); break;
		case GAME_EV_MARK_LOG: HandleNetMessage_GAME_EV_MARK_LOG (message); break;
		case GAME_EV_SUPPLY: HandleNetMessage_GAME_EV_SUPPLY (message); break;
		case GAME_EV_ADD_RUBBLE: HandleNetMessage_GAME_EV_ADD_RUBBLE (message); break;
		case GAME_EV_DETECTION_STATE: HandleNetMessage_GAME_EV_DETECTION_STATE (message); break;
		case GAME_EV_CLEAR_ANSWER: HandleNetMessage_GAME_EV_CLEAR_ANSWER (message); break;
		case GAME_EV_STOP_CLEARING: HandleNetMessage_GAME_EV_STOP_CLEARING (message); break;
		case GAME_EV_NOFOG: HandleNetMessage_GAME_EV_NOFOG (message); break;
		case GAME_EV_DEFEATED: HandleNetMessage_GAME_EV_DEFEATED (message); break;
		case GAME_EV_FREEZE: HandleNetMessage_GAME_EV_FREEZE (message); break;
		case GAME_EV_UNFREEZE: HandleNetMessage_GAME_EV_UNFREEZE (message); break;
		case GAME_EV_DEL_PLAYER: HandleNetMessage_GAME_EV_DEL_PLAYER (message); break;
		case GAME_EV_TURN: HandleNetMessage_GAME_EV_TURN (message); break;
		case GAME_EV_HUD_SETTINGS: HandleNetMessage_GAME_EV_HUD_SETTINGS (message); break;
		case GAME_EV_STORE_UNIT: HandleNetMessage_GAME_EV_STORE_UNIT (message); break;
		case GAME_EV_EXIT_UNIT: HandleNetMessage_GAME_EV_EXIT_UNIT (message); break;
		case GAME_EV_DELETE_EVERYTHING: HandleNetMessage_GAME_EV_DELETE_EVERYTHING (message); break;
		case GAME_EV_UNIT_UPGRADE_VALUES: HandleNetMessage_GAME_EV_UNIT_UPGRADE_VALUES (message); break;
		case GAME_EV_CREDITS_CHANGED: HandleNetMessage_GAME_EV_CREDITS_CHANGED (message); break;
		case GAME_EV_UPGRADED_BUILDINGS: HandleNetMessage_GAME_EV_UPGRADED_BUILDINGS (message); break;
		case GAME_EV_UPGRADED_VEHICLES: HandleNetMessage_GAME_EV_UPGRADED_VEHICLES (message); break;
		case GAME_EV_RESEARCH_SETTINGS: HandleNetMessage_GAME_EV_RESEARCH_SETTINGS (message); break;
		case GAME_EV_RESEARCH_LEVEL: HandleNetMessage_GAME_EV_RESEARCH_LEVEL (message); break;
		case GAME_EV_FINISHED_RESEARCH_AREAS: HandleNetMessage_GAME_EV_FINISHED_RESEARCH_AREAS (message); break;
		case GAME_EV_REFRESH_RESEARCH_COUNT: // sent, when the player was resynced (or a game was loaded)
			HandleNetMessage_GAME_EV_REFRESH_RESEARCH_COUNT (message); break;
		case GAME_EV_SET_AUTOMOVE: HandleNetMessage_GAME_EV_SET_AUTOMOVE (message); break;
		case GAME_EV_COMMANDO_ANSWER: HandleNetMessage_GAME_EV_COMMANDO_ANSWER (message); break;
		case GAME_EV_REQ_SAVE_INFO: HandleNetMessage_GAME_EV_REQ_SAVE_INFO (message); break;
		case GAME_EV_SAVED_REPORT: HandleNetMessage_GAME_EV_SAVED_REPORT (message); break;
		case GAME_EV_CASUALTIES_REPORT: HandleNetMessage_GAME_EV_CASUALTIES_REPORT (message); break;
		case GAME_EV_SCORE: HandleNetMessage_GAME_EV_SCORE (message); break;
		case GAME_EV_NUM_ECOS: HandleNetMessage_GAME_EV_NUM_ECOS (message); break;
		case GAME_EV_UNIT_SCORE: HandleNetMessage_GAME_EV_UNIT_SCORE (message); break;
		case GAME_EV_GAME_SETTINGS: HandleNetMessage_GAME_EV_GAME_SETTINGS (message); break;
		case GAME_EV_SELFDESTROY: HandleNetMessage_GAME_EV_SELFDESTROY (message); break;
		case GAME_EV_END_MOVE_ACTION_SERVER: HandleNetMessage_GAME_EV_END_MOVE_ACTION_SERVER (message); break;
		case GAME_EV_SET_GAME_TIME: HandleNetMessage_GAME_EV_SET_GAME_TIME (message); break;
		case GAME_EV_REVEAL_MAP: HandleNetMessage_GAME_EV_REVEAL_MAP (message); break;
		case NET_GAME_TIME_SERVER: gameTimer->handleSyncMessage (message); break;

		default:
			Log.write ("Client: Can not handle message type " + message.getTypeAsString(), cLog::eLOG_TYPE_NET_ERROR);
			break;
	}

	return 0;
}

void cClient::addUnit (const cPosition& position, cVehicle& addedVehicle, bool addToMap)
{
	// place the vehicle
	if (addToMap) getMap ()->addVehicle (addedVehicle, position);

	if (addedVehicle.getOwner () != ActivePlayer && addedVehicle.iID == ActivePlayer->lastDeletedUnit)
	{
		ActivePlayer->addSavedReport (std::make_unique<cSavedReportCapturedByEnemy> (addedVehicle));
		unitStolen (addedVehicle);
	}
	else if (addedVehicle.getOwner () != ActivePlayer)
	{
		ActivePlayer->addSavedReport (std::make_unique<cSavedReportDetected> (addedVehicle));
		unitDetected (addedVehicle);
	}
}

void cClient::addUnit(const cPosition& position, cBuilding& addedBuilding)
{
	// place the building
	getMap()->addBuilding (addedBuilding, position);
}

cPlayer* cClient::getPlayerFromNumber (int iNum)
{
	for (unsigned int i = 0; i < getPlayerList().size(); i++)
	{
		cPlayer& p = *getPlayerList() [i];
		if (p.getNr() == iNum) return &p;
	}
	return NULL;
}

cPlayer* cClient::getPlayerFromString (const string& playerID)
{
	// first try to find player by number
	const int playerNr = atoi (playerID.c_str());
	if (playerNr != 0 || playerID[0] == '0')
	{
		return getPlayerFromNumber (playerNr);
	}

	// try to find plyer by name
	for (unsigned int i = 0; i < playerList.size (); i++)
	{
		if (playerList[i]->getName () == playerID) return playerList[i].get();
	}
	return NULL;
}

void cClient::deleteUnit (cBuilding* building)
{
	if (!building) return;

	getMap ()->deleteBuilding (*building);

	if (!building->getOwner ())
	{
		auto iter = neutralBuildings.find (*building);
		assert (iter != neutralBuildings.end());
		if (iter != neutralBuildings.end ()) neutralBuildings.erase (iter);
		return;
	}

	for (unsigned int i = 0; i < attackJobs.size(); i++)
	{
		attackJobs[i]->onRemoveUnit (*building);
	}

	auto owner = building->getOwner ();

	if (owner == ActivePlayer)
	{
		owner->base.deleteBuilding (building, NULL);
	}

	owner->removeUnit (*building);

	owner->doScan();
}

void cClient::deleteUnit (cVehicle* vehicle)
{
	if (!vehicle) return;

	getMap ()->deleteVehicle (*vehicle);

	for (unsigned int i = 0; i < attackJobs.size(); i++)
	{
		attackJobs[i]->onRemoveUnit (*vehicle);
	}
	helperJobs.onRemoveUnit (vehicle);

	auto owner = vehicle->getOwner ();

	owner->lastDeletedUnit = vehicle->iID;

	owner->removeUnit (*vehicle);

	owner->doScan();
}

void cClient::handleEnd()
{
	if (isFreezed()) return;
	bWantToEnd = true;
	sendWantToEndTurn (*this);
	startedTurnEndProcess ();
}


void cClient::addActiveMoveJob (cClientMoveJob& MoveJob)
{
	MoveJob.bSuspended = false;
	if (MoveJob.Vehicle) MoveJob.Vehicle->MoveJobActive = true;
	if (Contains (ActiveMJobs, &MoveJob)) return;
	ActiveMJobs.push_back (&MoveJob);
}

void cClient::addAutoMoveJob (std::weak_ptr<cAutoMJob> autoMoveJob)
{
	autoMoveJobs.push_back (std::move (autoMoveJob));
}

void cClient::handleMoveJobs()
{
	for (int i = ActiveMJobs.size() - 1; i >= 0; i--)
	{
		cClientMoveJob* MoveJob = ActiveMJobs[i];
		cVehicle* Vehicle = MoveJob->Vehicle;

		// suspend movejobs of attacked vehicles
		if (Vehicle && Vehicle->isBeeingAttacked ()) continue;

		if (MoveJob->bFinished || MoveJob->bEndForNow)
		{
			if(Vehicle) MoveJob->stopped(*Vehicle);
		}

		if (MoveJob->bFinished)
		{
			if (Vehicle && Vehicle->getClientMoveJob () == MoveJob)
			{
				Log.write (" Client: Movejob is finished and will be deleted now", cLog::eLOG_TYPE_NET_DEBUG);
				Vehicle->setClientMoveJob (nullptr);
				Vehicle->setMoving (false);
				Vehicle->MoveJobActive = false;
			}
			else Log.write (" Client: Delete movejob with nonactive vehicle (released one)", cLog::eLOG_TYPE_NET_DEBUG);
			ActiveMJobs.erase (ActiveMJobs.begin() + i);
			delete MoveJob;
			continue;
		}
		if (MoveJob->bEndForNow)
		{
			Log.write (" Client: Movejob has end for now and will be stopped (delete from active ones)", cLog::eLOG_TYPE_NET_DEBUG);
			if (Vehicle)
			{
				Vehicle->MoveJobActive = false;
				Vehicle->setMoving (false);
			}
			ActiveMJobs.erase (ActiveMJobs.begin () + i);
			continue;
		}

		if (Vehicle == NULL) continue;


		if (MoveJob->iNextDir != Vehicle->dir && Vehicle->data.speedCur)
		{
			// rotate vehicle
			if (gameTimer->timer100ms)
			{
				Vehicle->rotateTo (MoveJob->iNextDir);
			}
		}
		else if (Vehicle->MoveJobActive)
		{
			// move vehicle
			if (gameTimer->timer10ms)
			{
				MoveJob->moveVehicle();
			}
		}
	}
}

void cClient::handleAutoMoveJobs ()
{
	std::vector<cAutoMJob*> activeAutoMoveJobs;

	// clean up deleted and finished move jobs
	for (auto i = autoMoveJobs.begin (); i != autoMoveJobs.end (); /*erase in loop*/)
	{
		if (i->expired ())
		{
			i = autoMoveJobs.erase (i);
		}
		else
		{
			auto job = i->lock ();
			if (job->isFinished ())
			{
				job->getVehicle ().stopAutoMoveJob ();
				if (job.use_count() == 1)
				{
					i = autoMoveJobs.erase (i);
				}
			}
			else
			{
				activeAutoMoveJobs.push_back (job.get ());
				++i;
			}
		}
	}

	// We do not want to execute all auto move jobs at the same time.
	// instead we do a round-robin like scheduling for the auto move jobs:
	// we do executed only a limited number of auto move jobs at a time.
	// To do so we execute only the first N auto move jobs in the list
	// and afterwards push them to the end of the list so that they will only be executed
	// when all the other jobs have been executed once as well.
	const size_t maxConcurrentJobs = 2;

	auto jobsToExecute = std::min (activeAutoMoveJobs.size (), maxConcurrentJobs);

	while (jobsToExecute > 0)
	{
		auto job = autoMoveJobs.front ().lock ();
		job->doAutoMove (activeAutoMoveJobs);

		autoMoveJobs.pop_front ();
		autoMoveJobs.push_back (job);

		--jobsToExecute;
	}
}

cVehicle* cClient::getVehicleFromID (unsigned int id)
{
	for (unsigned int i = 0; i < getPlayerList ().size (); i++)
	{
		cPlayer& player = *getPlayerList ()[i];
		auto unit = player.getVehicleFromId (id);
		if (unit) return unit;
	}
	return nullptr;
}

cBuilding* cClient::getBuildingFromID (unsigned int id)
{
	for (unsigned int i = 0; i < getPlayerList().size(); i++)
	{
		cPlayer& player = *getPlayerList() [i];
		auto unit = player.getBuildingFromId (id);
		if (unit) return unit;
	}
	auto iter = neutralBuildings.find (id);
	return iter == neutralBuildings.end () ? nullptr : iter->get ();
}

void cClient::doGameActions ()
{
	runFx();

	// run attackJobs
	if (gameTimer->timer50ms)
		cClientAttackJob::handleAttackJobs (*this);

	// run moveJobs - this has to be called before handling the auto movejobs
	handleMoveJobs();

	// run surveyor ai
	if (gameTimer->timer50ms)
	{
		handleAutoMoveJobs ();
	}

	runJobs();
}

sSubBase* cClient::getSubBaseFromID (int iID)
{
	cBuilding* building = getBuildingFromID (iID);
	if (building)
		return building->SubBase;
	return NULL;
}

void cClient::destroyUnit (cVehicle& vehicle)
{
	// play explosion
	if (vehicle.data.isBig)
	{
		addFx (std::make_shared<cFxExploBig> (vehicle.getPosition() * 64 + 64, getMap ()->isWaterOrCoast (vehicle.getPosition())));
	}
	else if (vehicle.data.factorAir > 0 && vehicle.getFlightHeight () != 0)
	{
		addFx(std::make_shared<cFxExploAir>(vehicle.getPosition() * 64 + vehicle.getMovementOffset() + 32));
	}
	else if (getMap()->isWaterOrCoast (vehicle.getPosition()))
	{
		addFx (std::make_shared<cFxExploWater> (vehicle.getPosition() * 64 + vehicle.getMovementOffset() + 32));
	}
	else
	{
		addFx (std::make_shared<cFxExploSmall> (vehicle.getPosition() * 64 + vehicle.getMovementOffset() + 32));
	}

	if (vehicle.data.hasCorpse)
	{
		// add corpse
		addFx (std::make_shared<cFxCorpse> (vehicle.getPosition() * 64 + vehicle.getMovementOffset() + 32));
	}
}

void cClient::destroyUnit (cBuilding& building)
{
	// play explosion animation
	cBuilding* topBuilding = getMap()->getField(building.getPosition()).getBuilding();
	if (topBuilding && topBuilding->data.isBig)
	{
		addFx (std::make_shared<cFxExploBig> (topBuilding->getPosition() * 64 + 64, getMap ()->isWaterOrCoast (topBuilding->getPosition())));
	}
	else
	{
		addFx (std::make_shared<cFxExploSmall> (building.getPosition() * 64 + 32));
	}
}

//------------------------------------------------------------------------------
void cClient::deletePlayer (cPlayer& player)
{
	player.setIsRemovedFromGame (true);
	playerList.erase (std::remove_if (playerList.begin (), playerList.end (), [&player](const std::shared_ptr<cPlayer>& entry){ return entry.get () == &player; }), playerList.end ());
}

//------------------------------------------------------------------------------
void cClient::addJob (cJob* job)
{
	helperJobs.addJob (*job);
}

//------------------------------------------------------------------------------
void cClient::runJobs()
{
	helperJobs.run (*gameTimer);
}

//------------------------------------------------------------------------------
void cClient::enableFreezeMode (eFreezeMode mode, int playerNumber)
{
	freezeModes.enable (mode, playerNumber);
	freezeModeChanged ();
}

//------------------------------------------------------------------------------
void cClient::disableFreezeMode (eFreezeMode mode)
{
	freezeModes.disable (mode);
	freezeModeChanged ();
}

//------------------------------------------------------------------------------
bool cClient::isFreezed() const
{
	return freezeModes.isFreezed();
}

//------------------------------------------------------------------------------
bool cClient::getFreezeMode (eFreezeMode mode) const
{
	return freezeModes.isEnable (mode);
}

//------------------------------------------------------------------------------
void cClient::handleChatMessage (const std::string& message)
{
    if (message.empty ()) return;

    sendChatMessageToServer (*this, getActivePlayer (), message);
}