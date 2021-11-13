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

#include "unitdata.h"

#include "game/data/player/clans.h"
#include "utility/crc.h"
#include "utility/log.h"

#include <3rd/tinyxml2/tinyxml2.h>

#include <algorithm>

cUnitsData UnitsDataGlobal;

namespace serialization
{
	//--------------------------------------------------------------------------
	/*static*/ std::string sEnumSerializer<eMuzzleType>::toString (eMuzzleType e)
	{
		switch (e)
		{
			case eMuzzleType::None: return "None";
			case eMuzzleType::Big: return "Big";
			case eMuzzleType::Rocket: return "Rocket";
			case eMuzzleType::Small: return "Small";
			case eMuzzleType::Med: return "Med";
			case eMuzzleType::MedLong: return "MedLong";
			case eMuzzleType::RocketCluster: return "RocketCluster";
			case eMuzzleType::Torpedo: return "Torpedo";
			case eMuzzleType::Sniper: return "Sniper";
		}
		Log.write ("Unknown eMuzzleType " + std::to_string (static_cast<int> (e)), cLog::eLOG_TYPE_WARNING);
		return std::to_string (static_cast<int> (e));
	}
	//--------------------------------------------------------------------------
	/*static*/ eMuzzleType sEnumSerializer<eMuzzleType>::fromString (const std::string& s)
	{
		if (s == "None") return eMuzzleType::None;
		if (s == "Big") return eMuzzleType::Big;
		if (s == "Rocket") return eMuzzleType::Rocket;
		if (s == "Small") return eMuzzleType::Small;
		if (s == "Med") return eMuzzleType::Med;
		if (s == "MedLong") return eMuzzleType::MedLong;
		if (s == "RocketCluster") return eMuzzleType::RocketCluster;
		if (s == "Torpedo") return eMuzzleType::Torpedo;
		if (s == "Sniper") return eMuzzleType::Sniper;

		Log.write ("Unknown eMuzzleType " + s, cLog::eLOG_TYPE_WARNING);
		throw std::runtime_error ("Unknown eMuzzleType " + s);
	}

	//--------------------------------------------------------------------------
	/*static*/ std::string sEnumSerializer<eSurfacePosition>::toString (eSurfacePosition e)
	{
		switch (e)
		{
			case eSurfacePosition::BeneathSea: return "BeneathSea";
			case eSurfacePosition::AboveSea: return "AboveSea";
			case eSurfacePosition::Base: return "Base";
			case eSurfacePosition::AboveBase: return "AboveBase";
			case eSurfacePosition::Ground: return "Ground";
			case eSurfacePosition::Above: return "Above";
		}
		Log.write ("Unknown eSurfacePosition " + std::to_string (static_cast<int> (e)), cLog::eLOG_TYPE_WARNING);
		return std::to_string (static_cast<int> (e));
	}
	//--------------------------------------------------------------------------
	/*static*/ eSurfacePosition sEnumSerializer<eSurfacePosition>::fromString (const std::string& s)
	{
		if (s == "BeneathSea") return eSurfacePosition::BeneathSea;
		if (s == "AboveSea") return eSurfacePosition::AboveSea;
		if (s == "Base") return eSurfacePosition::Base;
		if (s == "AboveBase") return eSurfacePosition::AboveBase;
		if (s == "Ground") return eSurfacePosition::Ground;
		if (s == "Above") return eSurfacePosition::Above;

		Log.write ("Unknown eSurfacePosition " + s, cLog::eLOG_TYPE_WARNING);
		throw std::runtime_error ("Unknown eSurfacePosition " + s);
	}

	//--------------------------------------------------------------------------
	/*static*/ std::string sEnumSerializer<eOverbuildType>::toString (eOverbuildType e)
	{
		switch (e)
		{
			case eOverbuildType::No: return "No";
			case eOverbuildType::Yes: return "Yes";
			case eOverbuildType::YesNRemove: return "YesNRemove";
		}
		Log.write ("Unknown eOverbuildType " + std::to_string (static_cast<int> (e)), cLog::eLOG_TYPE_WARNING);
		return std::to_string (static_cast<int> (e));
	}
	//--------------------------------------------------------------------------
	/*static*/ eOverbuildType sEnumSerializer<eOverbuildType>::fromString (const std::string& s)
	{
		if (s == "No") return eOverbuildType::No;
		if (s == "Yes") return eOverbuildType::Yes;
		if (s == "YesNRemove") return eOverbuildType::YesNRemove;

		Log.write ("Unknown eOverbuildType " + s, cLog::eLOG_TYPE_WARNING);
		throw std::runtime_error ("Unknown eOverbuildType " + s);
	}

