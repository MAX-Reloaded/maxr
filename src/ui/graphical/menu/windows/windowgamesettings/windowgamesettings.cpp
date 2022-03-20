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

#include "ui/graphical/menu/windows/windowgamesettings/windowgamesettings.h"

#include "game/data/gamesettings.h"
#include "game/startup/lobbyclient.h"
#include "resources/pcx.h"
#include "ui/graphical/application.h"
#include "ui/graphical/menu/widgets/checkbox.h"
#include "ui/graphical/menu/widgets/label.h"
#include "ui/graphical/menu/widgets/lineedit.h"
#include "ui/graphical/menu/widgets/pushbutton.h"
#include "ui/graphical/menu/widgets/special/editablecheckbox.h"
#include "ui/graphical/menu/widgets/special/radiogroupvalue.h"
#include "ui/graphical/menu/widgets/tools/validatorint.h"
#include "ui/uidefines.h"
#include "utility/language.h"

namespace
{
	constexpr int unlimited = -2;
	constexpr int custom = -1;
}
//------------------------------------------------------------------------------
cWindowGameSettings::cWindowGameSettings (bool forHotSeatGame_) :
	cWindow (LoadPCX (GFXOD_OPTIONS)),
	forHotSeatGame (forHotSeatGame_)
{
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (0, 13), getPosition() + cPosition (getArea().getMaxCorner().x(), 23)), lngPack.i18n ("Text~Others~Game_Options"), eUnicodeFontType::LatinNormal, eAlignmentType::CenterHorizontal));

	int currentLine = 57;
	const int lineHeight = 16;

	// Resources
	auto addResourceRadioGroup = [&] (const std::string& resourceName) {
		addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)), resourceName + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
		auto* radioGroup = addChild (std::make_unique<cRadioGroupValue<eGameSettingsResourceAmount>>());
		radioGroup->emplaceCheckBox (eGameSettingsResourceAmount::Limited, getPosition() + cPosition (240, currentLine), lngPack.i18n ("Text~Option~Limited"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
		radioGroup->emplaceCheckBox (eGameSettingsResourceAmount::Normal, getPosition() + cPosition (240 + 86, currentLine), lngPack.i18n ("Text~Option~Normal"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
		radioGroup->emplaceCheckBox (eGameSettingsResourceAmount::High, getPosition() + cPosition (240 + 86 * 2, currentLine), lngPack.i18n ("Text~Option~High"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
		radioGroup->emplaceCheckBox (eGameSettingsResourceAmount::TooMuch, getPosition() + cPosition (240 + 86 * 3, currentLine), lngPack.i18n ("Text~Option~TooMuch"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
		currentLine += lineHeight;
		return radioGroup;
	};

	metalGroup = addResourceRadioGroup (lngPack.i18n ("Text~Title~Metal"));
	oilGroup = addResourceRadioGroup (lngPack.i18n ("Text~Title~Oil"));
	goldGroup = addResourceRadioGroup (lngPack.i18n ("Text~Title~Gold"));

	// Density
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)), lngPack.i18n ("Text~Title~Resource_Density") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	densityGroup = addChild (std::make_unique<cRadioGroupValue<eGameSettingsResourceDensity>>());

	densityGroup->emplaceCheckBox (eGameSettingsResourceDensity::Sparse, getPosition() + cPosition (240, currentLine), lngPack.i18n ("Text~Option~Sparse"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	densityGroup->emplaceCheckBox (eGameSettingsResourceDensity::Normal, getPosition() + cPosition (240 + 86, currentLine), lngPack.i18n ("Text~Option~Normal"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	densityGroup->emplaceCheckBox (eGameSettingsResourceDensity::Dense, getPosition() + cPosition (240 + 86 * 2, currentLine), lngPack.i18n ("Text~Option~Dense"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	densityGroup->emplaceCheckBox (eGameSettingsResourceDensity::TooMuch, getPosition() + cPosition (240 + 86 * 3, currentLine), lngPack.i18n ("Text~Option~TooMuch"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	currentLine += lineHeight * 2;

	// Bridgehead
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)), lngPack.i18n ("Text~Title~BridgeHead") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	bridgeheadGroup = addChild (std::make_unique<cRadioGroupValue<eGameSettingsBridgeheadType>>());
	bridgeheadGroup->emplaceCheckBox (eGameSettingsBridgeheadType::Mobile, getPosition() + cPosition (240, currentLine), lngPack.i18n ("Text~Option~Mobile"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	bridgeheadGroup->emplaceCheckBox (eGameSettingsBridgeheadType::Definite, getPosition() + cPosition (240 + 173, currentLine), lngPack.i18n ("Text~Option~Definite"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	currentLine += lineHeight;

	// alien
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)), lngPack.i18n ("Text~Title~Alien_Tech") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	alienGroup = addChild (std::make_unique<cRadioGroupValue<bool>>());
	alienGroup->emplaceCheckBox (false, getPosition() + cPosition (240, currentLine), lngPack.i18n ("Text~Option~Off"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	alienGroup->emplaceCheckBox (true, getPosition() + cPosition (240 + 173, currentLine), lngPack.i18n ("Text~Option~On"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	currentLine += lineHeight;

	//
	// Game type
	//
	if (!forHotSeatGame)
	{
		addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)), lngPack.i18n ("Text~Title~Game_Type") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	}
	gameTypeGroup = addChild (std::make_unique<cRadioGroupValue<eGameSettingsGameType>>());
	auto* gameTypeTurnsCheckBox = gameTypeGroup->emplaceCheckBox (eGameSettingsGameType::Turns, getPosition() + cPosition (240, currentLine), lngPack.i18n ("Text~Option~Type_Turns"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	gameTypeGroup->emplaceCheckBox (eGameSettingsGameType::Simultaneous, getPosition() + cPosition (240 + 173, currentLine), lngPack.i18n ("Text~Option~Type_Simu"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	signalConnectionManager.connect (gameTypeTurnsCheckBox->toggled, [this, gameTypeTurnsCheckBox]()
	{
		if (gameTypeTurnsCheckBox->isChecked()) disableTurnEndDeadlineOptions();
		else enableTurnEndDeadlineOptions();
	});
	if (forHotSeatGame)
	{
		gameTypeGroup->disable();
		gameTypeGroup->hide();
	}
	currentLine += lineHeight * 2;

	// Clans
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)), lngPack.i18n ("Text~Title~Clans") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	clansGroup = addChild (std::make_unique<cRadioGroupValue<bool>>());
	clansGroup->emplaceCheckBox (true, getPosition() + cPosition (240, currentLine), lngPack.i18n ("Text~Option~On"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	clansGroup->emplaceCheckBox (false, getPosition() + cPosition (240 + 64, currentLine), lngPack.i18n ("Text~Option~Off"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	currentLine += lineHeight * 2;

	auto savedLine = currentLine;

	// Credits
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)), lngPack.i18n ("Text~Title~Credits_start") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	currentLine += lineHeight;
	creditsGroup = addChild (std::make_unique<cRadioGroupValue<int>>());

	creditsGroup->emplaceCheckBox (cGameSettings::defaultCreditsNone, getPosition() + cPosition (140, currentLine), lngPack.i18n ("Text~Option~None") + " (" + std::to_string (cGameSettings::defaultCreditsNone) + ")", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
	currentLine += lineHeight;
	creditsGroup->emplaceCheckBox (cGameSettings::defaultCreditsLow, getPosition() + cPosition (140, currentLine), lngPack.i18n ("Text~Option~Low") + " (" + std::to_string (cGameSettings::defaultCreditsLow) + ")", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
	currentLine += lineHeight;
	creditsGroup->emplaceCheckBox (cGameSettings::defaultCreditsLimited, getPosition() + cPosition (140, currentLine), lngPack.i18n ("Text~Option~Limited") + " (" + std::to_string (cGameSettings::defaultCreditsLimited) + ")", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
	currentLine += lineHeight;
	creditsGroup->emplaceCheckBox (cGameSettings::defaultCreditsNormal, getPosition() + cPosition (140, currentLine), lngPack.i18n ("Text~Option~Normal") + " (" + std::to_string (cGameSettings::defaultCreditsNormal) + ")", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
	currentLine += lineHeight;
	creditsGroup->emplaceCheckBox (cGameSettings::defaultCreditsHigh, getPosition() + cPosition (140, currentLine), lngPack.i18n ("Text~Option~High") + " (" + std::to_string (cGameSettings::defaultCreditsHigh) + ")", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
	currentLine += lineHeight;
	creditsGroup->emplaceCheckBox (cGameSettings::defaultCreditsMore, getPosition() + cPosition (140, currentLine), lngPack.i18n ("Text~Option~More") + " (" + std::to_string (cGameSettings::defaultCreditsMore) + ")", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
	currentLine += lineHeight * 2;

	auto savedLine2 = currentLine;

	//
	// Victory conditions
	//
	currentLine = savedLine;
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (300, currentLine), getPosition() + cPosition (400, currentLine + 10)), lngPack.i18n ("Text~Comp~GameEndsAt") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	victoryGroup = addChild (std::make_unique<cRadioGroupValue<std::pair<eGameSettingsVictoryCondition, int>>>());
	currentLine += lineHeight;

	savedLine = currentLine;
	for (auto turn : cGameSettings::defaultVictoryTurnsOptions)
	{
		victoryGroup->emplaceCheckBox ({eGameSettingsVictoryCondition::Turns, turn}, getPosition() + cPosition (380, currentLine), std::to_string (turn) + " " + lngPack.i18n ("Text~Comp~Turns"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
		currentLine += lineHeight;
	}
	const auto customTurnArea = cBox<cPosition> (getPosition() + cPosition (330, currentLine), getPosition() + cPosition (420, currentLine + 10));
	customVictoryTurnsCheckBox = victoryGroup->addCheckBox ({eGameSettingsVictoryCondition::Turns, custom}, std::make_unique<cEditableCheckBox> (customTurnArea, lngPack.i18n ("Text~Comp~Turns") + lngPack.i18n ("Text~Punctuation~Colon"), "", eUnicodeFontType::LatinNormal));
	customVictoryTurnsCheckBox->setText ("420");
	customVictoryTurnsCheckBox->setValidator (std::make_unique<cValidatorInt> (0, 9999));

	currentLine = savedLine;
	for (auto point : cGameSettings::defaultVictoryPointsOptions)
	{
		victoryGroup->emplaceCheckBox ({eGameSettingsVictoryCondition::Points, point}, getPosition() + cPosition (500, currentLine), std::to_string (point) + " " + lngPack.i18n ("Text~Comp~Points"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
		currentLine += lineHeight;
	}
	const auto customPointArea = cBox<cPosition> (getPosition() + cPosition (440, currentLine), getPosition() + cPosition (540, currentLine + 10));
	customVictoryPointsCheckBox = victoryGroup->addCheckBox ({eGameSettingsVictoryCondition::Points, custom}, std::make_unique<cEditableCheckBox> (customPointArea, lngPack.i18n ("Text~Comp~Points") + lngPack.i18n ("Text~Punctuation~Colon"), "", eUnicodeFontType::LatinNormal));
	customVictoryPointsCheckBox->setText ("900");
	customVictoryPointsCheckBox->setValidator (std::make_unique<cValidatorInt> (0, 9999));
	currentLine += lineHeight;

	victoryGroup->emplaceCheckBox ({eGameSettingsVictoryCondition::Death, 0}, getPosition() + cPosition (440, currentLine), lngPack.i18n ("Text~Comp~NoLimit"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly, true);
	currentLine += lineHeight;

	// Turn Limit
	currentLine = savedLine2;
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)), lngPack.i18n ("Text~Title~Turn_limit") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	turnLimitGroup = addChild (std::make_unique<cRadioGroupValue<int>>());
	turnLimitGroup->emplaceCheckBox (unlimited, getPosition() + cPosition (240, currentLine), lngPack.i18n ("Text~Settings~Unlimited_11"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	turnLimitGroup->emplaceCheckBox (cGameSettings::defaultTurnLimitOption0.count(), getPosition() + cPosition (240 + 85         , currentLine), std::to_string (cGameSettings::defaultTurnLimitOption0.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	turnLimitGroup->emplaceCheckBox (cGameSettings::defaultTurnLimitOption1.count(), getPosition() + cPosition (240 + 85 + 40    , currentLine), std::to_string (cGameSettings::defaultTurnLimitOption1.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	turnLimitGroup->emplaceCheckBox (cGameSettings::defaultTurnLimitOption2.count(), getPosition() + cPosition (240 + 85 + 40 * 2, currentLine), std::to_string (cGameSettings::defaultTurnLimitOption2.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	turnLimitGroup->emplaceCheckBox (cGameSettings::defaultTurnLimitOption3.count(), getPosition() + cPosition (240 + 85 + 40 * 3, currentLine), std::to_string (cGameSettings::defaultTurnLimitOption3.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	turnLimitGroup->emplaceCheckBox (cGameSettings::defaultTurnLimitOption4.count(), getPosition() + cPosition (240 + 85 + 40 * 4, currentLine), std::to_string (cGameSettings::defaultTurnLimitOption4.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	turnLimitGroup->emplaceCheckBox (cGameSettings::defaultTurnLimitOption5.count(), getPosition() + cPosition (240 + 85 + 40 * 5, currentLine), std::to_string (cGameSettings::defaultTurnLimitOption5.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	currentLine += lineHeight;

	customTurnLimitCheckBox = turnLimitGroup->addCheckBox (custom, std::make_unique<cEditableCheckBox> (cBox<cPosition> (getPosition() + cPosition (240, currentLine), cPosition (240 + 85 + 30, currentLine + 10)), lngPack.i18n ("Text~Title~Custom_11") + lngPack.i18n ("Text~Punctuation~Colon"), "s", eUnicodeFontType::LatinNormal));
	customTurnLimitCheckBox->setText ("410");
	customTurnLimitCheckBox->setValidator (std::make_unique<cValidatorInt> (0, 9999));

	currentLine += lineHeight;

	// Turn End Deadline
	turnEndDeadlineLabel = addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (64, currentLine), getPosition() + cPosition (230, currentLine + 10)),  lngPack.i18n ("Text~Title~Turn_end") + lngPack.i18n ("Text~Punctuation~Colon"), eUnicodeFontType::LatinNormal, eAlignmentType::Left));
	endTurnDeadlineGroup = addChild (std::make_unique<cRadioGroupValue<int>>());
	endTurnDeadlineGroup->emplaceCheckBox (unlimited, getPosition() + cPosition (240, currentLine), lngPack.i18n ("Text~Settings~Unlimited_11"), eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	endTurnDeadlineGroup->emplaceCheckBox (cGameSettings::defaultEndTurnDeadlineOption0.count(), getPosition() + cPosition (240 + 85         , currentLine), std::to_string (cGameSettings::defaultEndTurnDeadlineOption0.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	endTurnDeadlineGroup->emplaceCheckBox (cGameSettings::defaultEndTurnDeadlineOption1.count(), getPosition() + cPosition (240 + 85 + 40    , currentLine), std::to_string (cGameSettings::defaultEndTurnDeadlineOption1.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	endTurnDeadlineGroup->emplaceCheckBox (cGameSettings::defaultEndTurnDeadlineOption2.count(), getPosition() + cPosition (240 + 85 + 40 * 2, currentLine), std::to_string (cGameSettings::defaultEndTurnDeadlineOption2.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	endTurnDeadlineGroup->emplaceCheckBox (cGameSettings::defaultEndTurnDeadlineOption3.count(), getPosition() + cPosition (240 + 85 + 40 * 3, currentLine), std::to_string (cGameSettings::defaultEndTurnDeadlineOption3.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	endTurnDeadlineGroup->emplaceCheckBox (cGameSettings::defaultEndTurnDeadlineOption4.count(), getPosition() + cPosition (240 + 85 + 40 * 4, currentLine), std::to_string (cGameSettings::defaultEndTurnDeadlineOption4.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	endTurnDeadlineGroup->emplaceCheckBox (cGameSettings::defaultEndTurnDeadlineOption5.count(), getPosition() + cPosition (240 + 85 + 40 * 5, currentLine), std::to_string (cGameSettings::defaultEndTurnDeadlineOption5.count()) + "s", eUnicodeFontType::LatinNormal, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	currentLine += lineHeight;

	customEndTurnDeadlineCheckBox = endTurnDeadlineGroup->addCheckBox (custom, std::make_unique<cEditableCheckBox> (cBox<cPosition> (getPosition() + cPosition (240, currentLine), cPosition (240 + 85 + 30, currentLine + 10)), lngPack.i18n ("Text~Title~Custom_11") + lngPack.i18n ("Text~Punctuation~Colon"), "s", eUnicodeFontType::LatinNormal));
	customEndTurnDeadlineCheckBox->setText ("105");
	customEndTurnDeadlineCheckBox->setValidator (std::make_unique<cValidatorInt> (0, 9999));

	currentLine += lineHeight;

	//
	// Buttons
	//
	auto okButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (390, 440), ePushButtonType::StandardBig, lngPack.i18n ("Text~Others~OK")));
	signalConnectionManager.connect (okButton->clicked, [this]() { okClicked(); });

	auto backButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (50, 440), ePushButtonType::StandardBig, lngPack.i18n ("Text~Others~Back")));
	signalConnectionManager.connect (backButton->clicked, [this]() { backClicked(); });
}


//------------------------------------------------------------------------------
void cWindowGameSettings::initFor (cLobbyClient& lobbyClient)
{
	const auto* gameSettings = lobbyClient.getLobbyPreparationData().gameSettings.get();
	if (gameSettings) applySettings (*gameSettings);
	else applySettings (cGameSettings());

	signalConnectionManager.connect (done, [this, &lobbyClient](const cGameSettings& gameSettings)
	{
		lobbyClient.selectGameSettings (gameSettings);
		close();
	});
}

//------------------------------------------------------------------------------
void cWindowGameSettings::applySettings (const cGameSettings& gameSettings)
{
	metalGroup->selectValue (gameSettings.metalAmount);
	oilGroup->selectValue (gameSettings.oilAmount);
	goldGroup->selectValue (gameSettings.goldAmount);

	densityGroup->selectValue (gameSettings.resourceDensity);

	bridgeheadGroup->selectValue (gameSettings.bridgeheadType);
	alienGroup->selectValue (gameSettings.alienEnabled);
	gameTypeGroup->selectValue (forHotSeatGame ? eGameSettingsGameType::Turns : gameSettings.gameType);
	if (*gameTypeGroup->getSelectedValue() == eGameSettingsGameType::Simultaneous) enableTurnEndDeadlineOptions();
	else disableTurnEndDeadlineOptions();

	clansGroup->selectValue (gameSettings.clansEnabled);

	creditsGroup->selectValue (gameSettings.startCredits);

	switch (gameSettings.victoryConditionType)
	{
		case eGameSettingsVictoryCondition::Turns:
			if (!victoryGroup->selectValue ({eGameSettingsVictoryCondition::Turns, gameSettings.victoryTurns}))
			{
				customVictoryTurnsCheckBox->setText (std::to_string (gameSettings.victoryTurns));
				victoryGroup->selectValue ({eGameSettingsVictoryCondition::Turns, custom});
			}
			break;
		case eGameSettingsVictoryCondition::Points:
			if (!victoryGroup->selectValue ({eGameSettingsVictoryCondition::Points, gameSettings.victoryPoints}))
			{
				customVictoryPointsCheckBox->setText (std::to_string (gameSettings.victoryPoints));
				victoryGroup->selectValue ({eGameSettingsVictoryCondition::Points, custom});
			}
			break;
		case eGameSettingsVictoryCondition::Death:
			victoryGroup->selectValue ({eGameSettingsVictoryCondition::Death, 0});
			break;
	}

	if (gameSettings.turnLimitActive)
	{
		if (!turnLimitGroup->selectValue (gameSettings.turnLimit.count()))
		{
			turnLimitGroup->selectValue (custom);
			customTurnLimitCheckBox->setText (std::to_string (gameSettings.turnLimit.count()));
		}
	}
	else
	{
		turnLimitGroup->selectValue (unlimited);
	}

	if (gameSettings.turnEndDeadlineActive)
	{
		if (!endTurnDeadlineGroup->selectValue (gameSettings.turnEndDeadline.count()))
		{
			endTurnDeadlineGroup->selectValue (custom);
			customEndTurnDeadlineCheckBox->setText (std::to_string (gameSettings.turnEndDeadline.count()));
		}
	}
	else
	{
		endTurnDeadlineGroup->selectValue (unlimited);
	}
}

//------------------------------------------------------------------------------
cGameSettings cWindowGameSettings::getGameSettings() const
{
	cGameSettings gameSettings;

	gameSettings.metalAmount = *metalGroup->getSelectedValue();
	gameSettings.oilAmount = *oilGroup->getSelectedValue();
	gameSettings.goldAmount = *goldGroup->getSelectedValue();
	gameSettings.resourceDensity = *densityGroup->getSelectedValue();

	gameSettings.bridgeheadType = *bridgeheadGroup->getSelectedValue();
	gameSettings.alienEnabled = *alienGroup->getSelectedValue();

	gameSettings.gameType = forHotSeatGame ? eGameSettingsGameType::HotSeat : *gameTypeGroup->getSelectedValue();

	gameSettings.clansEnabled = *clansGroup->getSelectedValue();

	gameSettings.startCredits = *creditsGroup->getSelectedValue();

	const auto victoryCondition = *victoryGroup->getSelectedValue();
	const auto victoryType = victoryCondition.first;
	const auto victoryCount = victoryCondition.second;
	gameSettings.victoryConditionType = victoryType;
	switch (victoryType)
	{
		case eGameSettingsVictoryCondition::Points:
		{
			gameSettings.victoryPoints = (victoryCount == custom ? stoi (customVictoryPointsCheckBox->getText()): victoryCount);
			break;
		}
		case eGameSettingsVictoryCondition::Turns:
		{
			gameSettings.victoryTurns = (victoryCount == custom ? stoi (customVictoryTurnsCheckBox->getText()) : victoryCount);
			break;
		}
		case eGameSettingsVictoryCondition::Death: break;
	}

	gameSettings.turnLimitActive = true;
	switch (*turnLimitGroup->getSelectedValue())
	{
		case custom: gameSettings.turnLimit = std::chrono::seconds (atoi (customTurnLimitCheckBox->getText().c_str())); break;
		case unlimited: gameSettings.turnLimitActive = false; break;
		default: gameSettings.turnLimit = std::chrono::seconds (*turnLimitGroup->getSelectedValue()); break;
	}

	gameSettings.turnEndDeadlineActive = true;
	switch (*endTurnDeadlineGroup->getSelectedValue())
	{
		case custom: gameSettings.turnEndDeadline = std::chrono::seconds (atoi (customEndTurnDeadlineCheckBox->getText().c_str())); break;
		case unlimited: gameSettings.turnEndDeadlineActive = false; break;
		default: gameSettings.turnEndDeadline = std::chrono::seconds (*endTurnDeadlineGroup->getSelectedValue()); break;
	}

	return gameSettings;
}

//------------------------------------------------------------------------------
void cWindowGameSettings::okClicked()
{
	done (getGameSettings());
}

//------------------------------------------------------------------------------
void cWindowGameSettings::backClicked()
{
	close();
}

//------------------------------------------------------------------------------
void cWindowGameSettings::disableTurnEndDeadlineOptions()
{
	endTurnDeadlineGroup->disable();
	endTurnDeadlineGroup->hide();

	turnEndDeadlineLabel->hide();
}

//------------------------------------------------------------------------------
void cWindowGameSettings::enableTurnEndDeadlineOptions()
{
	if (forHotSeatGame) { return; }

	endTurnDeadlineGroup->enable();
	endTurnDeadlineGroup->show();

	turnEndDeadlineLabel->show();
}
