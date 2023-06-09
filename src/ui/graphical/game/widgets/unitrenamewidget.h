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

#ifndef ui_graphical_game_widgets_unitrenamewidgetH
#define ui_graphical_game_widgets_unitrenamewidgetH

#include "ui/widgets/widget.h"
#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"

#include <memory>

class cPosition;

template <typename>
class cBox;

class cUnit;

class cLabel;
class cLineEdit;
class cPlayer;
class cUnitsData;

class cUnitRenameWidget : public cWidget
{
public:
	cUnitRenameWidget (const cPosition&, int width);

	void setUnit (const cUnit*, const cUnitsData&);
	const cUnit* getUnit() const;

	void setPlayer (const cPlayer*, const cUnitsData&);

	const std::string& getUnitName() const;

	bool isAt (const cPosition&) const override;
	void retranslate() override;

	cSignal<void()> unitRenameTriggered;

private:
	cLabel* selectedUnitStatusLabel = nullptr;
	cLabel* selectedUnitNamePrefixLabel = nullptr;
	cLineEdit* selectedUnitNameEdit = nullptr;

	cSignalConnectionManager signalConnectionManager;
	cSignalConnectionManager unitSignalConnectionManager;

	const cUnit* activeUnit = nullptr;
	const cUnitsData* unitsData = nullptr;
	const cPlayer* player = nullptr;
};

#endif // ui_graphical_game_widgets_unitvideowidgetH
