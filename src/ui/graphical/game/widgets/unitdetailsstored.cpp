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

#include "ui/graphical/game/widgets/unitdetailsstored.h"

#include "ui/graphical/game/widgets/unitdetailshud.h"
#include "ui/graphical/menu/widgets/label.h"
#include "main.h"
#include "video.h"
#include "game/data/units/unit.h"
#include "game/data/player/player.h"
#include "game/data/units/building.h"

//------------------------------------------------------------------------------
cUnitDetailsStored::cUnitDetailsStored (const cBox<cPosition>& area) :
	cWidget (area),
	unit (nullptr)
{
	const auto size = getSize ();
	if (std::size_t(size.y ()) < maxRows*rowHeight) resize (cPosition (getSize ().x (), maxRows*rowHeight));

	for (size_t i = 0; i < maxRows; ++i)
	{
		amountLabels[i] = addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition () + cPosition (3, 2 + rowHeight * i), getPosition () + cPosition (3 + 30, 2 + rowHeight * i + rowHeight)), "", FONT_LATIN_SMALL_WHITE, toEnumFlag (eAlignmentType::CenterHorizontal) | eAlignmentType::Bottom));
		nameLabels[i] = addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition () + cPosition (35, 2 + rowHeight * i), getPosition () + cPosition (35 + 30, 2 + rowHeight * i + rowHeight)), "", FONT_LATIN_SMALL_WHITE, toEnumFlag (eAlignmentType::Left) | eAlignmentType::Bottom));
	}

    surface = AutoSurface (SDL_CreateRGBSurface (0, size.x (), size.y (), Video.getColDepth (), 0, 0, 0, 0));

	SDL_FillRect (surface.get (), nullptr, 0xFF00FF);
	SDL_SetColorKey (surface.get (), SDL_TRUE, 0xFF00FF);
}

//------------------------------------------------------------------------------
void cUnitDetailsStored::setUnit (const cUnit* unit_)
{
	unit = unit_;

	reset ();

	unitSignalConnectionManager.disconnectAll ();

	if (unit)
	{
		unitSignalConnectionManager.connect (unit->data.hitpointsChanged, std::bind (&cUnitDetailsStored::reset, this));
		unitSignalConnectionManager.connect (unit->data.ammoChanged, std::bind (&cUnitDetailsStored::reset, this));
	}
}

//------------------------------------------------------------------------------
void cUnitDetailsStored::draw ()
{
	if (surface != nullptr)
	{
		SDL_Rect position = getArea ().toSdlRect ();
		SDL_BlitSurface (surface.get (), nullptr, cVideo::buffer, &position);
	}

	cWidget::draw ();
}

//------------------------------------------------------------------------------
void cUnitDetailsStored::reset ()
{
	SDL_FillRect (surface.get (), nullptr, 0xFF00FF);
	SDL_SetColorKey (surface.get (), SDL_TRUE, 0xFF00FF);

	for (std::size_t i = 0; i < maxRows; ++i)
	{
		amountLabels[i]->hide ();
		nameLabels[i]->hide ();
	}

	if (unit == nullptr) return;

	const auto& data = unit->data;

	drawRow (0, eUnitDataSymbolType::Hits, data.getHitpoints (), data.hitpointsMax, lngPack.i18n ("Text~Others~Hitpoints_7"));

	if (data.canAttack) drawRow (1, eUnitDataSymbolType::Ammo, data.getAmmo (), data.ammoMax, lngPack.i18n ("Text~Others~Ammo_7"));
}

//------------------------------------------------------------------------------
void cUnitDetailsStored::drawRow (size_t index, eUnitDataSymbolType symbolType, int amount, int maximalAmount, const std::string& name)
{
	if (index >= maxRows) return;

	amountLabels[index]->show ();
	nameLabels[index]->show ();

	eUnicodeFontType fontType;
	if (amount > maximalAmount / 2) fontType = FONT_LATIN_SMALL_GREEN;
	else if (amount > maximalAmount / 4) fontType = FONT_LATIN_SMALL_YELLOW;
	else fontType = FONT_LATIN_SMALL_RED;

	amountLabels[index]->setFont (fontType);
	amountLabels[index]->setText (iToStr (amount) + "/" + iToStr (maximalAmount));

	nameLabels[index]->setText (name);
	cUnitDetailsHud::drawSmallSymbols (surface.get (), rowHeight, symbolType, cPosition (65, 4 + rowHeight * index), amount, maximalAmount);
}