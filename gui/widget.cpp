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

#include "widget.h"

#include "../settings.h"
#include "../video.h"

//------------------------------------------------------------------------------
cWidget::cWidget () :
	parent (nullptr),
	enabled (true),
	area (cPosition (0, 0), cPosition (0, 0))
{
	createFrameSurface ();
}

//------------------------------------------------------------------------------
cWidget::cWidget (const cPosition& position) :
	parent (nullptr),
	enabled (true),
	area (position, position)
{
	createFrameSurface ();
}

//------------------------------------------------------------------------------
cWidget::cWidget (const cBox<cPosition>& area_) :
	parent (nullptr),
	enabled (true),
	area (area_)
{
	createFrameSurface ();
}

//------------------------------------------------------------------------------
cWidget::~cWidget ()
{}

//------------------------------------------------------------------------------
cWidget* cWidget::getParent () const
{
	return parent;
}

//------------------------------------------------------------------------------
bool cWidget::isEnabled () const
{
	return enabled;
}

//------------------------------------------------------------------------------
void cWidget::disable ()
{
	enabled = false;
}

//------------------------------------------------------------------------------
void cWidget::enable ()
{
	enabled = true;
}

//------------------------------------------------------------------------------
const cPosition& cWidget::getPosition () const
{
	return area.getMinCorner ();
}

//------------------------------------------------------------------------------
const cPosition& cWidget::getEndPosition () const
{
	return area.getMaxCorner ();
}

//------------------------------------------------------------------------------
void cWidget::moveTo (const cPosition& newPosition)
{
	auto offset = newPosition - area.getMinCorner ();
	move (offset);
}

//------------------------------------------------------------------------------
void cWidget::move (const cPosition& offset)
{
	area.getMinCorner () += offset;
	area.getMaxCorner () += offset;
	handleMoved (offset);
}

//------------------------------------------------------------------------------
cPosition cWidget::getSize () const
{
	return area.getMaxCorner () - area.getMinCorner ();
}

//------------------------------------------------------------------------------
void cWidget::resize (const cPosition& newSize)
{
	area.getMaxCorner () = area.getMinCorner () + newSize;

	createFrameSurface ();
}

//------------------------------------------------------------------------------
const cBox<cPosition>& cWidget::getArea () const
{
	return area;
}

//------------------------------------------------------------------------------
void cWidget::setArea (const cBox<cPosition>& area_)
{
	const cPosition newSize = area_.getMaxCorner () - area_.getMinCorner ();
	const cPosition offset = area_.getMinCorner () - getArea ().getMinCorner ();

	move (offset);
	resize (newSize);
}

//------------------------------------------------------------------------------
cWidget* cWidget::getChildAt (const cPosition& position) const
{
	// reverse order because the last child is the one that will be drawn last and therefor
	// is visually the one above all the others.
	// We want to find this one first, because we will abort on the first child that
	// intersects the point, regardless of whether there are other overlapping children.
	for (auto i = children.rbegin (); i != children.rend (); ++i)
	{
		auto child = i->get();

		if (child->isAt (position))
		{
			auto childChild = child->getChildAt (position);
			return childChild ? childChild : child;
		}
	}
	return nullptr;
}

//------------------------------------------------------------------------------
bool cWidget::isAt (const cPosition& position) const
{
	return area.withinOrTouches (position);
}

//------------------------------------------------------------------------------
void cWidget::draw ()
{
	if (frameSurface)
	{
		SDL_Rect position = getArea ().toSdlRect ();
		SDL_BlitSurface (frameSurface, NULL, cVideo::buffer, &position);
	}

	for (auto i = children.begin (); i != children.end (); ++i)
	{
		(*i)->draw ();
	}
}

//------------------------------------------------------------------------------
bool cWidget::handleMouseMoved (cApplication& application, cMouse& mouse)
{
	return false;
}

//------------------------------------------------------------------------------
bool cWidget::handleMousePressed (cApplication& application, cMouse& mouse, eMouseButtonType button)
{
	return false;
}

//------------------------------------------------------------------------------
bool cWidget::handleMouseReleased (cApplication& application, cMouse& mouse, eMouseButtonType button)
{
	return false;
}

//------------------------------------------------------------------------------
bool cWidget::handleMouseWheelMoved (cApplication& application, cMouse& mouse, const cPosition& amount)
{
	return false;
}

//------------------------------------------------------------------------------
bool cWidget::handleKeyPressed (cApplication& application, cKeyboard& keyboard, SDL_Keycode key)
{
	return false;
}

//------------------------------------------------------------------------------
bool cWidget::handleKeyReleased (cApplication& application, cKeyboard& keyboard, SDL_Keycode key)
{
	return false;
}

//------------------------------------------------------------------------------
bool cWidget::handleTextEntered (cApplication& application, cKeyboard& keyboard, const char* text)
{
	return false;
}

////------------------------------------------------------------------------------
//void cWidget::handleHoveredOn (cApplication& application, cMouse& mouse)
//{}
//
////------------------------------------------------------------------------------
//void  cWidget::handleHoveredAway (cApplication& application, cMouse& mouse)
//{}

//------------------------------------------------------------------------------
bool cWidget::handleGetKeyFocus (cApplication& application)
{
	return false;
}

//------------------------------------------------------------------------------
void cWidget::handleLooseKeyFocus (cApplication& application)
{}

//------------------------------------------------------------------------------
void cWidget::handleGetMouseFocus (cApplication& application)
{}

//------------------------------------------------------------------------------
void cWidget::handleLooseMouseFocus (cApplication& application)
{}

//------------------------------------------------------------------------------
void cWidget::handleMoved (const cPosition& offset)
{
	for (auto i = children.begin (); i != children.end (); ++i)
	{
		(*i)->move (offset);
	}
}

//------------------------------------------------------------------------------
const std::vector<std::unique_ptr<cWidget>>& cWidget::getChildren () const
{
	return children;
}

//------------------------------------------------------------------------------
void cWidget::createFrameSurface ()
{
	const bool debugDrawFrame = true;
	if (debugDrawFrame)
	{
		const auto size = getSize();

		frameSurface = SDL_CreateRGBSurface (0, size.x (), size.y (), Video.getColDepth (), 0, 0, 0, 0);
		SDL_SetColorKey (frameSurface, SDL_TRUE, 0xFF00FF);
		SDL_FillRect (frameSurface, NULL, 0xFF00FF);

		const auto color = SDL_MapRGB (frameSurface->format, 0xff, 0, 0);

		drawRectangle (frameSurface, cBox<cPosition>(cPosition(0,0), size), color);
	}
}

//------------------------------------------------------------------------------
void cWidget::drawRectangle (SDL_Surface* surface, const cBox<cPosition>& rectangle, Uint32 color)
{
	const cPosition size = rectangle.getMaxCorner () - rectangle.getMinCorner ();

	SDL_Rect line_h = {rectangle.getMinCorner ().x (), rectangle.getMinCorner ().y (), size.x (), 1};

	SDL_FillRect (surface, &line_h, color);
	line_h.y += size.y () - 1;
	SDL_FillRect (surface, &line_h, color);
	SDL_Rect line_v = {rectangle.getMinCorner ().x (), rectangle.getMinCorner ().y (), 1, size.y ()};
	SDL_FillRect (surface, &line_v, color);
	line_v.x += size.x () - 1;
	SDL_FillRect (surface, &line_v, color);
}