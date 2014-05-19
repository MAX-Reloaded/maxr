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

#include "player.h"

#include "buildings.h"
#include "client.h"
#include "clist.h"
#include "hud.h"
#include "netmessage.h"
#include "server.h"
#include "serverevents.h"
#include "vehicles.h"
#include "game/data/report/savedreport.h"

using namespace std;

//------------------------------------------------------------------------------
sPlayer::sPlayer (const string& name_, unsigned int colorIndex_, int nr_, int socketIndex_) :
	name (name_),
	colorIndex (colorIndex_),
	Nr (nr_),
	socketIndex (socketIndex_),
	ready (false)
{
	assert (colorIndex < PLAYERCOLORS);
}

//------------------------------------------------------------------------------
sPlayer::sPlayer (const sPlayer& other) :
	name (other.name),
	colorIndex (other.colorIndex),
	Nr (other.Nr),
	socketIndex (other.socketIndex),
	ready (other.ready)
{}

//------------------------------------------------------------------------------
sPlayer& sPlayer::operator=(const sPlayer& other)
{
	name = other.name;
	colorIndex = other.colorIndex;
	Nr = other.Nr;
	socketIndex = other.socketIndex;
	ready = other.ready;
	return *this;
}

//------------------------------------------------------------------------------
const std::string& sPlayer::getName () const
{
	return name;
}

//------------------------------------------------------------------------------
void sPlayer::setName (std::string name_)
{
	std::swap (name, name_);
	if (name != name_) nameChanged ();
}

//------------------------------------------------------------------------------
int sPlayer::getNr () const
{
	return Nr;
}

//------------------------------------------------------------------------------
void sPlayer::setNr (int nr)
{
	std::swap (Nr, nr);
	if (Nr != nr) numberChanged ();
}

//------------------------------------------------------------------------------
int sPlayer::getSocketIndex () const
{
	return socketIndex;
}

//------------------------------------------------------------------------------
void sPlayer::setSocketIndex (int index)
{
	std::swap (socketIndex, index);
	if (socketIndex != index) socketIndexChanged ();
}

//------------------------------------------------------------------------------
void sPlayer::setLocal()
{
	socketIndex = MAX_CLIENTS;
}

//------------------------------------------------------------------------------
bool sPlayer::isLocal() const
{
	return socketIndex == MAX_CLIENTS;
}

//------------------------------------------------------------------------------
void sPlayer::onSocketIndexDisconnected (int socketIndex_)
{
	if (isLocal() || socketIndex == -1) return;
	if (socketIndex == socketIndex_)
	{
		socketIndex = -1;
	}
	else if (socketIndex > socketIndex_)
	{
		--socketIndex;
	}
}

//------------------------------------------------------------------------------
SDL_Surface* sPlayer::getColorSurface() const
{
	return OtherData.colors[getColorIndex ()].get ();
}

//------------------------------------------------------------------------------
void sPlayer::setColorIndex (unsigned int index)
{
	assert (index < PLAYERCOLORS);
	std::swap(colorIndex, index);
	if (colorIndex != index) colorChanged();
}

//------------------------------------------------------------------------------
void sPlayer::setToNextColorIndex()
{
	setColorIndex ((colorIndex + 1) % PLAYERCOLORS);
}

//------------------------------------------------------------------------------
void sPlayer::setToPrevColorIndex()
{
	setColorIndex ((colorIndex - 1 + PLAYERCOLORS) % PLAYERCOLORS);
}

//------------------------------------------------------------------------------
void sPlayer::setReady (bool ready_)
{
	std::swap (ready, ready_);
	if (ready != ready_) readyChanged ();
}

//------------------------------------------------------------------------------
bool sPlayer::isReady () const
{
	return ready;
}

//------------------------------------------------------------------------------
// Implementation cPlayer class
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
cPlayer::cPlayer (const sPlayer& splayer_) :
	splayer (splayer_),
	landingPosX (-1),
	landingPosY (-1),
	savedHud (new sHudStateContainer),
	numEcos (0),
	lastDeletedUnit (0),
	clan (-1)
{
	// get the default (no clan) unit data
	VehicleData = UnitsData.getUnitData_Vehicles (-1);
	BuildingData = UnitsData.getUnitData_Buildings (-1);

	VehicleList = NULL;
	BuildingList = NULL;

	workingResearchCenterCount = 0;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		researchCentersWorkingOnArea[i] = 0;
	Credits = 0;

	isDefeated = false;
	isRemovedFromGame = false;
	bFinishedTurn = false;

	researchFinished = false;
}

//------------------------------------------------------------------------------
cPlayer::~cPlayer()
{
	deleteAllUnits ();

	for (size_t i = 0; i != ReportVehicles.size(); ++i)
	{
		delete ReportVehicles[i];
	}
	ReportVehicles.clear();
	for (size_t i = 0; i != ReportBuildings.size(); ++i)
	{
		delete ReportBuildings[i];
	}
	ReportBuildings.clear();
}

