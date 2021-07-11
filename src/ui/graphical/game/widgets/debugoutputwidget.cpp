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

#include "ui/graphical/game/widgets/debugoutputwidget.h"

#include "game/data/player/player.h"
#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"
#include "game/logic/client.h"
#include "game/logic/server.h"
#include "input/mouse/mouse.h"
#include "output/video/video.h"
#include "resources/uidata.h"
#include "ui/graphical/game/control/chatcommand/chatcommand.h"
#include "ui/graphical/game/control/chatcommand/chatcommandexecutor.h"
#include "ui/graphical/game/control/chatcommand/chatcommandparser.h"
#include "ui/graphical/game/control/chatcommand/chatcommandarguments.h"
#include "ui/graphical/game/widgets/gamemapwidget.h"
#include "ui/graphical/game/animations/animation.h"
#include "ui/graphical/playercolor.h"
#include "ui/translations.h"
#include "utility/indexiterator.h"
#include "utility/language.h"
#include "utility/listhelpers.h"
#include "utility/mathtools.h"

namespace
{
	//--------------------------------------------------------------------------
	std::string iToHex (unsigned int x)
	{
		std::stringstream strStream;
		strStream << std::hex << x;
		return strStream.str();
	}

	//--------------------------------------------------------------------------
	std::string pToStr (const void* x)
	{
		std::stringstream strStream;
		strStream << x;
		return "0x" + strStream.str();
	}
}


//------------------------------------------------------------------------------
cDebugOutputWidget::cDebugOutputWidget (const cBox<cPosition>& area) :
	cWidget (area)
{}

