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

#ifndef gui_menu_windows_windowhangar_windowhangarH
#define gui_menu_windows_windowhangar_windowhangarH

#include "../../../window.h"
#include "../../../../utility/signal/signalconnectionmanager.h"
#include "../../../../utility/signal/signal.h"

class cImage;
class cLabel;
class cPlayer;
class cCheckBox;
class cUnitDetails;
template<typename T>
class cListView;
class cUnitListViewItemBuy;

class cUnitUpgrade;
struct sID;

class cWindowHangar : public cWindow
{
public:
	explicit cWindowHangar (SDL_Surface* surface, cPlayer& owner);
	~cWindowHangar ();

	cSignal<void ()> done;

protected:
	void addSelectionUnit (const sID& unitId);
	void clearSelectionUnits ();

	cPlayer& getPlayer () const;

	void setActiveUpgrades (const cUnitUpgrade& unitUpgrades);

	virtual void setActiveUnit (const sID& unitId);

	const sID* getActiveUnit ();

	cSignal<void (const cUnitListViewItemBuy&)> selectionUnitClickedSecondTime;

	cSignal<void (cUnitListViewItemBuy*)> selectionUnitSelectionChanged;
private:
	cSignalConnectionManager signalConnectionManager;

	cPlayer& owner;

	cImage* infoImage;
	cLabel* infoLabel;

	cUnitDetails* unitDetails;

	cListView<cUnitListViewItemBuy>* selectionUnitList;

	cCheckBox* infoTextCheckBox;

	void infoCheckBoxToggled ();

	void selectionUnitClicked (cUnitListViewItemBuy& unitItem);

	void okClicked ();
	void backClicked ();

	void handleSelectionChanged ();
};

#endif // gui_menu_windows_windowhangar_windowhangarH