//------------------------------------------------------------------------------
void cPlayer::setClan (int newClan)
{
	if (newClan == clan || newClan < -1 || 7 < newClan)
		return;

	clan = newClan;

	VehicleData = UnitsData.getUnitData_Vehicles (clan);
	BuildingData = UnitsData.getUnitData_Buildings (clan);
}

//------------------------------------------------------------------------------
sUnitData* cPlayer::getUnitDataCurrentVersion (const sID& id)
{
	const cPlayer* constMe = this;
	return const_cast<sUnitData*>(constMe->getUnitDataCurrentVersion (id));
}

//------------------------------------------------------------------------------
const sUnitData* cPlayer::getUnitDataCurrentVersion (const sID& id) const
{
	if (id.isAVehicle ())
	{
		for (size_t i = 0; i != VehicleData.size (); ++i)
		{
			if (VehicleData[i].ID == id) return &VehicleData[i];
		}
	}
	else if (id.isABuilding ())
	{
		for (unsigned int i = 0; i < BuildingData.size (); ++i)
		{
			if (BuildingData[i].ID == id) return &BuildingData[i];
		}
	}
	return nullptr;
}

//------------------------------------------------------------------------------
/** Adds the vehicle to the list of the player */
//------------------------------------------------------------------------------
cVehicle* cPlayer::addVehicle(const cPosition& position, const sID& id, unsigned int uid)
{
	const sUnitData& unitData = *id.getUnitDataOriginalVersion (this);
	cVehicle* n = new cVehicle (unitData, this, uid);
	n->setPosition(position);

	addUnitToList (*n);

	drawSpecialCircle(n->getPosition(), n->data.scan, ScanMap, mapSize);
	if (n->data.canDetectStealthOn & TERRAIN_GROUND) drawSpecialCircle (n->getPosition(), n->data.scan, DetectLandMap, mapSize);
	if (n->data.canDetectStealthOn & TERRAIN_SEA) drawSpecialCircle (n->getPosition(), n->data.scan, DetectSeaMap, mapSize);
	if (n->data.canDetectStealthOn & AREA_EXP_MINE)
	{
		const int minx = std::max (n->getPosition().x() - 1, 0);
		const int maxx = std::min (n->getPosition().x() + 1, mapSize.x() - 1);
		const int miny = std::max (n->getPosition().y() - 1, 0);
		const int maxy = std::min (n->getPosition().y() + 1, mapSize.y() - 1);
		for (int x = minx; x <= maxx; ++x)
			for (int y = miny; y <= maxy; ++y)
				DetectMinesMap[x + mapSize.x() * y] = 1;
	}
	return n;
}

//------------------------------------------------------------------------------
/** initialize the maps */
//------------------------------------------------------------------------------
void cPlayer::initMaps (cMap& map)
{
	mapSize = map.getSize ();
	const int size = mapSize.x () * mapSize.y ();
	// Scanner-Map:
	ScanMap.clear();
	ScanMap.resize (size, 0);
	// Ressource-Map
	ResourceMap.clear();
	ResourceMap.resize (size, 0);

	base.map = &map;
	// Sentry-Map:
	SentriesMapAir.clear();
	SentriesMapAir.resize (size, 0);
	SentriesMapGround.clear();
	SentriesMapGround.resize (size, 0);

	// Detect-Maps:
	DetectLandMap.clear();
	DetectLandMap.resize (size, 0);
	DetectSeaMap.clear();
	DetectSeaMap.resize (size, 0);
	DetectMinesMap.clear();
	DetectMinesMap.resize (size, 0);
}

const cPosition& cPlayer::getMapSize () const
{
	return mapSize;
}

template <typename T>
T* getPreviousUnitById (T* root, unsigned int id)
{
	if (root == 0 || id < root->iID) return 0;
	T* it = root;
	for (; it->next; it = it->next)
	{
		if (it->next->iID > id)
			return it;
	}
	return it;
}

void cPlayer::addUnitToList (cUnit& addedUnit)
{
	//units in the linked list are sorted in increasing order of IDs

	//find unit before the added unit
	if (addedUnit.isABuilding())
	{
		cBuilding* addedBuilding = static_cast<cBuilding*> (&addedUnit);
		cBuilding* prev = getPreviousUnitById (BuildingList, addedUnit.iID);
		insert_after_in_intrusivelist (BuildingList, prev, *addedBuilding);
	}
	else
	{
		cVehicle* addedVehicle = static_cast<cVehicle*> (&addedUnit);
		cVehicle* prev = getPreviousUnitById (VehicleList, addedUnit.iID);
		insert_after_in_intrusivelist (VehicleList, prev, *addedVehicle);
	}
}

