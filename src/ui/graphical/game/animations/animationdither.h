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

#ifndef ui_graphical_game_animations_animationditherH
#define ui_graphical_game_animations_animationditherH

#include "ui/graphical/game/animations/animation.h"
#include "utility/signal/signalconnectionmanager.h"

class cAnimationTimer;
class cVehicle;

/**
 * Simple dither animation for flying planes.
 */
class cAnimationDither : public cAnimation
{
public:
	cAnimationDither (cAnimationTimer&, const cVehicle&);
	~cAnimationDither();

	bool isLocatedIn (const cBox<cPosition>& box) const override;

private:
	cSignalConnectionManager signalConnectionManager;
	cSignalConnectionManager animationTimerConnectionManager;
	cAnimationTimer& animationTimer;
	const cVehicle* vehicle = nullptr;

	bool incrementEffect = false;

	void activate();
	void run();
};

#endif // ui_graphical_game_animations_animationditherH
