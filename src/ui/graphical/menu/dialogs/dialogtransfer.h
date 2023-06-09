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

#ifndef ui_graphical_menu_dialogs_dialogtransferH
#define ui_graphical_menu_dialogs_dialogtransferH

#include "game/data/units/unit.h"
#include "ui/graphical/menu/widgets/special/resourcebar.h"
#include "ui/widgets/window.h"
#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"

class cImage;
class cLabel;
class cPushButton;
class cResourceBar;
class cUnit;

class cNewDialogTransfer : public cWindow
{
public:
	cNewDialogTransfer (const cUnit& sourceUnit, const cUnit& destinationUnit);

	void retranslate() override;

	int getTransferValue() const;

	eResourceType getResourceType() const;

	cSignal<void()> done;

private:
	cSignalConnectionManager signalConnectionManager;

	eResourceType getCommonResourceType (const cUnit& sourceUnit, const cUnit& destinationUnit) const;
	eResourceBarType getResourceBarType (const cUnit& sourceUnit, const cUnit& destinationUnit) const;

	void initUnitImage (cImage& image, const cUnit& unit);
	void initCargo (int& cargo, int& maxCargo, const cUnit& sourceUnit, const cUnit& destinationUnit);

	cPushButton* doneButton = nullptr;
	cPushButton* cancelButton = nullptr;

	cResourceBar* resourceBar = nullptr;

	cLabel* transferLabel = nullptr;

	cImage* arrowImage = nullptr;

	cLabel* sourceUnitCargoLabel = nullptr;
	cLabel* destinationUnitCargoLabel = nullptr;

	const eResourceType resourceType;

	int sourceCargo;
	int sourceMaxCargo;

	int destinationCargo;
	int destinationMaxCargo;

	void transferValueChanged();

	void closeOnUnitDestruction();
};

#endif // ui_graphical_menu_dialogs_dialogtransferH