//------------------------------------------------------------------------------
/** Adds the building to the list of the player */
//------------------------------------------------------------------------------
cBuilding* cPlayer::addBuilding (const cPosition& position, const sID& id, unsigned int uid)
{
	const sUnitData* unitData = id.getUnitDataOriginalVersion (this);
	cBuilding* Building = new cBuilding (unitData, this, uid);

	Building->setPosition(position);

	addUnitToList (*Building);

	if (Building->data.scan)
	{
		if (Building->data.isBig) drawSpecialCircleBig (Building->getPosition(), Building->data.scan, ScanMap, mapSize);
		else drawSpecialCircle (Building->getPosition(), Building->data.scan, ScanMap, mapSize);
	}
	return Building;
}

//------------------------------------------------------------------------------
void cPlayer::deleteAllUnits ()
{
	// first delete all stored vehicles
	for (cVehicle* ptr = VehicleList; ptr; ptr = ptr->next)
	{
		ptr->deleteStoredUnits ();
	}
	// now delete all other vehicles
	while (VehicleList)
	{
		cVehicle* ptr = VehicleList->next;
		VehicleList->setSentryActive (false);
		delete VehicleList;
		VehicleList = ptr;
	}
	while (BuildingList)
	{
		cBuilding* ptr = BuildingList->next;
		BuildingList->setSentryActive (false);

		// Stored Vehicles are already deleted; just clear the list
		BuildingList->storedUnits.clear ();

		delete BuildingList;
		BuildingList = ptr;
	}
}

//------------------------------------------------------------------------------
void cPlayer::addSentry (cUnit& u)
{
	u.setSentryActive (true);
	if (u.data.canAttack & TERRAIN_AIR)
	{
		drawSpecialCircle (u.getPosition(), u.data.range, SentriesMapAir, mapSize);
	}
	if ((u.data.canAttack & TERRAIN_GROUND) || (u.data.canAttack & TERRAIN_SEA))
	{
		drawSpecialCircle (u.getPosition(), u.data.range, SentriesMapGround, mapSize);
	}
}

//------------------------------------------------------------------------------
void cPlayer::deleteSentry (cUnit& u)
{
	u.setSentryActive (false);
	if (u.data.canAttack & TERRAIN_AIR)
	{
		refreshSentryAir();
	}
	else if ((u.data.canAttack & TERRAIN_GROUND) || (u.data.canAttack & TERRAIN_SEA))
	{
		refreshSentryGround();
	}
}

//------------------------------------------------------------------------------
void cPlayer::refreshSentryAir()
{
	std::fill (SentriesMapAir.begin(), SentriesMapAir.end(), 0);

	for (const cVehicle* unit = VehicleList; unit; unit = unit->next)
	{
		if (unit->isSentryActive() && unit->data.canAttack & TERRAIN_AIR)
		{
			drawSpecialCircle (unit->getPosition(), unit->data.range, SentriesMapAir, mapSize);
		}
	}

	for (const cBuilding* unit = BuildingList; unit; unit = unit->next)
	{
		if (unit->isSentryActive() && unit->data.canAttack & TERRAIN_AIR)
		{
			drawSpecialCircle (unit->getPosition(), unit->data.range, SentriesMapAir, mapSize);
		}
	}
}

//------------------------------------------------------------------------------
void cPlayer::refreshSentryGround()
{
	std::fill (SentriesMapGround.begin(), SentriesMapGround.end(), 0);

	for (const cVehicle* unit = VehicleList; unit; unit = unit->next)
	{
		if (unit->isSentryActive() && ((unit->data.canAttack & TERRAIN_GROUND) || (unit->data.canAttack & TERRAIN_SEA)))
		{
			drawSpecialCircle (unit->getPosition(), unit->data.range, SentriesMapGround, mapSize);
		}
	}
	for (const cBuilding* unit = BuildingList; unit; unit = unit->next)
	{
		if (unit->isSentryActive() && ((unit->data.canAttack & TERRAIN_GROUND) || (unit->data.canAttack & TERRAIN_SEA)))
		{
			drawSpecialCircle (unit->getPosition(), unit->data.range, SentriesMapGround, mapSize);
		}
	}
}