//------------------------------------------------------------------------------
void cDebugOutputWidget::setClient (const cClient* client_)
{
	client = client_;
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::setServer (const cServer* server_)
{
	server = server_;
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::setGameMap (const cGameMapWidget* gameMap_)
{
	gameMap = gameMap_;
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::initChatCommand (std::vector<std::unique_ptr<cChatCommandExecutor>>& chatCommands)
{
	chatCommands.push_back (
		cChatCommand ("base", "Enable/disable debug information about bases")
		.addArgument<cChatCommandArgumentChoice> (std::vector<std::string>{"client", "server", "off"})
		.setAction ([this](const std::string& value)
		{
			if (value == "server")
			{
				debugBaseServer = true;
				debugBaseClient = false;
			}
			else if (value == "client")
			{
				debugBaseServer = false;
				debugBaseClient = true;
			}
			else
			{
				debugBaseServer = false;
				debugBaseClient = false;
			}
		})
	);
	chatCommands.push_back (
		cChatCommand ("sentry", "Enable/disable debug information about the sentry status")
		.addArgument<cChatCommandArgumentChoice> (std::vector<std::string>{"server", "off"})
		.setAction ([this](const std::string& value)
		{
			debugSentry = (value == "server");
		})
	);
	chatCommands.push_back (
		cChatCommand ("fx", "Enable/disable debug information about effects")
		.addArgument<cChatCommandArgumentBool>()
		.setAction ([this](bool flag)
		{
			debugFX = flag;
		})
	);
	chatCommands.push_back (
		cChatCommand ("trace", "Enable/disable debug information about the unit currently under the cursor")
		.addArgument<cChatCommandArgumentChoice> (std::vector<std::string>{"client", "server", "off"})
		.setAction ([this](const std::string& value)
		{
			if (value == "server")
			{
				debugTraceServer = true;
				debugTraceClient = false;
			}
			else if (value == "client")
			{
				debugTraceServer = false;
				debugTraceClient = true;
			}
			else
			{
				debugTraceServer = false;
				debugTraceClient = false;
			}
		})
	);
	chatCommands.push_back (
		cChatCommand ("ajobs", "Enable/disable debug information about attack jobs")
		.addArgument<cChatCommandArgumentBool>()
		.setAction ([this](bool flag)
		{
			debugAjobs = flag;
		})
	);
	chatCommands.push_back (
		cChatCommand ("players", "Enable/disable debug information about players")
		.addArgument<cChatCommandArgumentBool>()
		.setAction ([this](bool flag)
		{
			debugPlayers = flag;
		})
	);
	chatCommands.push_back (
		cChatCommand ("cache debug", "Enable/disable debug information about the drawing cache")
		.addArgument<cChatCommandArgumentBool>()
		.setAction ([this](bool flag)
		{
			debugCache = flag;
		})
	);
	chatCommands.push_back (
		cChatCommand ("sync debug", "Enable/disable debug information about the sync state of the game data")
		.addArgument<cChatCommandArgumentBool>()
		.setAction ([this](bool flag)
		{
			debugSync = flag;
		})
	);
	chatCommands.push_back (
		cChatCommand ("debug stealth", "Enable/disable debug information about the stealth state of units")
		.addArgument<cChatCommandArgumentBool>()
		.setAction ([this](bool flag)
		{
			debugStealth = flag;
		})
	);
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::draw (SDL_Surface& destination, const cBox<cPosition>& clipRect)
{
	cWidget::draw (destination, clipRect);

	if (!client) return;

	auto font = cUnicodeFont::font.get();

	const cPlayer& player = client->getActivePlayer();

	setPrintPosition (cPosition (getEndPosition().x() - 200, getPosition().y()));

	if (debugPlayers)
	{
		print ("Players: " + std::to_string (client->model.getPlayerList().size()));

		SDL_Rect rDest = {drawPosition.x(), drawPosition.y(), 20, 10};
		SDL_Rect rSrc = {0, 0, 20, 10};
		SDL_Rect rDotDest  = {drawPosition.x() - 10, drawPosition.y(), 10, 10};
		SDL_Rect rBlackOut = {drawPosition.x() + 20, drawPosition.y(),  0, 10};
		const auto& playerList = client->model.getPlayerList();
		for (size_t i = 0; i != playerList.size(); ++i)
		{
			// HACK SHOWFINISHEDPLAYERS
			SDL_Rect rDot = {10, 0, 10, 10}; // for green dot

			if (playerList[i]->getHasFinishedTurn() /* && playerList[i] != &player*/)
			{
				SDL_BlitSurface (GraphicsData.gfx_player_ready.get(), &rDot, &destination, &rDotDest);
			}
#if 0
			else if (playerList[i] == &player && client->bWantToEnd)
			{
				SDL_BlitSurface (GraphicsData.gfx_player_ready.get(), &rDot, &destination, &rDotDest);
			}
#endif
			else
			{
				rDot.x = 0; // for red dot
				SDL_BlitSurface (GraphicsData.gfx_player_ready.get(), &rDot, &destination, &rDotDest);
			}

			SDL_BlitSurface (cPlayerColor::getTexture (playerList[i]->getColor()), &rSrc, &destination, &rDest);
			if (playerList[i].get() == &player)
			{
				std::string sTmpLine = " " + playerList[i]->getName() + ", nr: " + std::to_string (playerList[i]->getId()) + " << you! ";
				// black out background for better recognizing
				rBlackOut.w = font->getTextWide (sTmpLine, FONT_LATIN_SMALL_WHITE);
				SDL_FillRect (&destination, &rBlackOut, 0xFF000000);
				font->showText (rBlackOut.x, drawPosition.y() + 1, sTmpLine, FONT_LATIN_SMALL_WHITE);
			}
			else
			{
				std::string sTmpLine = " " + playerList[i]->getName() + ", nr: " + std::to_string (playerList[i]->getId()) + " ";
				// black out background for better recognizing
				rBlackOut.w = font->getTextWide (sTmpLine, FONT_LATIN_SMALL_WHITE);
				SDL_FillRect (&destination, &rBlackOut, 0xFF000000);
				font->showText (rBlackOut.x, drawPosition.y() + 1, sTmpLine, FONT_LATIN_SMALL_WHITE);
			}
			// use 10 for pixel high of dots instead of text high
			drawPosition.y() += 10;
			rDest.y = rDotDest.y = rBlackOut.y = drawPosition.y();
		}
	}

	if (debugAjobs)
	{
		print ("ClientAttackJobs: " + std::to_string (client->getModel().attackJobs.size()));
		if (server)
		{
			print ("ServerAttackJobs: " + std::to_string (server->getModel().attackJobs.size()));
		}
	}

	if (debugBaseClient)
	{
		print ("subbases: " + std::to_string (player.base.SubBases.size()));
	}

	if (debugBaseServer && server)
	{
		const auto serverPlayer = server->model.getPlayer (player.getId());
		print ("subbases: " + std::to_string (serverPlayer->base.SubBases.size()));
	}

	if (debugFX)
	{
		if (gameMap)
		{
			print ("total-animations-count: " + std::to_string (gameMap->animations.size()));

			const auto runningAnimations = ranges::count_if (gameMap->animations, [] (const std::unique_ptr<cAnimation>& animation) { return animation->isRunning(); });
			print ("running-animations-count: " + std::to_string (runningAnimations));
			const auto finishedAnimations = ranges::count_if (gameMap->animations, [] (const std::unique_ptr<cAnimation>& animation) { return animation->isFinished(); });
			print ("finished-animations-count: " + std::to_string (finishedAnimations));
			print ("gui-fx-count: " + std::to_string (gameMap->effects.size()));
		}
		print ("client-fx-count: " + std::to_string (client->getModel().effectsList.size()));
	}
	if (debugTraceServer || debugTraceClient)
	{
		trace();
	}
	if (debugCache && gameMap)
	{
		const auto& drawingCache = gameMap->getDrawingCache();

		print ("Max cache size: " + std::to_string (drawingCache.getMaxCacheSize()));
		print ("cache size: " + std::to_string (drawingCache.getCacheSize()));
		print ("cache hits: " + std::to_string (drawingCache.getCacheHits()));
		print ("cache misses: " + std::to_string (drawingCache.getCacheMisses()));
		print ("not cached: " + std::to_string (drawingCache.getNotCached()));
	}

	if (debugSync)
	{
		font->showText (drawPosition.x() - 10, drawPosition.y(), "-Client:", FONT_LATIN_SMALL_YELLOW);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_YELLOW);
		if (server)
		{
			print ("-Server:");
			font->showText (drawPosition.x(), drawPosition.y(), "Server Time: ", FONT_LATIN_SMALL_WHITE);
			font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (server->model.getGameTime()), FONT_LATIN_SMALL_WHITE);
			drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

			font->showText (drawPosition.x(), drawPosition.y(), "Net MSG Queue: ", FONT_LATIN_SMALL_WHITE);
			font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (server->eventQueue.safe_size()), FONT_LATIN_SMALL_WHITE);
			drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

			font->showText (drawPosition.x(), drawPosition.y(), "EventCounter: ", FONT_LATIN_SMALL_WHITE);
			font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (server->gameTimer.eventCounter), FONT_LATIN_SMALL_WHITE);
			drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

			for (const auto& player : server->model.playerList)
			{
				font->showText (drawPosition.x(), drawPosition.y(), "Client " + std::to_string (player->getId()) + lngPack.i18n ("Text~Punctuation~Colon"), FONT_LATIN_SMALL_WHITE);
				drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

				font->showText (drawPosition.x() + 10, drawPosition.y(), "Client time: ", FONT_LATIN_SMALL_WHITE);
				font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (server->gameTimer.receivedTime.at (player->getId())), FONT_LATIN_SMALL_WHITE);
				drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);


				if (server->gameTimer.clientDebugData.at (player->getId()).crcOK)
					font->showText (drawPosition.x() + 10, drawPosition.y(), "Sync OK", FONT_LATIN_SMALL_GREEN);
				else
					font->showText (drawPosition.x() + 10, drawPosition.y(), "Out of Sync!", FONT_LATIN_SMALL_RED);
				drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

				const auto& debugData= server->gameTimer.clientDebugData.at (player->getId());
				font->showText (drawPosition.x() + 10, drawPosition.y(), "Timebuffer: ", FONT_LATIN_SMALL_WHITE);
				font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string ((int)debugData.timeBuffer), FONT_LATIN_SMALL_WHITE);
				drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

				font->showText (drawPosition.x() + 10, drawPosition.y(), "Ticks per Frame: ", FONT_LATIN_SMALL_WHITE);
				font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string ((int)debugData.ticksPerFrame), FONT_LATIN_SMALL_WHITE);
				drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

				font->showText (drawPosition.x() + 10, drawPosition.y(), "Queue Size: ", FONT_LATIN_SMALL_WHITE);
				font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string ((int)debugData.queueSize), FONT_LATIN_SMALL_WHITE);
				drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

				font->showText (drawPosition.x() + 10, drawPosition.y(), "Event counter: ", FONT_LATIN_SMALL_WHITE);
				font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string ((int)debugData.eventCounter), FONT_LATIN_SMALL_WHITE);
				drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

				font->showText (drawPosition.x() + 10, drawPosition.y(), "Ping (ms): ", FONT_LATIN_SMALL_WHITE);
				font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string ((int)debugData.ping), FONT_LATIN_SMALL_WHITE);
				drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);
			}
		}

		font->showText (drawPosition.x() - 10, drawPosition.y(), "-Client:", FONT_LATIN_SMALL_YELLOW);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);
		eUnicodeFontType fontType = FONT_LATIN_SMALL_GREEN;
		if (client->gameTimer->debugRemoteChecksum != client->gameTimer->localChecksum)
			fontType = FONT_LATIN_SMALL_RED;
		font->showText (drawPosition.x(), drawPosition.y(), "Server Checksum: ", FONT_LATIN_SMALL_WHITE);
		font->showText (drawPosition.x() + 110, drawPosition.y(), "0x" + iToHex (client->gameTimer->debugRemoteChecksum), fontType);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

		font->showText (drawPosition.x(), drawPosition.y(), "Client Checksum: ", FONT_LATIN_SMALL_WHITE);
		font->showText (drawPosition.x() + 110, drawPosition.y(), "0x" + iToHex (client->gameTimer->localChecksum), fontType);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

		font->showText (drawPosition.x(), drawPosition.y(), "Client Time: ", FONT_LATIN_SMALL_WHITE);
		font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (client->model.getGameTime()), FONT_LATIN_SMALL_WHITE);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

		font->showText (drawPosition.x(), drawPosition.y(), "Net MGS Queue: ", FONT_LATIN_SMALL_WHITE);
		font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (client->eventQueue.safe_size()), FONT_LATIN_SMALL_WHITE);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

		font->showText (drawPosition.x(), drawPosition.y(), "EventCounter: ", FONT_LATIN_SMALL_WHITE);
		font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (client->gameTimer->eventCounter), FONT_LATIN_SMALL_WHITE);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

		font->showText (drawPosition.x(), drawPosition.y(), "Time Buffer: ", FONT_LATIN_SMALL_WHITE);
		font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (client->gameTimer->getReceivedTime() - client->model.getGameTime()), FONT_LATIN_SMALL_WHITE);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

		font->showText (drawPosition.x(), drawPosition.y(), "Ticks per Frame ", FONT_LATIN_SMALL_WHITE);
		static unsigned int lastGameTime = 0;
		font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (client->model.getGameTime() - lastGameTime), FONT_LATIN_SMALL_WHITE);
		lastGameTime = client->model.getGameTime();
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);

		font->showText (drawPosition.x(), drawPosition.y(), "Ping: ", FONT_LATIN_SMALL_WHITE);
		font->showText (drawPosition.x() + 110, drawPosition.y(), std::to_string (client->gameTimer->ping), FONT_LATIN_SMALL_WHITE);
		drawPosition.y() += font->getFontHeight (FONT_LATIN_SMALL_WHITE);
	}

	if (debugStealth)
	{
		drawDetectedByPlayerList();
		drawDetectionMaps();
	}

	if (debugSentry)
	{
		drawSentryMaps();
	}
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::setPrintPosition (cPosition position)
{
	drawPosition = position;
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::print (const std::string& text, eUnicodeFontType font_ /*= FONT_LATIN_SMALL_WHITE*/)
{
	cUnicodeFont::font->showText (drawPosition.x(), drawPosition.y(), text, font_);
	drawPosition.y() += cUnicodeFont::font->getFontHeight (font_);
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::trace()
{
	if (!gameMap) return;

	auto mouse = gameMap->getActiveMouse();
	if (!mouse) return;

	if (!gameMap->getArea().withinOrTouches (mouse->getPosition())) return;

	const auto mapPosition = gameMap->getMapTilePosition (mouse->getPosition());

	const cMapField* field;

	if (debugTraceServer && server)
	{
		if (!server->getModel().getMap()->isValidPosition (mapPosition)) return;
		field = &server->getModel().getMap()->getField (mapPosition);
	}
	else
	{
		if (!client->getModel().getMap()->isValidPosition (mapPosition)) return;
		field = &client->getModel().getMap()->getField (mapPosition);
	}

	cPosition drawingPosition = getPosition() + cPosition (0, 0);

	if (field->getVehicle()) { traceVehicle (*field->getVehicle(), drawingPosition); drawingPosition.y() += 20; }
	if (field->getPlane()) { traceVehicle (*field->getPlane(), drawingPosition); drawingPosition.y() += 20; }
	const auto& buildings = field->getBuildings();
	for (auto it = buildings.begin(); it != buildings.end(); ++it)
	{
		traceBuilding (**it, drawingPosition); drawingPosition.y() += 20;
	}
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::traceVehicle (const cVehicle& vehicle, cPosition& drawPosition)
{
	std::string tmpString;

	auto font = cUnicodeFont::font.get();

	tmpString = "name: \"" + getDisplayName (vehicle) + "\" id: \"" + std::to_string (vehicle.iID) + "\" owner: \"" + (vehicle.getOwner() ? vehicle.getOwner()->getName() : "<null>") + "\" pos: " + toString (vehicle.getPosition()) + " offset: " + toString (vehicle.getMovementOffset());
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	tmpString = "dir: " + std::to_string (vehicle.dir) + " moving: +" + std::to_string (vehicle.isUnitMoving()) + " mjob: " + pToStr (vehicle.getMoveJob()) + " speed: " + std::to_string (vehicle.data.getSpeed());
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	tmpString = " attacking: " + std::to_string (vehicle.isAttacking()) + " on sentry: +" + std::to_string (vehicle.isSentryActive()) + " dither: (" + std::to_string (vehicle.dither.x()) + ", " + std::to_string (vehicle.dither.y()) + ")";
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	tmpString = "is_building: " + std::to_string (vehicle.isUnitBuildingABuilding()) + " building_typ: " + vehicle.getBuildingType().getText() + " build_costs: +" + std::to_string (vehicle.getBuildCosts()) + " build_rounds: " + std::to_string (vehicle.getBuildTurns()) + " build_round_start: " + std::to_string (vehicle.getBuildTurnsStart());
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	tmpString = " band: " + toString (vehicle.bandPosition) + " build_big_saved_pos: " + std::to_string (vehicle.buildBigSavedPosition.x()) + "x" + std::to_string (vehicle.buildBigSavedPosition.y()) + " build_path: " + std::to_string (vehicle.BuildPath);
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	tmpString = " is_clearing: " + std::to_string (vehicle.isUnitClearing()) + " clearing_rounds: +" + std::to_string (vehicle.getClearingTurns()) + " clear_big: " + std::to_string (vehicle.getIsBig()) + " loaded: " + std::to_string (vehicle.isUnitLoaded());
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	tmpString = vehicle.getCommandoData().getDebugString() + " disabled: " + std::to_string (vehicle.getDisabledTurns());
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	tmpString = "clear_mines: +" + std::to_string (vehicle.isUnitClearingMines()) + " lay_mines: " + std::to_string (vehicle.isUnitLayingMines());
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	tmpString = " stored_vehicles_count: " + std::to_string ((int)vehicle.storedUnits.size());
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	for (size_t i = 0; i != vehicle.storedUnits.size(); ++i)
	{
		const cVehicle* storedVehicle = vehicle.storedUnits[i];
		font->showText (drawPosition, " store " + std::to_string (i) + ": \"" + getDisplayName (*storedVehicle) + "\"", FONT_LATIN_SMALL_WHITE);
		drawPosition.y() += 8;
	}

	tmpString = "flight height: " + std::to_string (vehicle.getFlightHeight());
	font->showText (drawPosition, tmpString, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::traceBuilding (const cBuilding& building, cPosition& drawPosition)
{
	std::string s;

	auto font = cUnicodeFont::font.get();

	s = "name: \"" + getDisplayName (building) + "\" id: \"" + std::to_string (building.iID) + "\" owner: \"" + (building.getOwner() ? building.getOwner()->getName() : "<null>") + "\" pos: " + toString (building.getPosition());
	font->showText (drawPosition, s, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	s= "dir: " + std::to_string (building.dir) + " on sentry: +" + std::to_string (building.isSentryActive()) + " sub_base: " + pToStr (building.subBase);
	font->showText (drawPosition, s, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	s = "attacking: " + std::to_string (building.isAttacking()) + " UnitsData.dirt_typ: " + std::to_string (building.rubbleTyp) + " UnitsData.dirt_value: +" + std::to_string (building.rubbleValue) + " big_dirt: " + std::to_string (building.getIsBig()) + " is_working: " + std::to_string (building.isUnitWorking());
	font->showText (drawPosition, s, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	s = " production: {" + std::to_string (building.prod.metal) + "/" + std::to_string (building.maxProd.metal)
		+ ", " + std::to_string (building.prod.oil) + "/" + std::to_string (building.maxProd.oil)
		+ ", " + std::to_string (building.prod.gold) + "/" + std::to_string (building.maxProd.gold) + "}";
	font->showText (drawPosition, s, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	s = "disabled: " + std::to_string (building.getDisabledTurns());
	font->showText (drawPosition, s, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	s = " stored_vehicles_count: " + std::to_string ((int)building.storedUnits.size());
	font->showText (drawPosition, s, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	for (size_t i = 0; i != building.storedUnits.size(); ++i)
	{
		const cVehicle* storedVehicle = building.storedUnits[i];
		font->showText (drawPosition, " store " + std::to_string (i) + ": \"" + getDisplayName (*storedVehicle) + "\"", FONT_LATIN_SMALL_WHITE);
		drawPosition.y() += 8;
	}

	const size_t buildingBuildListSize = building.getBuildListSize();
	s = "build_speed: "        + std::to_string (building.getBuildSpeed()) +
		" repeat_build: "      + std::to_string (building.getRepeatBuild()) +
		" build_list_count: +" + std::to_string ((int)buildingBuildListSize);
	font->showText (drawPosition, s, FONT_LATIN_SMALL_WHITE);
	drawPosition.y() += 8;

	for (size_t i = 0; i != buildingBuildListSize; ++i)
	{
		const auto& item = building.getBuildListItem (i);
		auto itemName = getStaticUnitName (client->getModel().getUnitsData()->getStaticUnitData (item.getType()));
		font->showText (drawPosition, "  build " + std::to_string (i) + lngPack.i18n ("Text~Punctuation~Colon") + item.getType().getText() + " \"" + itemName + "\"", FONT_LATIN_SMALL_WHITE);
		drawPosition.y() += 8;
	}
}

void cDebugOutputWidget::drawDetectedByPlayerList()
{
	if (!gameMap || !client) return;
	const auto& map = *client->getModel().getMap();
	const auto zoomedTileSize = gameMap->getZoomedTileSize();
	const auto tileDrawingRange = gameMap->computeTileDrawingRange();
	const auto zoomedStartTilePixelOffset = gameMap->getZoomedStartTilePixelOffset();

	auto font = cUnicodeFont::font.get();

	for (auto i = makeIndexIterator (tileDrawingRange.first, tileDrawingRange.second); i.hasMore(); i.next())
	{
		auto& mapField = map.getField (*i);
		auto building = mapField.getBuilding();
		if (building == nullptr) continue;

		auto drawDestination = gameMap->computeTileDrawingArea (zoomedTileSize, zoomedStartTilePixelOffset, tileDrawingRange.first, building->getPosition());
		drawDestination.x += 4;
		drawDestination.y += 4;

		for (const auto& playerId : building->detectedByPlayerList)
		{
			if (Contains (building->detectedInThisTurnByPlayerList, playerId))
			{
				font->showText (drawDestination.x, drawDestination.y, std::to_string (playerId) + "#", FONT_LATIN_SMALL_RED);
			}
			else
			{
				font->showText (drawDestination.x, drawDestination.y, std::to_string (playerId), FONT_LATIN_SMALL_RED);
			}
			drawDestination.y += font->getFontHeight (FONT_LATIN_SMALL_RED);
		}
	}

	for (auto i = makeIndexIterator (tileDrawingRange.first, tileDrawingRange.second); i.hasMore(); i.next())
	{
		auto& mapField = map.getField (*i);
		auto vehicle = mapField.getVehicle();
		if (vehicle == nullptr) continue;

		auto drawDestination = gameMap->computeTileDrawingArea (zoomedTileSize, zoomedStartTilePixelOffset, tileDrawingRange.first, vehicle->getPosition());
		drawDestination.x += (int)(4 + vehicle->getMovementOffset().x() * gameMap->getZoomFactor());
		drawDestination.y += (int)(4 + vehicle->getMovementOffset().y() * gameMap->getZoomFactor());

		for (const auto& playerId : vehicle->detectedByPlayerList)
		{
			if (Contains (vehicle->detectedInThisTurnByPlayerList, playerId))
			{
				font->showText (drawDestination.x, drawDestination.y, std::to_string (playerId) + "#", FONT_LATIN_SMALL_RED);
			}
			else
			{
				font->showText (drawDestination.x, drawDestination.y, std::to_string (playerId), FONT_LATIN_SMALL_RED);
			}
			drawDestination.y += font->getFontHeight (FONT_LATIN_SMALL_RED);
		}
	}
}

//------------------------------------------------------------------------------
void cDebugOutputWidget::drawDetectionMaps()
{
	if (!gameMap || !client) return;

	const auto zoomedTileSize = gameMap->getZoomedTileSize();
	const auto tileDrawingRange = gameMap->computeTileDrawingRange();
	const auto zoomedStartTilePixelOffset = gameMap->getZoomedStartTilePixelOffset();

	auto font = cUnicodeFont::font.get();

	for (auto i = makeIndexIterator (tileDrawingRange.first, tileDrawingRange.second); i.hasMore(); i.next())
	{
		auto drawDestination = gameMap->computeTileDrawingArea (zoomedTileSize, zoomedStartTilePixelOffset, tileDrawingRange.first, *i);
		drawDestination.x += zoomedTileSize.x() - font->getTextWide ("SLM");
		drawDestination.y += 4;

		for (const auto& player : client->getModel().getPlayerList())
		{
			std::string s;
			s += player->hasSeaDetection (*i)  ? "S" : " ";
			s += player->hasLandDetection (*i) ? "L" : " ";
			s += player->hasMineDetection (*i) ? "M" : " ";
			if (s != "   ")
			{
				font->showText (drawDestination.x, drawDestination.y, std::to_string (player->getId()) + s, FONT_LATIN_SMALL_YELLOW);
			}
			drawDestination.y += font->getFontHeight (FONT_LATIN_SMALL_YELLOW);
		}
	}
}
//------------------------------------------------------------------------------
void cDebugOutputWidget::drawSentryMaps()
{
	if (!gameMap || !client) return;

	const auto zoomedTileSize = gameMap->getZoomedTileSize();
	const auto tileDrawingRange = gameMap->computeTileDrawingRange();
	const auto zoomedStartTilePixelOffset = gameMap->getZoomedStartTilePixelOffset();

	auto font = cUnicodeFont::font.get();

	for (auto i = makeIndexIterator (tileDrawingRange.first, tileDrawingRange.second); i.hasMore(); i.next())
	{
		auto drawDestination = gameMap->computeTileDrawingArea (zoomedTileSize, zoomedStartTilePixelOffset, tileDrawingRange.first, *i);
		drawDestination.x += zoomedTileSize.x() - font->getTextWide ("GA");
		drawDestination.y += 8;

		for (const auto& player : client->getModel().getPlayerList())
		{
			std::string s;
			s += player->hasSentriesGround (*i)  ? "G" : " ";
			s += player->hasSentriesAir (*i) ? "A" : " ";
			if (s != "  ")
			{
				font->showText (drawDestination.x, drawDestination.y, std::to_string (player->getId()) + s, FONT_LATIN_SMALL_YELLOW);
			}
			drawDestination.y += font->getFontHeight (FONT_LATIN_SMALL_YELLOW);
		}
	}
}
