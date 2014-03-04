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

#include "animationtimer.h"

//--------------------------------------------------------------------------
cAnimationTimeFlags::cAnimationTimeFlags () :
	is10msFlag (false),
	is50msFlag (false),
	is100msFlag (false),
	is400msFlag (false)
{}

//--------------------------------------------------------------------------
bool cAnimationTimeFlags::is10ms () const
{
	return is50msFlag;
}

//--------------------------------------------------------------------------
bool cAnimationTimeFlags::is50ms () const
{
	return is50msFlag;
}

//--------------------------------------------------------------------------
bool cAnimationTimeFlags::is100ms () const
{
	return is100msFlag;
}

//--------------------------------------------------------------------------
bool cAnimationTimeFlags::is400ms () const
{
	return is400msFlag;
}

//--------------------------------------------------------------------------
void  cAnimationTimeFlags::set10ms (bool flag)
{
	is50msFlag = flag;
}

//--------------------------------------------------------------------------
void  cAnimationTimeFlags::set50ms (bool flag)
{
	is50msFlag = flag;
}

//--------------------------------------------------------------------------
void  cAnimationTimeFlags::set100ms (bool flag)
{
	is100msFlag = flag;
}

//--------------------------------------------------------------------------
void  cAnimationTimeFlags::set400ms (bool flag)
{
	is400msFlag = flag;
}

namespace {

//--------------------------------------------------------------------------
Uint32 timerCallback (Uint32 interval, void* arg)
{
	static_cast<cAnimationTimer*>(arg)->increaseTimer ();
	return interval;
}

}

//--------------------------------------------------------------------------
cAnimationTimer::cAnimationTimer () :
	timerTime (0),
	lastUpdateTimerTime (0)
{
	timerId = SDL_AddTimer (10, timerCallback, this);
}

//--------------------------------------------------------------------------
cAnimationTimer::~cAnimationTimer ()
{
	SDL_RemoveTimer (timerId);
}

//--------------------------------------------------------------------------
void cAnimationTimer::increaseTimer ()
{
	++timerTime;
}

//--------------------------------------------------------------------------
unsigned long long cAnimationTimer::getAnimationTime () const
{
	return timerTime / 10; // in 100ms steps
}

//--------------------------------------------------------------------------
void cAnimationTimer::updateAnimationFlags ()
{
	animationFlags.set10ms (false);
	animationFlags.set50ms (false);
	animationFlags.set100ms (false);
	animationFlags.set400ms (false);

	if (lastUpdateTimerTime == timerTime) return;

	lastUpdateTimerTime = timerTime;

	animationFlags.set10ms (true);

	if (timerTime % 5 == 0) animationFlags.set50ms (true);
	if (timerTime % 10 == 0) animationFlags.set100ms (true);
	if (timerTime % 40 == 0) animationFlags.set400ms (true);

	triggered10ms ();
	if (timerTime % 5 == 0) triggered50ms ();
	if (timerTime % 10 == 0) triggered100ms ();
	if (timerTime % 40 == 0) triggered400ms ();
}

//--------------------------------------------------------------------------
const cAnimationTimeFlags& cAnimationTimer::getAnimationFlags () const
{
	return animationFlags;
}