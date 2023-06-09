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

#include "editablecheckbox.h"

#include "output/video/unifonts.h"
#include "ui/widgets/application.h"

//------------------------------------------------------------------------------
cEditableCheckBox::cEditableCheckBox (const cBox<cPosition>& area, const std::string& prefix, const std::string& suffix, eUnicodeFontType fontType) :
	cWidget (area)
{
	checkBox = emplaceChild<cCheckBox> (area.getMinCorner(), prefix, fontType, eCheckBoxTextAnchor::Left, eCheckBoxType::TextOnly);
	auto prefixSize = cUnicodeFont::font->getTextWide (prefix + ".", fontType);
	auto suffixSize = cUnicodeFont::font->getTextWide (suffix + ".", fontType);
	lineEdit = emplaceChild<cLineEdit> (cBox<cPosition> (area.getMinCorner() + cPosition (prefixSize, 0), area.getMaxCorner() + cPosition (-suffixSize, 0)));
	suffixLabel = emplaceChild<cLabel> (cBox<cPosition> (cPosition (area.getMaxCorner().x() - suffixSize, area.getMinCorner().y()), area.getMaxCorner()), suffix, eUnicodeFontType::LatinNormal, eAlignmentType::Right);

	signalConnectionManager.connect (checkBox->toggled, [this]() {
		if (checkBox->isChecked())
		{
			auto* application = getActiveApplication();
			if (application)
			{
				application->grapKeyFocus (*lineEdit);
			}
		}
	});
	signalConnectionManager.connect (lineEdit->clicked, [this]() { checkBox->setChecked (true); });
	signalConnectionManager.connect (suffixLabel->clicked, [this]() { checkBox->setChecked (true); });
}