//------------------------------------------------------------------------------
/** Does a scan for all units of the player */
//------------------------------------------------------------------------------
void cPlayer::doScan()
{
	if (isDefeated) return;
	std::fill (ScanMap.begin(), ScanMap.end(), 0);
	std::fill (DetectLandMap.begin(), DetectLandMap.end(), 0);
	std::fill (DetectSeaMap.begin(), DetectSeaMap.end(), 0);
	std::fill (DetectMinesMap.begin(), DetectMinesMap.end(), 0);

	// iterate the vehicle list
	for (const cVehicle* vp = VehicleList; vp; vp = vp->next)
	{
		if (vp->isUnitLoaded ()) continue;

		if (vp->isDisabled())
			ScanMap[getOffset (vp->getPosition())] = 1;
		else
		{
			if (vp->data.isBig)
				drawSpecialCircleBig (vp->getPosition(), vp->data.scan, ScanMap, mapSize);
			else
				drawSpecialCircle (vp->getPosition(), vp->data.scan, ScanMap, mapSize);

			//detection maps
			if (vp->data.canDetectStealthOn & TERRAIN_GROUND) drawSpecialCircle (vp->getPosition(), vp->data.scan, DetectLandMap, mapSize);
			else if (vp->data.canDetectStealthOn & TERRAIN_SEA) drawSpecialCircle (vp->getPosition(), vp->data.scan, DetectSeaMap, mapSize);
			if (vp->data.canDetectStealthOn & AREA_EXP_MINE)
			{
				const int minx = std::max (vp->getPosition().x() - 1, 0);
				const int maxx = std::min (vp->getPosition().x() + 1, mapSize.x() - 1);
				const int miny = std::max (vp->getPosition().y() - 1, 0);
				const int maxy = std::min (vp->getPosition().y() + 1, mapSize.y() - 1);
				for (int x = minx; x <= maxx; ++x)
				{
					for (int y = miny; y <= maxy; ++y)
					{
						DetectMinesMap[x + mapSize.x() * y] = 1;
					}
				}
			}
		}
	}

	// iterate the building list
	for (const cBuilding* bp = BuildingList; bp; bp = bp->next)
	{
		if (bp->isDisabled())
			ScanMap[getOffset (bp->getPosition())] = 1;
		else if (bp->data.scan)
		{
			if (bp->data.isBig)
				drawSpecialCircleBig (bp->getPosition(), bp->data.scan, ScanMap, mapSize);
			else
				drawSpecialCircle (bp->getPosition(), bp->data.scan, ScanMap, mapSize);
		}
	}
}

void cPlayer::revealMap()
{
	std::fill (ScanMap.begin(), ScanMap.end(), 1);
}

void cPlayer::revealPosition (const cPosition& position)
{
	if (position.x () < 0 || position.x () >= mapSize.x () || position.y () < 0 || position.y () >= mapSize.y ()) return;

	ScanMap[getOffset (position)] = 1;
}

void cPlayer::revealResource()
{
	std::fill (ResourceMap.begin(), ResourceMap.end(), 1);
}

bool cPlayer::canSeeAnyAreaUnder (const cUnit& unit) const
{
	if (canSeeAt (unit.getPosition ())) return true;
	if (!unit.data.isBig) return false;

	return canSeeAt (unit.getPosition () + cPosition (0, 1)) ||
		canSeeAt (unit.getPosition () + cPosition (1, 1)) ||
		canSeeAt (unit.getPosition () + cPosition (1, 0));
}

cVehicle* cPlayer::getNextVehicle (cVehicle* start)
{
	start = (start == NULL) ? VehicleList : start->next;
	for (cVehicle* it = start; it; it = it->next)
	{
		if (!it->isMarkedAsDone () && (!it->isUnitBuildingABuilding () || it->getBuildTurns () == 0)
			&& !it->isUnitClearing () && !it->isSentryActive () && !it->isUnitLoaded ()
			&& (it->data.speedCur || it->data.getShots ()))
			return it;
	}
	return NULL;
}

cBuilding* cPlayer::getNextBuilding (cBuilding* start)
{
	start = (start == NULL) ? BuildingList : start->next;
	for (cBuilding* it = start; it; it = it->next)
	{
		if (!it->isMarkedAsDone () && !it->isUnitWorking () && !it->isSentryActive ()
			&& (!it->data.canBuild.empty () || it->data.getShots ()
				|| it->data.canMineMaxRes > 0 || it->data.convertsGold > 0
				|| it->data.canResearch))
			return it;
	}
	return NULL;
}

cBuilding* cPlayer::getNextMiningStation (cBuilding* start)
{
	start = (start == NULL) ? BuildingList : start->next;
	for (cBuilding* it = start; it; it = it->next)
	{
		if (it->data.canMineMaxRes > 0)
			return it;
	}
	return NULL;
}

//------------------------------------------------------------------------------
/** Returns the next unit that can still fire/shoot */
//------------------------------------------------------------------------------
cUnit* cPlayer::getNextUnit (cUnit* start)
{
	if (start == NULL || start->owner != this)
	{
		cVehicle* nextVehicle = getNextVehicle (NULL);
		if (nextVehicle) return nextVehicle;
		cBuilding* nextBuilding = getNextBuilding (NULL);
		if (nextBuilding) return nextBuilding;
	}
	else if (start->isAVehicle())
	{
		cVehicle* nextVehicle = getNextVehicle (static_cast<cVehicle*> (start));
		if (nextVehicle) return nextVehicle;
		cBuilding* nextBuilding = getNextBuilding (NULL);
		if (nextBuilding) return nextBuilding;
		nextVehicle = getNextVehicle (NULL);
		if (nextVehicle) return nextVehicle;
	}
	else
	{
		assert (start->isABuilding());
		cBuilding* building = static_cast<cBuilding*> (start);
		cBuilding* nextBuilding = getNextBuilding (building);
		if (nextBuilding) return nextBuilding;
		cVehicle* nextVehicle = getNextVehicle (NULL);
		if (nextVehicle) return nextVehicle;
		nextBuilding = getNextBuilding (NULL);
		if (nextBuilding) return nextBuilding;
	}
	// finally, return the more recent built Mining station.
	// since list order is by increasing age, take the first in list.
	return getNextMiningStation (NULL);
}

