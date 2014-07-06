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

#ifndef utility_colorH
#define utility_colorH

#include <random>

#include <SDL.h>

/**
 * Simple RGBA Color class (RGBA = Red-Green-Blue-Alpha).
 */
class cColor
{
public:
	cColor::cColor ();
	cColor::cColor (unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 0xFF);

	bool cColor::operator==(const cColor& other) const;
	bool cColor::operator!=(const cColor& other) const;

	SDL_Color cColor::toSdlColor () const;
	Uint32 cColor::toMappedSdlRGBColor (const SDL_PixelFormat* format) const;
	Uint32 cColor::toMappedSdlRGBAColor (const SDL_PixelFormat* format) const;

	cColor cColor::exchangeRed (unsigned char red);
	cColor cColor::exchangeGreen (unsigned char green);
	cColor cColor::exchangeBlue (unsigned char blue);
	cColor cColor::exchangeAlpha (unsigned char alpha);

	unsigned char r, g, b;
	unsigned char a;

	// predefined colors
	inline static cColor red (unsigned char alpha_ = 0xFF) { return cColor (0xFF, 0, 0, alpha_); }
	inline static cColor green (unsigned char alpha_ = 0xFF) { return cColor (0, 0xFF, 0, alpha_); }
	inline static cColor blue (unsigned char alpha_ = 0xFF) { return cColor (0, 0, 0xFF, alpha_); }
	inline static cColor yellow (unsigned char alpha_ = 0xFF) { return cColor (0xFF, 0xFF, 0, alpha_); }
	inline static cColor black (unsigned char alpha_ = 0xFF) { return cColor (0, 0, 0, alpha_); }
	inline static cColor white (unsigned char alpha_ = 0xFF) { return cColor (0xFF, 0xFF, 0xFF, alpha_); }
	inline static cColor transparent () { return cColor (0, 0, 0, 0); }

	inline static cColor random (unsigned char alpha_ = 0xFF)
	{
		static std::random_device rd;
		static std::mt19937 engine (rd());
		// Workaround for C++11 bug: uniform_int_distribution does not allow char types
		std::uniform_int_distribution<unsigned int> distribution (0, (unsigned int)std::numeric_limits<unsigned char>::max ());
		return cColor ((unsigned char)distribution (engine), (unsigned char)distribution (engine), (unsigned char)distribution (engine), alpha_);
	}
};

#endif // utility_colorH