	//--------------------------------------------------------------------------
	/*static*/ std::string sEnumSerializer<eStorageUnitsImageType>::toString (eStorageUnitsImageType e)
	{
		switch (e)
		{
			case eStorageUnitsImageType::None: return "None";
			case eStorageUnitsImageType::Tank: return "Tank";
			case eStorageUnitsImageType::Plane: return "Plane";
			case eStorageUnitsImageType::Ship: return "Ship";
			case eStorageUnitsImageType::Human: return "Human";
		}
		Log.write ("Unknown eStorageUnitsImageType " + std::to_string (static_cast<int> (e)), cLog::eLOG_TYPE_WARNING);
		return std::to_string (static_cast<int> (e));
	}
	//--------------------------------------------------------------------------
	/*static*/ eStorageUnitsImageType sEnumSerializer<eStorageUnitsImageType>::fromString (const std::string& s)
	{
		if (s == "None") return eStorageUnitsImageType::None;
		if (s == "Tank") return eStorageUnitsImageType::Tank;
		if (s == "Plane") return eStorageUnitsImageType::Plane;
		if (s == "Ship") return eStorageUnitsImageType::Ship;
		if (s == "Human") return eStorageUnitsImageType::Human;

		Log.write ("Unknown eStorageUnitsImageType " + s, cLog::eLOG_TYPE_WARNING);
		throw std::runtime_error ("Unknown eStorageUnitsImageType " + s);
	}
}

//------------------------------------------------------------------------------
uint32_t sStaticBuildingData::computeChecksum (uint32_t crc) const
{
	crc = calcCheckSum (canBeLandedOn, crc);
	crc = calcCheckSum (canMineMaxRes, crc);
	crc = calcCheckSum (canBeOverbuild, crc);
	crc = calcCheckSum (canResearch, crc);
	crc = calcCheckSum (canScore, crc);
	crc = calcCheckSum (canSelfDestroy, crc);
	crc = calcCheckSum (canWork, crc);
	crc = calcCheckSum (connectsToBase, crc);
	crc = calcCheckSum (convertsGold, crc);
	crc = calcCheckSum (explodesOnContact, crc);
	crc = calcCheckSum (isBig, crc);
	crc = calcCheckSum (maxBuildFactor, crc);
	crc = calcCheckSum (modifiesSpeed, crc);

	return crc;
}

//------------------------------------------------------------------------------
uint32_t sStaticVehicleData::computeChecksum (uint32_t crc) const
{
	crc = calcCheckSum (canBuildPath, crc);
	crc = calcCheckSum (canCapture, crc);
	crc = calcCheckSum (canClearArea, crc);
	crc = calcCheckSum (canDriveAndFire, crc);
	crc = calcCheckSum (canDisable, crc);
	crc = calcCheckSum (canPlaceMines, crc);
	crc = calcCheckSum (canSurvey, crc);
	crc = calcCheckSum (isHuman, crc);
	crc = calcCheckSum (isStorageType, crc);

	return crc;
}

//------------------------------------------------------------------------------
uint32_t sSpecialBuildingsId::computeChecksum (uint32_t crc) const
{
	crc = calcCheckSum (alienFactory, crc);
	crc = calcCheckSum (connector, crc);
	crc = calcCheckSum (landMine, crc);
	crc = calcCheckSum (mine, crc);
	crc = calcCheckSum (seaMine, crc);
	crc = calcCheckSum (smallBeton, crc);
	crc = calcCheckSum (smallGenerator, crc);
	return crc;
}

//------------------------------------------------------------------------------
void sSpecialBuildingsId::logMissing() const
{
	if (alienFactory == 0) Log.write ("special \"alienFactory\" missing", cLog::eLOG_TYPE_ERROR);
	if (connector == 0) Log.write ("special \"connector\" missing", cLog::eLOG_TYPE_ERROR);
	if (landMine == 0) Log.write ("special \"landmine\" missing", cLog::eLOG_TYPE_ERROR);
	if (mine == 0) Log.write ("special \"mine\" missing", cLog::eLOG_TYPE_ERROR);
	if (seaMine == 0) Log.write ("special \"seamine\" missing", cLog::eLOG_TYPE_ERROR);
	if (smallBeton == 0) Log.write ("special \"smallBeton\" missing", cLog::eLOG_TYPE_ERROR);
	if (smallGenerator == 0) Log.write ("special \"energy\" missing", cLog::eLOG_TYPE_ERROR);
}