cVehicle* cPlayer::getPrevVehicle (cVehicle* start)
{
	start = (start == NULL) ? get_last_of_intrusivelist (VehicleList) : start->prev;
	for (cVehicle* it = start; it; it = it->prev)
	{
		if (!it->isMarkedAsDone () && (!it->isUnitBuildingABuilding () || it->getBuildTurns () == 0)
			&& !it->isUnitClearing () && !it->isSentryActive () && !it->isUnitLoaded ()
			&& (it->data.speedCur || it->data.getShots ()))
			return it;
	}
	return NULL;
}

cBuilding* cPlayer::getPrevBuilding (cBuilding* start)
{
	start = (start == NULL) ? get_last_of_intrusivelist (BuildingList) : start->prev;
	for (cBuilding* it = start; it; it = it->prev)
	{
		if (!it->isMarkedAsDone () && !it->isUnitWorking () && !it->isSentryActive ()
			&& (!it->data.canBuild.empty () || it->data.getShots ()
				|| it->data.canMineMaxRes > 0 || it->data.convertsGold > 0
				|| it->data.canResearch))
			return it;
	}
	return NULL;
}

cBuilding* cPlayer::getPrevMiningStation (cBuilding* start)
{
	start = (start == NULL) ? get_last_of_intrusivelist (BuildingList) : start->prev;
	for (cBuilding* it = start; it; it = it->prev)
	{
		if (it->data.canMineMaxRes > 0)
			return it;
	}
	return NULL;
}

//------------------------------------------------------------------------------
/** Returns the previous vehicle, that can still move / shoot */
//------------------------------------------------------------------------------
cUnit* cPlayer::getPrevUnit (cUnit* start)
{
	if (start == NULL || start->owner != this)
	{
		cVehicle* prevVehicle = getPrevVehicle (NULL);
		if (prevVehicle) return prevVehicle;
		cBuilding* prevBuilding = getPrevBuilding (NULL);
		if (prevBuilding) return prevBuilding;
	}
	else if (start->isAVehicle())
	{
		cVehicle* prevVehicle = getPrevVehicle (static_cast<cVehicle*> (start));
		if (prevVehicle) return prevVehicle;
		cBuilding* prevBuilding = getPrevBuilding (NULL);
		if (prevBuilding) return prevBuilding;
		prevVehicle = getPrevVehicle (NULL);
		if (prevVehicle) return prevVehicle;
	}
	else
	{
		assert (start->isABuilding());
		cBuilding* building = static_cast<cBuilding*> (start);
		cBuilding* prevBuilding = getPrevBuilding (building);
		if (prevBuilding) return prevBuilding;
		cVehicle* prevVehicle = getPrevVehicle (NULL);
		if (prevVehicle) return prevVehicle;
		prevBuilding = getPrevBuilding (NULL);
		if (prevBuilding) return prevBuilding;
	}
	// finally, return the more recent built Mining station.
	// since list order is by increasing age, take the first in list.
	return getNextMiningStation (NULL);
}

//------------------------------------------------------------------------------
/** Starts a research center. */
//------------------------------------------------------------------------------
void cPlayer::startAResearch (int researchArea)
{
	if (0 <= researchArea && researchArea <= cResearch::kNrResearchAreas)
	{
		workingResearchCenterCount++;
		researchCentersWorkingOnArea[researchArea] += 1;
	}
}

//------------------------------------------------------------------------------
/** Stops a research center. */
//------------------------------------------------------------------------------
void cPlayer::stopAResearch (int researchArea)
{
	if (0 <= researchArea && researchArea <= cResearch::kNrResearchAreas)
	{
		workingResearchCenterCount--;
		if (researchCentersWorkingOnArea[researchArea] > 0)
			researchCentersWorkingOnArea[researchArea] -= 1;
	}
}

//------------------------------------------------------------------------------
/** At turnend update the research level */
//------------------------------------------------------------------------------
void cPlayer::doResearch (cServer& server)
{
	bool researchFinished = false;
	std::vector<sUnitData*> upgradedUnitDatas;
	std::vector<int> areasReachingNextLevel;
	reportResearchAreasFinished.clear();
	for (int area = 0; area < cResearch::kNrResearchAreas; ++area)
	{
		if (researchCentersWorkingOnArea[area] > 0 &&
			researchLevel.doResearch (researchCentersWorkingOnArea[area], area))
		{
			// next level reached
			areasReachingNextLevel.push_back (area);
			reportResearchAreasFinished.push_back (area);
			researchFinished = true;
		}
	}
	if (researchFinished)
	{
		upgradeUnitTypes (areasReachingNextLevel, upgradedUnitDatas);

		for (size_t i = 0; i != upgradedUnitDatas.size(); ++i)
			sendUnitUpgrades (server, *upgradedUnitDatas[i], *this);
	}
	sendResearchLevel (server, researchLevel, *this);
}

