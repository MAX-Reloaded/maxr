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

#include "lobbyplayerlistviewitem.h"
#include "../label.h"
#include "../image.h"
#include "../../../../player.h"

//------------------------------------------------------------------------------
cLobbyPlayerListViewItem::cLobbyPlayerListViewItem (std::shared_ptr<sPlayer> player_, int width) :
	player (std::move (player_))
{
	assert (player != nullptr);

	readyImage = addChild (std::make_unique<cImage> (getPosition () + cPosition (width - 10, 0), nullptr, SoundData.SNDHudButton));
	signalConnectionManager.connect (readyImage->clicked, [&](){ player->setReady (!player->isReady ()); });

	colorImage = addChild (std::make_unique<cImage> (getPosition ()));

	updatePlayerColor ();
	updatePlayerReady ();

	nameLabel = addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition () + cPosition (colorImage->getEndPosition().x() + 4, 0), getPosition () + cPosition (width - readyImage->getSize ().x (), readyImage->getSize ().y ())), player->getName ()));

	fitToChildren ();

	signalConnectionManager.connect (player->nameChanged, std::bind (&cLobbyPlayerListViewItem::updatePlayerName, this));
	signalConnectionManager.connect (player->colorChanged, std::bind (&cLobbyPlayerListViewItem::updatePlayerColor, this));
	signalConnectionManager.connect (player->readyChanged, std::bind (&cLobbyPlayerListViewItem::updatePlayerReady, this));
}

//------------------------------------------------------------------------------
const std::shared_ptr<sPlayer>& cLobbyPlayerListViewItem::getPlayer () const
{
	return player;
}

//------------------------------------------------------------------------------
void cLobbyPlayerListViewItem::updatePlayerName ()
{
	nameLabel->setText (player->getName ());
}

//------------------------------------------------------------------------------
void cLobbyPlayerListViewItem::updatePlayerColor ()
{
	SDL_Rect src = {0, 0, 10, 10};

	AutoSurface colorSurface (SDL_CreateRGBSurface (0, src.w, src.h, Video.getColDepth (), 0, 0, 0, 0));
	SDL_BlitSurface (player->getColorSurface (), &src, colorSurface, nullptr);

	colorImage->setImage (colorSurface);
}

//------------------------------------------------------------------------------
void cLobbyPlayerListViewItem::updatePlayerReady ()
{
	SDL_Rect src = {player->isReady () ? 10 : 0, 0, 10, 10};

	AutoSurface readySurface (SDL_CreateRGBSurface (0, src.w, src.h, Video.getColDepth (), 0, 0, 0, 0));
	SDL_SetColorKey (readySurface, SDL_TRUE, 0xFF00FF);
	SDL_FillRect (readySurface, NULL, 0xFF00FF);
	SDL_BlitSurface (GraphicsData.gfx_player_ready, &src, readySurface, NULL);

	readyImage->setImage (readySurface);
}