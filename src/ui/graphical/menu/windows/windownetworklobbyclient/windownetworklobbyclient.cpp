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

#include "ui/graphical/menu/windows/windownetworklobbyclient/windownetworklobbyclient.h"

#include "game/startup/lobbyclient.h"
#include "ui/graphical/menu/widgets/lineedit.h"
#include "ui/graphical/menu/widgets/pushbutton.h"
#include "utility/language.h"

//------------------------------------------------------------------------------
cWindowNetworkLobbyClient::cWindowNetworkLobbyClient() :
	cWindowNetworkLobby (lngPack.i18n ("Text~Others~TCPIP_Client"), false)
{
	setIsHost (false);

	auto connectButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (470, 200), ePushButtonType::StandardSmall, lngPack.i18n ("Text~Title~Connect")));
	signalConnectionManager.connect (connectButton->clicked, [this](){ triggeredConnect(); });

	signalConnectionManager.connect (ipLineEdit->returnPressed, [this]()
	{
		triggeredConnect ();
	});
}

//------------------------------------------------------------------------------
void cWindowNetworkLobbyClient::bindConnections (cLobbyClient& lobbyClient)
{
	cWindowNetworkLobby::bindConnections (lobbyClient);

	signalConnectionManager.connect (lobbyClient.onOptionsChanged, [this](std::shared_ptr<cGameSettings> settings, std::shared_ptr<cStaticMap> map, const cSaveGameInfo& saveGameInfo){
		setSaveGame (saveGameInfo);
	});
	signalConnectionManager.connect (lobbyClient.onPlayersList, [this](const cPlayerBasicData& localPlayer, const std::vector<cPlayerBasicData>& players){
		setIsHost (!players.empty() && localPlayer.getNr() == players[0].getNr());
	});

	signalConnectionManager.connect (triggeredConnect, [&lobbyClient, this](){
		// Connect only if there isn't a connection yet
		if (lobbyClient.isConnectedToServer()) return;

		const auto& ip = getIp();
		const auto& port = getPort();

		addInfoEntry (lngPack.i18n ("Text~Multiplayer~Network_Connecting") + ip + ":" + iToStr (port)); // e.g. Connecting to 127.0.0.1:55800
		disablePortEdit();
		disableIpEdit();

		lobbyClient.connectToServer (ip, port);
	});
}

//------------------------------------------------------------------------------
void cWindowNetworkLobbyClient::setIsHost (bool isHost)
{
	if (isHost && saveGameInfo.number == -1)
	{
		mapButton->show();
		okButton->show();
		settingsButton->show();
	}
	else
	{
		loadButton->hide();
		mapButton->hide();
		okButton->hide();
		settingsButton->hide();
	}
}

//------------------------------------------------------------------------------
void cWindowNetworkLobbyClient::setSaveGame (const cSaveGameInfo& saveInfo_)
{
	saveGameInfo = saveInfo_;

	updateSettingsText();
}
