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

#ifndef SDLutility_drawingH
#define SDLutility_drawingH

#include <SDL.h>

class cRgbColor;
class cPosition;
template <typename>
class cBox;

void drawPoint (SDL_Surface&, const cPosition&, const cRgbColor&);
void drawLine (SDL_Surface&, const cPosition& start, const cPosition& end, const cRgbColor&);
void drawRectangle (SDL_Surface&, const cBox<cPosition>&, const cRgbColor&, int thickness = 1);

void drawSelectionCorner (SDL_Surface&, const cBox<cPosition>&, const cRgbColor&, int cornerSize);
void drawSelectionCorner (SDL_Surface&, const cBox<cPosition>&, const cRgbColor&, int cornerSize, const cBox<cPosition>& clipRect);

Uint32 getPixel (const SDL_Surface&, const cPosition&);
void putPixel (SDL_Surface&, const cPosition&, Uint32 pixel);

void replaceColor (SDL_Surface&, const cRgbColor& sourceColor, const cRgbColor& destinationColor);

/**
 * Blits a surface onto an other one at a given destination area.
 *
 * Additionally the destination area where the surface pixels will be changed will be clipped by an additional input area.
 *
 * @param source The surface to blit onto the the destination surface.
 * @param area The area in the destination where the source should be blit to.
 * @param destination The surface on which the source should be blit onto.
 * @param clipRect The clipping area.
 */
void blitClipped (SDL_Surface& source, const cBox<cPosition>& area, SDL_Surface& destination, const cBox<cPosition>& clipRect);

#endif
