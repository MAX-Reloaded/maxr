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

#include "ui/graphical/game/widgets/unitcontextmenuwidget.h"

#include "game/data/map/mapfieldview.h"
#include "game/data/map/mapview.h"
#include "game/data/units/building.h"
#include "game/data/units/unit.h"
#include "game/data/units/vehicle.h"
#include "ui/graphical/menu/widgets/checkbox.h"
#include "ui/graphical/menu/widgets/pushbutton.h"
#include "ui/graphical/menu/widgets/radiogroup.h"
#include "utility/language.h"

//------------------------------------------------------------------------------
cUnitContextMenuWidget::cUnitContextMenuWidget() :
	unit (nullptr)
{}

//------------------------------------------------------------------------------
void cUnitContextMenuWidget::setUnit (const cUnit* unit_, eMouseModeType mouseInputMode, const cPlayer* player, const cMapView* map)
{
	unit = unit_;
	removeChildren();

	if (unit == nullptr) return;

	cBox<cPosition> area (getPosition(), getPosition());
	cPosition nextButtonPosition = getPosition();
	auto mouseActionGroup = addChild (std::make_unique<cRadioGroup> (true));

	// Attack
	if (unitHasAttackEntry (unit, player))
	{
		auto button = mouseActionGroup->addButton (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Attack_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (mouseInputMode == eMouseModeType::Attack);
		button->toggled.connect ([&]() { attackToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Build:
	if (unitHasBuildEntry (unit, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Build_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { buildClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	const cBuilding* building = dynamic_cast<const cBuilding*> (unit);
	// Distribute:
	if (unitHasDistributeEntry (building, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Distribution_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { distributeClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Transfer:
	if (unitHasTransferEntry (unit, player))
	{
		auto button = mouseActionGroup->addButton (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Transfer_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (mouseInputMode == eMouseModeType::Transfer);
		button->toggled.connect ([&]() { transferToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Start:
	if (unitHasStartEntry (building, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Start_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { startClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Auto survey movejob of surveyor
	const cVehicle* vehicle = dynamic_cast<const cVehicle*> (unit);
	if (unitHasAutoEntry (vehicle, player))
	{
		auto button = addChild (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Auto_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (vehicle->isSurveyorAutoMoveActive());
		button->toggled.connect ([&]() { autoToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Stop:
	if (unitHasStopEntry (unit, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Stop_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { stopClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Remove:
	if (unitHasRemoveEntry (vehicle, player, map))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Clear_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { removeClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Manual fire
	if (unitHasManualFireEntry (unit, player))
	{
		auto button = addChild (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~ManualFireMode_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (unit->isManualFireActive());
		button->toggled.connect ([&]() { manualFireToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Sentry status:
	if (unitHasSentryEntry (unit, player))
	{
		auto button = addChild (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Sentry"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (unit->isSentryActive());
		button->toggled.connect ([&]() { sentryToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Activate
	if (unitHasActivateEntry (unit, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Active_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { activateClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Load
	if (unitHasLoadEntry (unit, player))
	{
		auto button = mouseActionGroup->addButton (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Load_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (mouseInputMode == eMouseModeType::Load);
		button->toggled.connect ([&]() { loadToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Enter
	if (unitHasEnterEntry (vehicle, player))
	{
		auto button = mouseActionGroup->addButton (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Enter_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (mouseInputMode == eMouseModeType::Enter);
		button->toggled.connect ([&]() { enterToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// research
	if (unitHasResearchEntry (building, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Research"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { researchClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// gold upgrades screen
	if (unitHasBuyEntry (building, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Upgrademenu_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { buyUpgradesClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Update this
	if (unitHasUpgradeThisEntry (building, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Upgradethis_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { upgradeThisClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Update all
	if (unitHasUpgradeAllEntry (building, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~UpgradeAll_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { upgradeAllClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Self destruct
	if (unitHasSelfDestroyEntry (building, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Destroy_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { selfDestroyClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Ammo:
	if (unitHasSupplyEntry (vehicle, player))
	{
		auto button = mouseActionGroup->addButton (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Reload_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (mouseInputMode == eMouseModeType::SupplyAmmo);
		button->toggled.connect ([&]() { supplyAmmoToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Repair:
	if (unitHasRepairEntry (vehicle, player))
	{
		auto button = mouseActionGroup->addButton (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Repair_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (mouseInputMode == eMouseModeType::Repair);
		button->toggled.connect ([&]() { repairToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Lay mines:
	if (unitHasLayMinesEntry (vehicle, player))
	{
		auto button = addChild (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Seed"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (vehicle->isUnitLayingMines());
		button->toggled.connect ([&]() { layMinesToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Collect/clear mines:
	if (unitHasCollectMinesEntry (vehicle, player))
	{
		auto button = addChild (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Clear_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (vehicle->isUnitClearingMines());
		button->toggled.connect ([&]() { collectMinesToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Sabotage/disable:
	if (unitHasSabotageEntry (vehicle, player))
	{
		auto button = mouseActionGroup->addButton (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Disable_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (mouseInputMode == eMouseModeType::Disable);
		button->toggled.connect ([&]() { sabotageToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Steal:
	if (unitHasStealEntry (vehicle, player))
	{
		auto button = mouseActionGroup->addButton (std::make_unique<cCheckBox> (nextButtonPosition, lngPack.i18n ("Text~Others~Steal_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Right, eCheckBoxType::UnitContextMenu, false, &SoundData.SNDObjectMenu));
		button->setChecked (mouseInputMode == eMouseModeType::Steal);
		button->toggled.connect ([&]() { stealToggled(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Info
	if (unitHasInfoEntry (unit, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Info_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { infoClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	// Done
	if (unitHasDoneEntry (unit, player))
	{
		auto button = addChild (std::make_unique<cPushButton> (nextButtonPosition, ePushButtonType::UnitContextMenu, &SoundData.SNDObjectMenu, lngPack.i18n ("Text~Others~Done_7"), FONT_LATIN_SMALL_WHITE));
		button->clicked.connect ([&]() { doneClicked(); });
		nextButtonPosition.y() += button->getSize().y();
		area.add (button->getArea());
	}

	resize (area.getSize());
}

//------------------------------------------------------------------------------
const cUnit* cUnitContextMenuWidget::getUnit()
{
	return unit;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasAttackEntry (const cUnit* unit, const cPlayer* player)
{
	return unit && !unit->isDisabled() && unit->getOwner() == player && unit->getStaticUnitData().canAttack && unit->data.getShots();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasBuildEntry (const cUnit* unit, const cPlayer* player)
{
	const auto* vehicle = dynamic_cast<const cVehicle*> (unit);
	return unit && !unit->isDisabled() && unit->getOwner() == player && !unit->getStaticUnitData().canBuild.empty() && (!vehicle || !vehicle->isUnitBuildingABuilding());
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasDistributeEntry (const cBuilding* building, const cPlayer* player)
{
	return building && !building->isDisabled() && building->getOwner() == player && building->getStaticData().canMineMaxRes > 0 && building->isUnitWorking();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasTransferEntry (const cUnit* unit, const cPlayer* player)
{
	const auto* vehicle = dynamic_cast<const cVehicle*> (unit);
	return unit && !unit->isDisabled() && unit->getOwner() == player && unit->getStaticUnitData().storeResType != eResourceType::None && (!vehicle || !vehicle->isUnitBuildingABuilding()) && (!vehicle || !vehicle->isUnitClearing());
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasStartEntry (const cBuilding* building, const cPlayer* player)
{
	return building && !building->isDisabled() && building->getOwner() == player && building->buildingCanBeStarted();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasAutoEntry (const cVehicle* vehicle, const cPlayer* player)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && vehicle->getStaticData().canSurvey;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasStopEntry (const cUnit* unit, const cPlayer* player)
{
	return unit && !unit->isDisabled() && unit->getOwner() == player && unit->canBeStoppedViaUnitMenu();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasRemoveEntry (const cVehicle* vehicle, const cPlayer* player, const cMapView* map)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && vehicle->getStaticData().canClearArea && map && map->getField (vehicle->getPosition()).getRubble() && !vehicle->isUnitClearing();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasManualFireEntry (const cUnit* unit, const cPlayer* player)
{
	return unit && !unit->isDisabled() && unit->getOwner() == player && (unit->isManualFireActive() || unit->getStaticUnitData().canAttack);
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasSentryEntry (const cUnit* unit, const cPlayer* player)
{
	return unit && !unit->isDisabled() && unit->getOwner() == player && (unit->isSentryActive() || unit->getStaticUnitData().canAttack || (!unit->isABuilding() && !unit->canBeStoppedViaUnitMenu()));
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasActivateEntry (const cUnit* unit, const cPlayer* player)
{
	return unit && !unit->isDisabled() && unit->getOwner() == player && unit->getStaticUnitData().storageUnitsMax > 0;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasLoadEntry (const cUnit* unit, const cPlayer* player)
{
	return unit && !unit->isDisabled() && unit->getOwner() == player && unit->getStaticUnitData().storageUnitsMax > 0;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasEnterEntry (const cVehicle* vehicle, const cPlayer* player)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && !vehicle->isUnitClearing() && !vehicle->isUnitBuildingABuilding();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasResearchEntry (const cBuilding* building, const cPlayer* player)
{
	return building && !building->isDisabled() && building->getOwner() == player && building->getStaticData().canResearch && building->isUnitWorking();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasBuyEntry (const cBuilding* building, const cPlayer* player)
{
	return building && !building->isDisabled() && building->getOwner() == player && building->getStaticData().convertsGold;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasUpgradeThisEntry (const cBuilding* building, const cPlayer* player)
{
	return building && !building->isDisabled() && building->getOwner() == player && building->buildingCanBeUpgraded();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasUpgradeAllEntry (const cBuilding* building, const cPlayer* player)
{
	return building && !building->isDisabled() && building->getOwner() == player && building->buildingCanBeUpgraded();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasSelfDestroyEntry (const cBuilding* building, const cPlayer* player)
{
	return building && !building->isDisabled() && building->getOwner() == player && building->getStaticData().canSelfDestroy;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasSupplyEntry (const cVehicle* vehicle, const cPlayer* player)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && vehicle->getStaticUnitData().canRearm && vehicle->getStoredResources() > 0;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasRepairEntry (const cVehicle* vehicle, const cPlayer* player)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && vehicle->getStaticUnitData().canRepair && vehicle->getStoredResources() > 0;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasLayMinesEntry (const cVehicle* vehicle, const cPlayer* player)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && vehicle->getStaticData().canPlaceMines && vehicle->getStoredResources() > 0;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasCollectMinesEntry (const cVehicle* vehicle, const cPlayer* player)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && vehicle->getStaticData().canPlaceMines && vehicle->getStoredResources() < vehicle->getStaticUnitData().storageResMax;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasSabotageEntry (const cVehicle* vehicle, const cPlayer* player)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && vehicle->getStaticData().canDisable && vehicle->data.getShots();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasStealEntry (const cVehicle* vehicle, const cPlayer* player)
{
	return vehicle && !vehicle->isDisabled() && vehicle->getOwner() == player && vehicle->getStaticData().canCapture && vehicle->data.getShots();
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasInfoEntry (const cUnit* unit, const cPlayer* player)
{
	return unit != nullptr;
}

//------------------------------------------------------------------------------
/*static*/ bool cUnitContextMenuWidget::unitHasDoneEntry (const cUnit* unit, const cPlayer* player)
{
	return unit != nullptr;
}
