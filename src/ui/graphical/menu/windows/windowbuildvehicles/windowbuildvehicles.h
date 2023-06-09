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

#ifndef ui_graphical_menu_windows_windowbuildvehicles_windowbuildvehiclesH
#define ui_graphical_menu_windows_windowbuildvehicles_windowbuildvehiclesH

#include "ui/graphical/menu/windows/windowadvancedhangar/windowadvancedhangar.h"

#include <vector>

class cBuildListItem;
class cBuildSpeedHandlerWidget;
class cBuilding;
class cCheckBox;
class cLabel;
class cMapView;
class cTurnTimeClock;
class cUnitListViewItemBuild;
class cUnitsData;

class cWindowBuildVehicles : public cWindowAdvancedHangar<cUnitListViewItemBuild>
{
public:
	cWindowBuildVehicles (const cBuilding& building, const cMapView& map, std::shared_ptr<const cUnitsData> unitsData, std::shared_ptr<const cTurnTimeClock> turnTimeClock);

	void retranslate() override;

	std::vector<sID> getBuildList() const;
	int getSelectedBuildSpeed() const;
	bool isRepeatActive() const;

protected:
	void setActiveUnit (const sID& unitId) override;

private:
	cSignalConnectionManager signalConnectionManager;

	const cBuilding& building;

	cBuildSpeedHandlerWidget* speedHandler = nullptr;

	cLabel* titleLabel = nullptr;
	cCheckBox* repeatCheckBox = nullptr;

	void generateSelectionList (const cBuilding& building, const cMapView& map, const cUnitsData& unitsData);
	void generateBuildList (const cBuilding& building);

	void closeOnUnitDestruction();
};

#endif // ui_graphical_menu_windows_windowbuildvehicles_windowbuildvehiclesH
