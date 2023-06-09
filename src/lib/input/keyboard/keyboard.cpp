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

#include "input/keyboard/keyboard.h"

#include "events/eventmanager.h"
#include "events/keyboardevents.h"

#include <cassert>

//------------------------------------------------------------------------------
cKeyboard::cKeyboard()
{
	signalConnectionManager.connect (cEventManager::getInstance().keyboardEvent, [this] (const cKeyboardEvent& event) { handleKeyboardEvent (event); });
	signalConnectionManager.connect (cEventManager::getInstance().textInputEvent, [this] (const cTextInputEvent& event) { handleTextInputEvent (event); });
}

//------------------------------------------------------------------------------
cKeyboard& cKeyboard::getInstance()
{
	static cKeyboard instance;
	return instance;
}

//------------------------------------------------------------------------------
void cKeyboard::updateModifiersFromKeyPress (const cKeyboardEvent& event)
{
	if (event.getType() != cKeyboardEvent::eType::Down) return;

	SDL_Keycode key = event.getKey();

	if (key == SDLK_LSHIFT)
		currentModifiers |= eKeyModifierType::ShiftLeft;
	else if (key == SDLK_RSHIFT)
		currentModifiers |= eKeyModifierType::ShiftRight;
	else if (key == SDLK_LCTRL)
		currentModifiers |= eKeyModifierType::CtrlLeft;
	else if (key == SDLK_RCTRL)
		currentModifiers |= eKeyModifierType::CtrlRight;
	else if (key == SDLK_LALT)
		currentModifiers |= eKeyModifierType::AltLeft;
	else if (key == SDLK_RALT)
		currentModifiers |= eKeyModifierType::AltRight;
	else if (key == SDLK_LGUI)
		currentModifiers |= eKeyModifierType::GuiLeft;
	else if (key == SDLK_RGUI)
		currentModifiers |= eKeyModifierType::GuiRight;
	else if (key == SDLK_NUMLOCKCLEAR)
		currentModifiers |= eKeyModifierType::Num;
	else if (key == SDLK_CAPSLOCK)
		currentModifiers |= eKeyModifierType::Caps;
	else if (key == SDLK_MODE)
		currentModifiers |= eKeyModifierType::Mode;
}

//------------------------------------------------------------------------------
KeyModifierFlags cKeyboard::getCurrentModifiers() const
{
	return currentModifiers;
}

//------------------------------------------------------------------------------
bool cKeyboard::isAnyModifierActive (KeyModifierFlags flags) const
{
	return currentModifiers & flags;
}

//------------------------------------------------------------------------------
bool cKeyboard::isAllModifiersActive (KeyModifierFlags flags) const
{
	return (currentModifiers & flags) == flags;
}

//------------------------------------------------------------------------------
void cKeyboard::handleKeyboardEvent (const cKeyboardEvent& event)
{
	KeyModifierFlags oldModifiers = currentModifiers;

	currentModifiers = event.getModifiers(); //set modifier of current key event

	switch (event.getType())
	{
		case cKeyboardEvent::eType::Down: keyPressed (*this, event.getKey()); break;
		case cKeyboardEvent::eType::Up: keyReleased (*this, event.getKey()); break;
	}

	//set modifier in case the key event was a modifier key itself
	//this is needed when a mouse object will querry the current modifier state later
	updateModifiersFromKeyPress (event);

	if (currentModifiers != oldModifiers)
		modifierChanged();
}

//------------------------------------------------------------------------------
void cKeyboard::handleTextInputEvent (const cTextInputEvent& event)
{
	textEntered (*this, event.getText());
}
