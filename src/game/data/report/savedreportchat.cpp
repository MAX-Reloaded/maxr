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

#include "game/data/report/savedreportchat.h"

#include "netmessage.h"
#include "game/data/player/player.h"

//------------------------------------------------------------------------------
cSavedReportChat::cSavedReportChat (const cPlayer& player, std::string text_) :
	playerName (player.getName()),
	playerNumber (player.getNr()),
	text (std::move (text_))
{}

//------------------------------------------------------------------------------
cSavedReportChat::cSavedReportChat (std::string playerName_, std::string text_) :
	playerName (std::move (playerName_)),
	playerNumber (-1),
	text (std::move (text_))
{}

//------------------------------------------------------------------------------
cSavedReportChat::cSavedReportChat (cNetMessage& message)
{
	playerName = message.popString();
	playerNumber = message.popInt32();
	text = message.popString();
}

//------------------------------------------------------------------------------
cSavedReportChat::cSavedReportChat (const tinyxml2::XMLElement& element)
{
	text = element.Attribute ("msg");
	if (auto value = element.Attribute ("playername")) playerName = value;
	playerNumber = 0;
	element.QueryIntAttribute ("playernumber", &playerNumber);
}

//------------------------------------------------------------------------------
void cSavedReportChat::pushInto (cNetMessage& message) const
{
	message.pushString (text);
	message.pushInt32 (playerNumber);
	message.pushString (playerName);

	cSavedReport::pushInto (message);
}

//------------------------------------------------------------------------------
void cSavedReportChat::pushInto (tinyxml2::XMLElement& element) const
{
	element.SetAttribute ("msg", text.c_str());
	element.SetAttribute ("playername", playerName.c_str());
	element.SetAttribute ("playernumber", iToStr (playerNumber).c_str());

	cSavedReport::pushInto (element);
}

//------------------------------------------------------------------------------
eSavedReportType cSavedReportChat::getType() const
{
	return eSavedReportType::Chat;
}

//------------------------------------------------------------------------------
std::string cSavedReportChat::getMessage() const
{
	return playerName + lngPack.i18n ("Text~Punctuation~Colon") + text;
}

//------------------------------------------------------------------------------
bool cSavedReportChat::isAlert() const
{
	return false;
}

//------------------------------------------------------------------------------
int cSavedReportChat::getPlayerNumber() const
{
	return playerNumber;
}

//------------------------------------------------------------------------------
const std::string& cSavedReportChat::getText() const
{
	return text;
}

//------------------------------------------------------------------------------
const std::string& cSavedReportChat::getPlayerName() const
{
	return playerName;
}
