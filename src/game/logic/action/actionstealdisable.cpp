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


#include "game/data/model.h"

#include "actionstealdisable.h"
#include "actionstop.h"
#include "game/logic/casualtiestracker.h"

cActionStealDisable::cActionStealDisable(const cVehicle& infiltrator, const cUnit& target, bool steal) :
	infiltratorId(infiltrator.getId()),
	targetId(target.getId()),
	steal(steal)
{}

//------------------------------------------------------------------------------
cActionStealDisable::cActionStealDisable(cBinaryArchiveOut& archive)
{
	serializeThis(archive);
}

//------------------------------------------------------------------------------
void cActionStealDisable::execute(cModel& model) const
{
	//Note: this function handles incoming data from network. Make every possible sanity check!

	auto infiltrator = model.getVehicleFromID(infiltratorId);
	if (infiltrator == nullptr) return;
	if (infiltrator->getOwner()->getId() != playerNr) return;

	auto target = model.getUnitFromID(targetId);
	if (target == nullptr) return;

	if (!cCommandoData::canDoAction(*infiltrator, target, steal)) return;

	infiltrator->data.setShots(infiltrator->data.getShots() - 1);

	// check whether the action is successful or not
	const uint32_t chance = infiltrator->getCommandoData().computeChance(target, steal);
	bool success = model.randomGenerator.get(100) < chance;
	if (success)
	{

		// stop running movejobs, build orders, etc.
		cActionStop(*target).execute(model);

		if (steal)
		{
			const auto& previousOwner = *target->getOwner();
			changeUnitOwner(*target, *infiltrator->getOwner(), model);
			model.unitStolen(*infiltrator, *target, previousOwner);
		}
		else
		{
			// Only on disabling units the infiltrator gets exp.
			cCommandoData::increaseXp (*infiltrator);

			const int turns = infiltrator->getCommandoData().computeDisabledTurnCount(*target);

			target->setDisabledTurns (turns);
			target->getOwner()->removeFromScan(*target);

			model.unitDisabled(*infiltrator, *target);
		}
	}
	else
	{
		model.unitStealDisableFailed(*infiltrator, *target);

		// disabled units fail to detect infiltrator even if he screws up
		if (!target->isDisabled())
		{
			// detect the infiltrator on failed action
			// and let enemy units fire on him
			if (target->getOwner()->canSeeAnyAreaUnder(*infiltrator))
			{
				infiltrator->setDetectedByPlayer(target->getOwner());
			}

			infiltrator->inSentryRange(model);
		}
	}
}

void cActionStealDisable::changeUnitOwner(cUnit& unit, cPlayer& newOwner, cModel& model) const
{

	model.getCasualtiesTracker()->logCasualty(unit);

	cPlayer* oldOwner = unit.getOwner();
	if (!unit.isDisabled())
	{
		oldOwner->removeFromScan(unit);
	}

	// unit is fully operational for new owner
	unit.setDisabledTurns(0);

	if (unit.isAVehicle())
	{
		auto owningUnitPtr = oldOwner->removeUnit(*static_cast<const cVehicle*>(&unit));
		owningUnitPtr->setOwner(&newOwner);
		newOwner.addUnit(owningUnitPtr);
		owningUnitPtr->setSurveyorAutoMoveActive(false);
	}
	else
	{
		auto owningUnitPtr = oldOwner->removeUnit(*static_cast<const cBuilding*>(&unit));
		owningUnitPtr->setOwner(&newOwner);
		newOwner.addUnit(owningUnitPtr);
	}

	newOwner.addToScan(unit);

	for (const auto& player : model.getPlayerList())
	{
		unit.resetDetectedByPlayer(player.get());
	}
	unit.clearDetectedInThisTurnPlayerList();

	// let the unit work for his new owner
	auto* vehicle = static_cast<cVehicle*>(&unit);
	if (vehicle && vehicle->getStaticData().canSurvey)
	{
		vehicle->doSurvey(*model.getMap());
	}
	unit.detectOtherUnits(*model.getMap());
}
