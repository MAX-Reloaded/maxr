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

#include "freezemode.h"

#include <cassert>
 
 //------------------------------------------------------------------------------
std::string enumToString(ePlayerConnectionState value)
{
	switch (value)
	{
	case ePlayerConnectionState::INACTIVE: return "INACTIVE";
	case ePlayerConnectionState::CONNECTED: return "CONNECTED";
	case ePlayerConnectionState::NOT_RESPONDING: return "NOT_RESPONDING";
	case ePlayerConnectionState::DISCONNECTED: return "DISCONNECTED";
	}
	assert(false);
	return std::to_string(static_cast<int>(value));
}
 
//------------------------------------------------------------------------------
std::string enumToString(eFreezeMode value)
{
	switch (value)
	{
	case eFreezeMode::WAIT_FOR_TURNEND: return "WAIT_FOR_TURNEND";
	case eFreezeMode::PAUSE: return "PAUSE";
	case eFreezeMode::WAIT_FOR_CLIENT: return "WAIT_FOR_CLIENT";
	case eFreezeMode::WAIT_FOR_SERVER: return "WAIT_FOR_SERVER";
	}
	assert(false);
	return std::to_string(static_cast<int>(value));
}

//------------------------------------------------------------------------------
void cFreezeModes::enable (eFreezeMode mode)
{
	switch (mode)
	{
	case eFreezeMode::WAIT_FOR_TURNEND:
		waitForTurnEnd = true;
		break;
	case eFreezeMode::PAUSE:
		pause = true;
		break;
	case eFreezeMode::WAIT_FOR_CLIENT:
		waitForClient = true;
		break; 
	case eFreezeMode::WAIT_FOR_SERVER:
		waitForServer = true;
		break;
	}
	assert(false);
}

//------------------------------------------------------------------------------
void cFreezeModes::disable (eFreezeMode mode)
{
	switch (mode)
	{
	case eFreezeMode::WAIT_FOR_TURNEND:
		waitForTurnEnd = false;
		break;
	case eFreezeMode::PAUSE:
		pause = false;
		break;
	case eFreezeMode::WAIT_FOR_CLIENT:
		waitForClient = false;
		break;
	case eFreezeMode::WAIT_FOR_SERVER:
		waitForServer = false;
		break;
	}
	assert(false);
}

//------------------------------------------------------------------------------
bool cFreezeModes::isFreezed() const
{
	return pause | waitForTurnEnd | waitForClient | waitForServer;
}

//------------------------------------------------------------------------------
bool cFreezeModes::gameTimePaused() const
{
	return waitForClient | pause;
}

//------------------------------------------------------------------------------
bool cFreezeModes::isEnabled (eFreezeMode mode) const
{
	switch (mode)
	{
		case eFreezeMode::PAUSE: return pause;
		case eFreezeMode::WAIT_FOR_TURNEND: return waitForTurnEnd;
		case eFreezeMode::WAIT_FOR_CLIENT: return waitForClient;
		case eFreezeMode::WAIT_FOR_SERVER: return waitForServer;
	}
	assert (0); // Incorrect parameter
	return false;
}