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

#ifndef ui_graphical_menu_control_local_hotseat_localhotseatgamenewH
#define ui_graphical_menu_control_local_hotseat_localhotseatgamenewH

#include "game/data/player/playerbasicdata.h"
#include "game/startup/initplayerdata.h"
#include "ui/graphical/menu/control/local/hotseat/localhotseatgame.h"
#include "utility/position.h"
#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"

#include <memory>
#include <utility>
#include <vector>

class cApplication;
class cStaticMap;
class cGameSettings;
class cPlayerColor;
class cPosition;
class cUnitUpgrade;

struct sLandingUnit;
struct sID;

class cLocalHotSeatGameNew : public cLocalHotSeatGame
{
public:
	cLocalHotSeatGameNew() = default;

	void start (cApplication& application);

	void setUnitsData (std::shared_ptr<const cUnitsData> unitsData_) { unitsData = std::move (unitsData_); }
	void setClanData (std::shared_ptr<const cClanData> clanData_) { clanData = std::move (clanData_); }
	void setGameSettings (std::shared_ptr<cGameSettings> gameSettings);
	void setStaticMap (std::shared_ptr<cStaticMap> staticMap);
	void setPlayers (const std::vector<cPlayerBasicData>& players);
	void setPlayerClan (size_t playerIndex, int clan);
	void setLandingUnits (size_t playerIndex, std::vector<sLandingUnit> landingUnits);
	void setUnitUpgrades (size_t playerIndex, std::vector<std::pair<sID, cUnitUpgrade>> unitUpgrades);
	void setLandingPosition (size_t playerIndex, const cPosition& landingPosition);

	std::shared_ptr<const cUnitsData> getUnitsData() const { return unitsData; }
	std::shared_ptr<const cClanData> getClanData() const { return clanData; }
	const std::shared_ptr<cStaticMap>& getStaticMap();
	const std::shared_ptr<cGameSettings>& getGameSettings();
	const std::vector<sLandingUnit>& getLandingUnits (size_t playerIndex);

	size_t getPlayerCount() const;
	const cPlayerBasicData& getPlayer (size_t playerIndex) const;
	int getPlayerClan (size_t playerIndex) const;

private:
	cSignalConnectionManager signalConnectionManager;

	std::shared_ptr<const cUnitsData> unitsData;
	std::shared_ptr<const cClanData> clanData;
	std::shared_ptr<cStaticMap> staticMap;
	std::shared_ptr<cGameSettings> gameSettings;

	std::vector<cPlayerBasicData> playersBasicData;
	std::vector<sInitPlayerData> playersData;
};

#endif