//------------------------------------------------------------------------------
uint32_t sSpecialVehiclesId::computeChecksum (uint32_t crc) const
{
	crc = calcCheckSum (constructor, crc);
	crc = calcCheckSum (engineer, crc);
	crc = calcCheckSum (surveyor, crc);
	return crc;
}

//------------------------------------------------------------------------------
void sSpecialVehiclesId::logMissing() const
{
	if (constructor == 0) Log.write ("Constructor index not found. Constructor needs to have the property \"Can_Build = BigBuilding\"", cLog::eLOG_TYPE_ERROR);
	if (engineer == 0) Log.write ("Engineer index not found. Engineer needs to have the property \"Can_Build = SmallBuilding\"", cLog::eLOG_TYPE_ERROR);
	if (surveyor == 0) Log.write ("Surveyor index not found. Surveyor needs to have the property \"Can_Survey = Yes\"", cLog::eLOG_TYPE_ERROR);
}

//------------------------------------------------------------------------------
cUnitsData::cUnitsData()
{
	rubbleBig.buildingData.isBig = true;
	rubbleSmall.buildingData.isBig = false;
}

//------------------------------------------------------------------------------
int cUnitsData::getUnitIndexBy (sID id) const
{
	for (unsigned int i = 0; i != staticUnitData.size(); ++i)
	{
		if (staticUnitData[i].ID == id) return i;
	}
	Log.write ("Unitdata with id (" + std::to_string (id.firstPart) + ", " + std::to_string (id.secondPart) + ") not found", cLog::eLOG_TYPE_ERROR);
	return -1;
}