void cPlayer::accumulateScore (cServer& server)
{
	const int now = server.getTurn();
	int deltaScore = 0;

	for (cBuilding* bp = BuildingList; bp; bp = bp->next)
	{
		if (bp->data.canScore && bp->isUnitWorking ())
		{
			bp->points++;
			deltaScore++;

			sendUnitScore (server, *bp);
		}
	}
	setScore (getScore (now) + deltaScore, now);
	sendScore (server, *this, now);
}

void cPlayer::countEcoSpheres()
{
	numEcos = 0;

	for (const cBuilding* bp = BuildingList; bp; bp = bp->next)
	{
		if (bp->data.canScore && bp->isUnitWorking ())
			++numEcos;
	}
}

void cPlayer::setScore (int s, int turn)
{
	// turn begins at 1.
	unsigned int t = turn;

	if (pointsHistory.size() < t)
		pointsHistory.resize (t);
	pointsHistory[t - 1] = s;
}

void cPlayer::clearDone()
{
	for (cVehicle* unit = VehicleList; unit; unit = unit->next)
	{
		unit->setMarkedAsDone(false);
	}

	for (cBuilding* unit = BuildingList; unit; unit = unit->next)
	{
		unit->setMarkedAsDone(false);
	}
}

int cPlayer::getScore (int turn) const
{
	// turn begins at 1.
	unsigned int t = turn;

	if (pointsHistory.size() < t)
	{
		const int score = pointsHistory.empty() ? 0 : pointsHistory.back();
		pointsHistory.resize (t, score);
	}
	return pointsHistory[t - 1];
}

int cPlayer::getScore () const
{
	return pointsHistory.back ();
}

//------------------------------------------------------------------------------
void cPlayer::upgradeUnitTypes (const std::vector<int>& areasReachingNextLevel, std::vector<sUnitData*>& resultUpgradedUnitDatas)
{
	for (unsigned int i = 0; i < UnitsData.getNrVehicles(); i++)
	{
		const sUnitData& originalData = UnitsData.getVehicle (i, getClan());
		bool incrementVersion = false;
		for (size_t areaCounter = 0; areaCounter != areasReachingNextLevel.size(); areaCounter++)
		{
			const int researchArea = areasReachingNextLevel[areaCounter];
			const int newResearchLevel = researchLevel.getCurResearchLevel (researchArea);
			int startValue = 0;
			switch (researchArea)
			{
				case cResearch::kAttackResearch: startValue = originalData.damage; break;
				case cResearch::kShotsResearch: startValue = originalData.shotsMax; break;
				case cResearch::kRangeResearch: startValue = originalData.range; break;
				case cResearch::kArmorResearch: startValue = originalData.armor; break;
				case cResearch::kHitpointsResearch: startValue = originalData.hitpointsMax; break;
				case cResearch::kScanResearch: startValue = originalData.scan; break;
				case cResearch::kSpeedResearch: startValue = originalData.speedMax; break;
				case cResearch::kCostResearch: startValue = originalData.buildCosts; break;
			}
			int oldResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel - 10,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1,
								   originalData.isHuman ? cUpgradeCalculator::kInfantry : cUpgradeCalculator::kStandardUnit);
			int newResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1,
								   originalData.isHuman ? cUpgradeCalculator::kInfantry : cUpgradeCalculator::kStandardUnit);
			if (oldResearchBonus != newResearchBonus)
			{
				switch (researchArea)
				{
					case cResearch::kAttackResearch: VehicleData[i].damage += newResearchBonus - oldResearchBonus; break;
					case cResearch::kShotsResearch: VehicleData[i].shotsMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kRangeResearch: VehicleData[i].range += newResearchBonus - oldResearchBonus; break;
					case cResearch::kArmorResearch: VehicleData[i].armor += newResearchBonus - oldResearchBonus; break;
					case cResearch::kHitpointsResearch: VehicleData[i].hitpointsMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kScanResearch: VehicleData[i].scan += newResearchBonus - oldResearchBonus; break;
					case cResearch::kSpeedResearch: VehicleData[i].speedMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kCostResearch: VehicleData[i].buildCosts += newResearchBonus - oldResearchBonus; break;
				}
				if (researchArea != cResearch::kCostResearch)   // don't increment the version, if the only change are the costs
					incrementVersion = true;
				if (!Contains (resultUpgradedUnitDatas, & (VehicleData[i])))
					resultUpgradedUnitDatas.push_back (& (VehicleData[i]));
			}
		}
		if (incrementVersion)
			VehicleData[i].setVersion(VehicleData[i].getVersion() + 1);
	}

	for (unsigned int i = 0; i < UnitsData.getNrBuildings(); i++)
	{
		const sUnitData& originalData = UnitsData.getBuilding (i, getClan());
		bool incrementVersion = false;
		for (size_t areaCounter = 0; areaCounter != areasReachingNextLevel.size(); areaCounter++)
		{
			const int researchArea = areasReachingNextLevel[areaCounter];
			const int newResearchLevel = researchLevel.getCurResearchLevel (researchArea);

			int startValue = 0;
			switch (researchArea)
			{
				case cResearch::kAttackResearch: startValue = originalData.damage; break;
				case cResearch::kShotsResearch: startValue = originalData.shotsMax; break;
				case cResearch::kRangeResearch: startValue = originalData.range; break;
				case cResearch::kArmorResearch: startValue = originalData.armor; break;
				case cResearch::kHitpointsResearch: startValue = originalData.hitpointsMax; break;
				case cResearch::kScanResearch: startValue = originalData.scan; break;
				case cResearch::kCostResearch: startValue = originalData.buildCosts; break;
			}
			int oldResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel - 10,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1,
								   cUpgradeCalculator::kBuilding);
			int newResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1,
								   cUpgradeCalculator::kBuilding);
			if (oldResearchBonus != newResearchBonus)
			{
				switch (researchArea)
				{
					case cResearch::kAttackResearch: BuildingData[i].damage += newResearchBonus - oldResearchBonus; break;
					case cResearch::kShotsResearch: BuildingData[i].shotsMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kRangeResearch: BuildingData[i].range += newResearchBonus - oldResearchBonus; break;
					case cResearch::kArmorResearch: BuildingData[i].armor += newResearchBonus - oldResearchBonus; break;
					case cResearch::kHitpointsResearch: BuildingData[i].hitpointsMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kScanResearch: BuildingData[i].scan += newResearchBonus - oldResearchBonus; break;
					case cResearch::kCostResearch: BuildingData[i].buildCosts += newResearchBonus - oldResearchBonus; break;
				}
				if (researchArea != cResearch::kCostResearch)   // don't increment the version, if the only change are the costs
					incrementVersion = true;
				if (!Contains (resultUpgradedUnitDatas, & (BuildingData[i])))
					resultUpgradedUnitDatas.push_back (& (BuildingData[i]));
			}
		}
		if (incrementVersion)
			BuildingData[i].setVersion(BuildingData[i].getVersion() + 1);
	}
}

