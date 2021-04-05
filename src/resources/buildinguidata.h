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

#ifndef resources_buildinguidataH
#define resources_buildinguidataH

#include "game/data/units/unitdata.h" // for sID
#include "resources/sound.h"
#include "resources/uidata.h"

//--------------------------------------------------------------------------
/** struct for the images and sounds */
//--------------------------------------------------------------------------
struct sBuildingUIData
{
	sBuildingUIData() = default;
	sBuildingUIData (const sBuildingUIData&) = delete;
	sBuildingUIData (sBuildingUIData&&) = default;
	sBuildingUIData& operator= (const sBuildingUIData&) = delete;
	sBuildingUIData& operator= (sBuildingUIData&&) = default;

	/**
	* draws the main image of the building onto the given surface
	*/
	void render (unsigned long long animationTime, SDL_Surface&, const SDL_Rect& dest, float zoomFactor, const cBuilding&, bool drawShadow, bool drawConcrete) const;
	void render_simple (SDL_Surface&, const SDL_Rect& dest, float zoomFactor, const cPlayer* owner, int frameNr = 0, int alpha = 254) const;
	void render_simple (SDL_Surface&, const SDL_Rect& dest, float zoomFactor, const cBuilding&, unsigned long long animationTime = 0, int alpha = 254) const;

private:
	void drawConnectors (SDL_Surface&, SDL_Rect dest, float zoomFactor, const cBuilding&, bool drawShadow) const;

public:
	sID id;

	bool hasClanLogos = false;
	bool hasDamageEffect = false;
	bool hasBetonUnderground = false;
	bool hasPlayerColor = false;
	bool hasOverlay = false;

	bool buildUpGraphic = false;
	bool powerOnGraphic = false;
	bool isAnimated = false;

	bool isConnectorGraphic = false;
	int hasFrames = 0;

	AutoSurface img, img_org; // Surface of the building
	AutoSurface shw, shw_org; // Surfaces of the shadow
	AutoSurface eff, eff_org; // Surfaces of the effects
	AutoSurface video;  // video
	AutoSurface info;   // info image

	// Die Sounds:
	cSoundChunk Start;
	cSoundChunk Running;
	cSoundChunk Stop;
	cSoundChunk Attack;
	cSoundChunk Wait;
};

void render (const cBuilding&, unsigned long long animationTime, SDL_Surface&, const SDL_Rect& dest, float zoomFactor, bool drawShadow, bool drawConcrete);
void render_simple (const cBuilding&, SDL_Surface&, const SDL_Rect& dest, float zoomFactor, unsigned long long animationTime = 0, int alpha = 254);

#endif
