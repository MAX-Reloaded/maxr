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

#ifndef ui_graphical_game_control_gameguicontrollerH
#define ui_graphical_game_control_gameguicontrollerH

#include <memory>
#include <string>
#include <vector>
#include <array>

#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"
#include "utility/position.h"

#include "ui/graphical/game/gameguistate.h"

#include "game/logic/upgradecalculator.h"

class cGameGui;
class cClient;
class cApplication;
class cSoundManager;
class cAnimationTimer;

class cPlayer;
class cUnit;
class cVehicle;
class cBuilding;
class cStaticMap;
class cMap;
class cGameSettings;
class cUnitUpgrade;
class cCasualtiesTracker;
class cTurnTimeClock;
class cTurnClock;
class cSavedReport;
class cBuildListItem;
class cWindowUpgradesFilterState;

class cPosition;

struct sID;

class cGameGuiController
{
public:
	cGameGuiController (cApplication& application, std::shared_ptr<const cStaticMap> staticMap);
	~cGameGuiController();

	void start();

	void addPlayerGameGuiState (const cPlayer& player, cGameGuiState playerGameGuiState);

	void setSingleClient (std::shared_ptr<cClient> clients);
	void setClients (std::vector<std::shared_ptr<cClient>> clients, int activePlayerNumber);

	mutable cSignal<void (int saveNumber, const std::string& name)> triggeredSave;
	mutable cSignal<void ()> terminated;
private:
	cSignalConnectionManager signalConnectionManager;
	cSignalConnectionManager guiSignalConnectionManager;
	cSignalConnectionManager clientSignalConnectionManager;
	cSignalConnectionManager allClientsSignalConnectionManager;
	cSignalConnectionManager buildPositionSelectionConnectionManager;

	cApplication& application;

	std::shared_ptr<cSoundManager> soundManager;
	std::shared_ptr<cAnimationTimer> animationTimer;

	std::shared_ptr<cGameGui> gameGui;
	std::shared_ptr<cClient> activeClient; //TODO: rename -> client
	std::vector<std::shared_ptr<cClient>> clients; //TODO: remove

	std::map<int, cGameGuiState> playerGameGuiStates;

	std::pair<bool, cPosition> savedReportPosition;
	std::shared_ptr<cWindowUpgradesFilterState> upgradesFilterState;
	std::array<std::pair<bool, cPosition>, 4> savedPositions;

	void initShortcuts();

	void connectGuiStaticCommands();

	void setActiveClient (std::shared_ptr<cClient> client); //Todo: set active player. Oder signal active player changed

	void connectClient (cClient& client);

	void showNextPlayerDialog();

	void showFilesWindow();
	void showPreferencesDialog();
	void showReportsWindow();

	void showUnitHelpWindow (const cUnit& unit);
	void showUnitTransferDialog (const cUnit& sourceUnit, const cUnit& destinationUnit);
	void showBuildBuildingsWindow (const cVehicle& vehicle);
	void showBuildVehiclesWindow (const cBuilding& building);
	void showResourceDistributionDialog (const cUnit& unit);
	void showResearchDialog (const cUnit& unit);
	void showUpgradesWindow (const cUnit& unit);
	void showStorageWindow (const cUnit& unit);
	void showSelfDestroyDialog (const cUnit& unit);

	void handleChatCommand (const std::string& command);

	void handleReport (const cSavedReport& report);

	void selectNextUnit();
	void selectPreviousUnit();
	void markSelectedUnitAsDone();

	void centerSelectedUnit();

	void savePosition (size_t index);
	void jumpToSavedPosition (size_t index);

	std::vector<std::shared_ptr<const cPlayer>> getPlayers() const;
	std::shared_ptr<const cPlayer> getActivePlayer() const;
	std::shared_ptr<const cTurnClock> getTurnClock() const;
	std::shared_ptr<const cTurnTimeClock> getTurnTimeClock() const;
	std::shared_ptr<const cGameSettings> getGameSettings() const;
	std::shared_ptr<const cCasualtiesTracker> getCasualtiesTracker() const;
	std::shared_ptr<const cMap> getDynamicMap() const;

	mutable cSignal<void (const cUnit&, const cUnit&, int, int)> transferTriggered;
	mutable cSignal<void (const cVehicle&, const cPosition&, const sID&, int)> buildBuildingTriggered;
	mutable cSignal<void (const cVehicle&, const cPosition&, const sID&, int)> buildBuildingPathTriggered;
	mutable cSignal<void (const cBuilding&, const std::vector<cBuildListItem>&, int, bool)> buildVehiclesTriggered;
	mutable cSignal<void (const cUnit& unit, size_t index, const cPosition& position)> activateAtTriggered;
	mutable cSignal<void (const cUnit&, const cUnit&)> reloadTriggered;
	mutable cSignal<void (const cUnit&, const cUnit&)> repairTriggered;
	mutable cSignal<void (const cUnit&, size_t index)> upgradeTriggered;
	mutable cSignal<void (const cUnit&)> upgradeAllTriggered;
	mutable cSignal<void (const cBuilding&, int, int, int)> changeResourceDistributionTriggered;
	mutable cSignal<void (const std::array<int, cResearch::kNrResearchAreas>&)> changeResearchSettingsTriggered;
	mutable cSignal<void (const std::vector<std::pair<sID, cUnitUpgrade>>&)> takeUnitUpgradesTriggered;
	mutable cSignal<void (const cUnit&)> selfDestructionTriggered;
	mutable cSignal<void (const cUnit&)> resumeMoveJobTriggered;
	mutable cSignal<void ()> resumeAllMoveJobsTriggered;
};

#endif // ui_graphical_game_control_gameguicontrollerH