//------------------------------------------------------------------------------
void cPlayer::refreshResearchCentersWorkingOnArea()
{
	int newResearchCount = 0;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		researchCentersWorkingOnArea[i] = 0;

	for (const cBuilding* curBuilding = BuildingList; curBuilding; curBuilding = curBuilding->next)
	{
		if (curBuilding->data.canResearch && curBuilding->isUnitWorking ())
		{
			researchCentersWorkingOnArea[curBuilding->researchArea] += 1;
			newResearchCount++;
		}
	}
	workingResearchCenterCount = newResearchCount;
}

//------------------------------------------------------------------------------
bool cPlayer::mayHaveOffensiveUnit() const
{
	for (const cVehicle* vehicle = VehicleList; vehicle; vehicle = vehicle->next)
	{
		if (vehicle->data.canAttack || !vehicle->data.canBuild.empty()) return true;
	}
	for (const cBuilding* building = BuildingList; building; building = building->next)
	{
		if (building->data.canAttack || !building->data.canBuild.empty()) return true;
	}
	return false;
}

//------------------------------------------------------------------------------
bool cPlayer::canSeeAt (const cPosition& position) const
{
	if (position.x () < 0 || position.x () >= mapSize.x () || position.y () < 0 || position.y () >= mapSize.y ()) return false;

	return ScanMap[getOffset (position)] != 0;
}

//------------------------------------------------------------------------------
void cPlayer::deleteLock (cUnit& unit)
{
	std::vector<cUnit*>::iterator it = std::find (LockList.begin(), LockList.end(), &unit);
	if (it != LockList.end()) LockList.erase (it);
	unit.lockerPlayer = NULL;
}

//------------------------------------------------------------------------------
/** Toggles the lock state of a unit under the mouse
 * (when locked it's range and scan is displayed, although the unit is not selected).
*/
//------------------------------------------------------------------------------
void cPlayer::toggleLock (cMapField& OverUnitField)
{
	cUnit* unit = NULL;
	if (OverUnitField.getBaseBuilding() && OverUnitField.getBaseBuilding()->owner != this)
	{
		unit = OverUnitField.getBaseBuilding();
	}
	else if (OverUnitField.getTopBuilding() && OverUnitField.getTopBuilding()->owner != this)
	{
		unit = OverUnitField.getTopBuilding();
	}
	if (OverUnitField.getVehicle() && OverUnitField.getVehicle()->owner != this)
	{
		unit = OverUnitField.getVehicle();
	}
	if (OverUnitField.getPlane() && OverUnitField.getPlane()->owner != this)
	{
		unit = OverUnitField.getPlane();
	}
	if (unit == NULL) return;

	std::vector<cUnit*>::iterator it = std::find (LockList.begin(), LockList.end(), unit);
	if (it == LockList.end())
	{
		unit->lockerPlayer = this;
		LockList.push_back (unit);
	}
	else
	{
		unit->lockerPlayer = NULL;
		LockList.erase (it);
	}
}

