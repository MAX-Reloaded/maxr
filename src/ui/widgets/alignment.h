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

#ifndef ui_widgets_alignmentH
#define ui_widgets_alignmentH

#include "utility/enumflag.h"

/**
 * Alignment flag types.
 *
 * Using more than one horizontal or vertical flag at a time is not allowed.
 *
 * The @ref HorizontalMask and @ref MaskVertical are no valid alignment values
 * and should only be used to filter the horizontal and the vertical part from
 * the flags.
 */
enum class eAlignmentType
{
	Left = (1 << 0),
	Right = (1 << 1),
	CenterHorizontal = (1 << 2),
	HorizontalMask = Left | Right | CenterHorizontal,

	Top = (1 << 3),
	Bottom = (1 << 4),
	CenterVerical = (1 << 5),
	MaskVertical = Top | Bottom | CenterVerical,

	Center = CenterHorizontal | CenterVerical
};

using AlignmentFlags = cEnumFlag<eAlignmentType>;

#endif
