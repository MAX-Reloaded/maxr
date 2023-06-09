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

#include "ui/graphical/game/widgets/turntimeclockwidget.h"

#include "game/logic/turntimeclock.h"
#include "ui/widgets/label.h"

#include <iomanip>
#include <sstream>

//------------------------------------------------------------------------------
cTurnTimeClockWidget::cTurnTimeClockWidget (const cBox<cPosition>& area) :
	cWidget (area)
{
	textLabel = emplaceChild<cLabel> (area, "", eUnicodeFontType::LatinNormal, eAlignmentType::CenterHorizontal);
}

//------------------------------------------------------------------------------
void cTurnTimeClockWidget::setTurnTimeClock (std::shared_ptr<const cTurnTimeClock> turnTimeClock_)
{
	turnTimeClock = std::move (turnTimeClock_);

	signalConnectionManager.disconnectAll();

	if (turnTimeClock != nullptr)
	{
		signalConnectionManager.connect (turnTimeClock->secondChanged, [this]() { update(); });
		signalConnectionManager.connect (turnTimeClock->deadlinesChanged, [this]() { update(); });
	}
	update();
}

//------------------------------------------------------------------------------
void cTurnTimeClockWidget::update()
{
	if (turnTimeClock == nullptr)
	{
		textLabel->setText ("");
		textLabel->setFont (eUnicodeFontType::LatinNormal);
		return;
	}
	const auto time = turnTimeClock->hasDeadline() ? turnTimeClock->getTimeTillFirstDeadline() : turnTimeClock->getTimeSinceStart();

	textLabel->setText (to_MM_ss (time));

	if (turnTimeClock->hasDeadline() && std::chrono::duration_cast<std::chrono::seconds> (time) <= cTurnTimeClock::alertRemainingTime)
	{
		textLabel->setFont (eUnicodeFontType::LatinNormalRed);
	}
	else
	{
		textLabel->setFont (eUnicodeFontType::LatinNormal);
	}
}