//------------------------------------------------------------------------------
void cPlayer::drawSpecialCircle (const cPosition& position, int iRadius, std::vector<char>& map, const cPosition& mapsize)
{
	const float PI_ON_180 = 0.017453f;
	const float PI_ON_4 = PI_ON_180 * 45;
	if (iRadius <= 0) return;

	iRadius *= 10;
	const float step = (PI_ON_180 * 90 - acosf (1.0f / iRadius)) / 2;

	for (float angle = 0; angle <= PI_ON_4; angle += step)
	{
		int rx = (int) (cosf (angle) * iRadius);
		int ry = (int) (sinf (angle) * iRadius);
		rx /= 10;
		ry /= 10;

		int x1 = rx + position.x();
		int x2 = -rx + position.x();
		for (int k = x2; k <= x1; k++)
		{
			if (k < 0) continue;
			if (k >= mapsize.x()) break;
			if (position.y () + ry >= 0 && position.y () + ry < mapsize.y ())
				map[k + (position.y () + ry) * mapsize.x ()] |= 1;
			if (position.y () - ry >= 0 && position.y () - ry < mapsize.y ())
				map[k + (position.y () - ry) * mapsize.x ()] |= 1;
		}

		x1 = ry + position.x();
		x2 = -ry + position.x();
		for (int k = x2; k <= x1; k++)
		{
			if (k < 0) continue;
			if (k >= mapsize.x()) break;
			if (position.y () + rx >= 0 && position.y () + rx < mapsize.y ())
				map[k + (position.y () + rx) *mapsize.x ()] |= 1;
			if (position.y () - rx >= 0 && position.y () - rx < mapsize.y ())
				map[k + (position.y () - rx) *mapsize.x ()] |= 1;
		}
	}
}

//------------------------------------------------------------------------------
void cPlayer::drawSpecialCircleBig (const cPosition& position, int iRadius, std::vector<char>& map, const cPosition& mapsize)
{
	const float PI_ON_180 = 0.017453f;
	const float PI_ON_4 = PI_ON_180 * 45;
	if (iRadius <= 0) return;

	--iRadius;
	iRadius *= 10;
	const float step = (PI_ON_180 * 90 - acosf (1.0f / iRadius)) / 2;
	for (float angle = 0; angle <= PI_ON_4; angle += step)
	{
		int rx = (int) (cosf (angle) * iRadius);
		int ry = (int) (sinf (angle) * iRadius);
		rx /= 10;
		ry /= 10;

		int x1 = rx + position.x();
		int x2 = -rx + position.x();
		for (int k = x2; k <= x1 + 1; k++)
		{
			if (k < 0) continue;
			if (k >= mapsize.x ()) break;
			if (position.y () + ry >= 0 && position.y () + ry < mapsize.y ())
				map[k + (position.y () + ry) *mapsize.x ()] |= 1;
			if (position.y () - ry >= 0 && position.y () - ry < mapsize.y ())
				map[k + (position.y () - ry) *mapsize.x ()] |= 1;

			if (position.y () + ry + 1 >= 0 && position.y () + ry + 1 < mapsize.y ())
				map[k + (position.y () + ry + 1) *mapsize.x ()] |= 1;
			if (position.y () - ry + 1 >= 0 && position.y () - ry + 1 < mapsize.y ())
				map[k + (position.y () - ry + 1) *mapsize.x ()] |= 1;
		}

		x1 = ry + position.x();
		x2 = -ry + position.x();
		for (int k = x2; k <= x1 + 1; k++)
		{
			if (k < 0) continue;
			if (k >= mapsize.x ()) break;
			if (position.y () + rx >= 0 && position.y () + rx < mapsize.y ())
				map[k + (position.y () + rx) *mapsize.x ()] |= 1;
			if (position.y () - rx >= 0 && position.y () - rx < mapsize.y ())
				map[k + (position.y () - rx) *mapsize.x ()] |= 1;

			if (position.y () + rx + 1 >= 0 && position.y () + rx + 1 < mapsize.y ())
				map[k + (position.y () + rx + 1) *mapsize.x ()] |= 1;
			if (position.y () - rx + 1 >= 0 && position.y () - rx + 1 < mapsize.y ())
				map[k + (position.y () - rx + 1) *mapsize.x ()] |= 1;
		}
	}
}

//------------------------------------------------------------------------------
void cPlayer::addSavedReport (std::unique_ptr<cSavedReport> savedReport)
{
	if (savedReport == nullptr) return;

	savedReportsList.push_back (std::move (savedReport));

	reportAdded (*savedReportsList.back ());
}
