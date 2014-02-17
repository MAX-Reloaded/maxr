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

#include <functional>

#include "windowmain.h"
#include "../../../pcx.h"
#include "../../../main.h"
#include "../../../buildings.h"
#include "../../../vehicles.h"
#include "../widgets/label.h"
#include "../widgets/image.h"

//------------------------------------------------------------------------------
cWindowMain::cWindowMain (const std::string& title) :
	cWindow (LoadPCX (GFXOD_MAIN))
{
	const auto& menuPosition = getArea ().getMinCorner ();

	addChild (std::make_unique<cLabel> (cBox<cPosition> (menuPosition + cPosition (0, 147), menuPosition + cPosition (getArea ().getMaxCorner ().x (), 157)), title, FONT_LATIN_NORMAL, eAlignmentType::Center));

	addChild (std::make_unique<cLabel> (cBox<cPosition> (menuPosition + cPosition (0, 465), menuPosition + cPosition (getArea ().getMaxCorner (). x(), 475)), lngPack.i18n ("Text~Main~Credits_Reloaded") + " " + PACKAGE_VERSION, FONT_LATIN_NORMAL, eAlignmentType::Center));

	infoImage = addChild (std::make_unique<cImage> (menuPosition + cPosition (16, 182), getRandomInfoImage (), SoundData.SNDHudButton));
	signalConnectionManager.connect (infoImage->clicked, std::bind (&cWindowMain::infoImageClicked, this));
}

//------------------------------------------------------------------------------
cWindowMain::~cWindowMain ()
{}

//------------------------------------------------------------------------------
void cWindowMain::infoImageClicked ()
{
	infoImage->setImage (getRandomInfoImage());
}

//------------------------------------------------------------------------------
SDL_Surface* cWindowMain::getRandomInfoImage ()
{
	int const showBuilding = random (3);
	// I want 3 possible random numbers since a chance of 50:50 is boring
	// (and vehicles are way more cool so I prefer them to be shown) -- beko
	static int lastUnitShow = -1;
	int unitShow = -1;
	SDL_Surface* surface = NULL;

	if (showBuilding == 1 && UnitsData.getNrBuildings () > 0)
	{
		// that's a 33% chance that we show a building on 1
		do
		{
			unitShow = random (UnitsData.getNrBuildings () - 1);
			// make sure we don't show same unit twice
		}
		while (unitShow == lastUnitShow && UnitsData.getNrBuildings () > 1);
		surface = UnitsData.buildingUIs[unitShow].info;
	}
	else if (UnitsData.getNrVehicles () > 0)
	{
		// and a 66% chance to show a vehicle on 0 or 2
		do
		{
			unitShow = random (UnitsData.getNrVehicles () - 1);
			// make sure we don't show same unit twice
		}
		while (unitShow == lastUnitShow && UnitsData.getNrVehicles () > 1);
		surface = UnitsData.vehicleUIs[unitShow].info;
	}
	else surface = NULL;
	lastUnitShow = unitShow; //store shown unit
	return surface;
}