//------------------------------------------------------------------------------
void cUnitsData::initializeIDData()
{
	for (const auto& data : staticUnitData)
	{
		if (data.canBuild == "BigBuilding")
			specialVehicles.constructor = data.ID.secondPart;
		if (data.canBuild == "SmallBuilding")
			specialVehicles.engineer = data.ID.secondPart;
		if (data.vehicleData.canSurvey)
			specialVehicles.surveyor = data.ID.secondPart;
	}
	specialVehicles.logMissing();

	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
void cUnitsData::initializeClanUnitData (const cClanData& clanData)
{
	crcCache = std::nullopt;

	clanDynamicUnitData.reserve (clanData.getClans().size());

	for (const cClan& clan : clanData.getClans())
	{
		clanDynamicUnitData.emplace_back();
		std::vector<cDynamicUnitData>& clanListVehicles = clanDynamicUnitData.back();

		// make a copy of the vehicle's stats
		clanListVehicles = dynamicUnitData;
		for (size_t j = 0; j != dynamicUnitData.size(); ++j)
		{
			cDynamicUnitData& clanVehicle = clanListVehicles[j];
			const cClanUnitStat* changedStat = clan.getUnitStat (clanVehicle.getId());
			if (changedStat == nullptr) continue;

			if (changedStat->hasModification (EClanModification::Damage))
				clanVehicle.setDamage (changedStat->getModificationValue (EClanModification::Damage));
			if (changedStat->hasModification (EClanModification::Range))
				clanVehicle.setRange (changedStat->getModificationValue (EClanModification::Range));
			if (changedStat->hasModification (EClanModification::Armor))
				clanVehicle.setArmor (changedStat->getModificationValue (EClanModification::Armor));
			if (changedStat->hasModification (EClanModification::Hitpoints))
				clanVehicle.setHitpointsMax (changedStat->getModificationValue (EClanModification::Hitpoints));
			if (changedStat->hasModification (EClanModification::Scan))
				clanVehicle.setScan (changedStat->getModificationValue (EClanModification::Scan));
			if (changedStat->hasModification (EClanModification::Speed))
				clanVehicle.setSpeedMax (changedStat->getModificationValue (EClanModification::Speed) * 4);
			if (changedStat->hasModification (EClanModification::Built_Costs))
				clanVehicle.setBuildCost (changedStat->getModificationValue (EClanModification::Built_Costs));
		}
	}
}

//------------------------------------------------------------------------------
bool cUnitsData::isValidId (const sID& id) const
{
	return getUnitIndexBy (id) != -1;
}

//------------------------------------------------------------------------------
size_t cUnitsData::getNrOfClans() const
{
	return clanDynamicUnitData.size();
}

//------------------------------------------------------------------------------
const cDynamicUnitData& cUnitsData::getDynamicUnitData (const sID& id, int clan /*= -1*/) const
{
	if (clan < 0 || static_cast<unsigned> (clan) >= clanDynamicUnitData.size())
	{
		for (const auto& data : dynamicUnitData)
		{
			if (data.getId() == id) return data;
		}
		throw std::runtime_error ("Unitdata not found" + id.getText());
	}
	else
	{
		for (const auto& data : clanDynamicUnitData[clan])
		{
			if (data.getId() == id) return data;
		}
		throw std::runtime_error ("Unitdata not found" + id.getText());
	}
}

//------------------------------------------------------------------------------
const cStaticUnitData& cUnitsData::getStaticUnitData (const sID& id) const
{
	for (const auto& data : staticUnitData)
	{
		if (data.ID == id) return data;
	}
	throw std::runtime_error ("Unitdata not found" + id.getText());
}

//------------------------------------------------------------------------------
const std::vector<cDynamicUnitData>& cUnitsData::getDynamicUnitsData (int clan /*= -1*/) const
{
	if (clan < 0 || static_cast<unsigned> (clan) >= clanDynamicUnitData.size())
	{
		return dynamicUnitData;
	}
	else
	{
		return clanDynamicUnitData[clan];
	}
}

//------------------------------------------------------------------------------
const std::vector<cStaticUnitData>& cUnitsData::getStaticUnitsData() const
{
	return staticUnitData;
}

//------------------------------------------------------------------------------
uint32_t cUnitsData::getChecksum (uint32_t crc) const
{
	if (!crcCache)
	{
		crcCache = 0;
		*crcCache = specialBuildings.computeChecksum (*crcCache);
		*crcCache = specialVehicles.computeChecksum (*crcCache);
		*crcCache = calcCheckSum (staticUnitData, *crcCache);
		*crcCache = calcCheckSum (dynamicUnitData, *crcCache);
		*crcCache = calcCheckSum (clanDynamicUnitData, *crcCache);
	}

	return calcCheckSum (*crcCache, crc);
}

//------------------------------------------------------------------------------
const std::string& cStaticUnitData::getDefaultName() const
{
	return name;
}

//------------------------------------------------------------------------------
const std::string& cStaticUnitData::getDefaultDescription() const
{
	return description;
}

//------------------------------------------------------------------------------
uint32_t cStaticUnitData::getChecksum (uint32_t crc) const
{
	crc = calcCheckSum (ID, crc);
	crc = calcCheckSum (buildAs, crc);
	crc = calcCheckSum (canAttack, crc);
	crc = calcCheckSum (canBeCaptured, crc);
	crc = calcCheckSum (canBeDisabled, crc);
	crc = calcCheckSum (canBuild, crc);
	crc = calcCheckSum (canDetectStealthOn, crc);
	crc = calcCheckSum (canRearm, crc);
	crc = calcCheckSum (canRepair, crc);
	crc = calcCheckSum (description, crc);
	crc = calcCheckSum (doesSelfRepair, crc);
	crc = calcCheckSum (factorGround, crc);
	crc = calcCheckSum (factorSea, crc);
	crc = calcCheckSum (factorAir, crc);
	crc = calcCheckSum (factorCoast, crc);
	crc = calcCheckSum (isStealthOn, crc);
	crc = calcCheckSum (muzzleType, crc);
	crc = calcCheckSum (name, crc);
	crc = calcCheckSum (needsMetal, crc);
	crc = calcCheckSum (needsEnergy, crc);
	crc = calcCheckSum (needsHumans, crc);
	crc = calcCheckSum (needsOil, crc);
	crc = calcCheckSum (produceEnergy, crc);
	crc = calcCheckSum (produceHumans, crc);
	crc = calcCheckSum (storageResMax, crc);
	crc = calcCheckSum (storageUnitsMax, crc);
	crc = calcCheckSum (storeResType, crc);
	crc = calcCheckSum (storeUnitsImageType, crc);
	crc = calcCheckSum (storeUnitsTypes, crc);
	crc = calcCheckSum (surfacePosition, crc);

	if (ID.isABuilding()) crc = buildingData.computeChecksum (crc);
	else crc = vehicleData.computeChecksum (crc);

	return crc;
}

//------------------------------------------------------------------------------
cDynamicUnitData::cDynamicUnitData (const cDynamicUnitData& other) :
	id (other.id),
	buildCosts (other.buildCosts),
	version (other.version),
	speedCur (other.speedCur),
	speedMax (other.speedMax),
	hitpointsCur (other.hitpointsCur),
	hitpointsMax (other.hitpointsMax),
	shotsCur (other.shotsCur),
	shotsMax (other.shotsMax),
	ammoCur (other.ammoCur),
	ammoMax (other.ammoMax),
	range (other.range),
	scan (other.scan),
	damage (other.damage),
	armor (other.armor),
	crcCache (std::nullopt)
{}

//------------------------------------------------------------------------------
cDynamicUnitData& cDynamicUnitData::operator= (const cDynamicUnitData& other)
{
	id = other.id;
	buildCosts = other.buildCosts;
	version = other.version;
	speedCur = other.speedCur;
	speedMax = other.speedMax;
	hitpointsCur = other.hitpointsCur;
	hitpointsMax = other.hitpointsMax;
	shotsCur = other.shotsCur;
	shotsMax = other.shotsMax;
	ammoCur = other.ammoCur;
	ammoMax = other.ammoMax;
	range = other.range;
	scan = other.scan;
	damage = other.damage;
	armor = other.armor;
	crcCache = std::nullopt;

	return *this;
}

//------------------------------------------------------------------------------
sID cDynamicUnitData::getId() const
{
	return id;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setId (const sID& value)
{
	id = value;
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getBuildCost() const
{
	return buildCosts;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setBuildCost (int value)
{
	std::swap (buildCosts, value);
	if (buildCosts != value) buildCostsChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getVersion() const
{
	return version;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setVersion (int value)
{
	std::swap (version, value);
	if (version != value) versionChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getSpeed() const
{
	return speedCur;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setSpeed (int value)
{
	std::swap (speedCur, value);
	if (speedCur != value) speedChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getSpeedMax() const
{
	return speedMax;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setSpeedMax (int value)
{
	std::swap (speedMax, value);
	if (speedMax != value) speedMaxChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getHitpoints() const
{
	return hitpointsCur;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setHitpoints (int value)
{
	std::swap (hitpointsCur, value);
	if (hitpointsCur != value) hitpointsChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getHitpointsMax() const
{
	return hitpointsMax;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setHitpointsMax (int value)
{
	std::swap (hitpointsMax, value);
	if (hitpointsMax != value) hitpointsMaxChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getScan() const
{
	return scan;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setScan (int value)
{
	std::swap (scan, value);
	if (scan != value) scanChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getRange() const
{
	return range;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setRange (int value)
{
	std::swap (range, value);
	if (range != value) rangeChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getShots() const
{
	return shotsCur;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setShots (int value)
{
	std::swap (shotsCur, value);
	if (shotsCur != value) shotsChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getShotsMax() const
{
	return shotsMax;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setShotsMax (int value)
{
	std::swap (shotsMax, value);
	if (shotsMax != value) shotsMaxChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getAmmo() const
{
	return ammoCur;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setAmmo (int value)
{
	std::swap (ammoCur, value);
	if (ammoCur != value) ammoChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getAmmoMax() const
{
	return ammoMax;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setAmmoMax (int value)
{
	std::swap (ammoMax, value);
	if (ammoMax != value) ammoMaxChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getDamage() const
{
	return damage;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setDamage (int value)
{
	std::swap (damage, value);
	if (damage != value) damageChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
int cDynamicUnitData::getArmor() const
{
	return armor;
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setArmor (int value)
{
	std::swap (armor, value);
	if (armor != value) armorChanged();
	crcCache = std::nullopt;
}

//------------------------------------------------------------------------------
uint32_t cDynamicUnitData::getChecksum (uint32_t crc) const
{
	if (!crcCache)
	{
		crcCache = 0;
		*crcCache = calcCheckSum (id, *crcCache);
		*crcCache = calcCheckSum (buildCosts, *crcCache);
		*crcCache = calcCheckSum (version, *crcCache);
		*crcCache = calcCheckSum (speedCur, *crcCache);
		*crcCache = calcCheckSum (speedMax, *crcCache);
		*crcCache = calcCheckSum (hitpointsCur, *crcCache);
		*crcCache = calcCheckSum (hitpointsMax, *crcCache);
		*crcCache = calcCheckSum (shotsCur, *crcCache);
		*crcCache = calcCheckSum (shotsMax, *crcCache);
		*crcCache = calcCheckSum (ammoCur, *crcCache);
		*crcCache = calcCheckSum (ammoMax, *crcCache);
		*crcCache = calcCheckSum (range, *crcCache);
		*crcCache = calcCheckSum (scan, *crcCache);
		*crcCache = calcCheckSum (damage, *crcCache);
		*crcCache = calcCheckSum (armor, *crcCache);
	}

	return calcCheckSum (*crcCache, crc);
}

//------------------------------------------------------------------------------
void cDynamicUnitData::setMaximumCurrentValues()
{
	speedCur     = speedMax;
	ammoCur      = ammoMax;
	shotsCur     = shotsMax;
	hitpointsCur = hitpointsMax;

	crcCache = std::nullopt;
}
