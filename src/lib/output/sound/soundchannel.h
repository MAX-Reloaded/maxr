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

#ifndef output_sound_soundchannelH
#define output_sound_soundchannelH

#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"

#include <mutex>

class cSoundChunk;
class cPosition;

class cSoundChannel
{
public:
	explicit cSoundChannel (int sdlChannelId);

	void play (const cSoundChunk&, bool loop = false);

	void pause();
	void resume();

	void stop();

	void mute();
	void unmute();

	bool isPlaying() const;
	bool isPlaying (const cSoundChunk&) const;

	bool isLooping() const { return looping; }

	bool isPaused() const;

	bool isMuted() const { return muted; }

	void setVolume (int volume);

	void setPanning (unsigned char left, unsigned char right);
	void setDistance (unsigned char distance);
	void setPosition (short angle, unsigned char distance);

	int getSdlChannelId() const { return sdlChannelId; }

	cSignal<void()> started;
	cSignal<void(), std::recursive_mutex> stopped;

	cSignal<void(), std::recursive_mutex> paused;
	cSignal<void(), std::recursive_mutex> resumed;

private:
	int sdlChannelId;

	bool muted = false;
	int volume;

	bool looping = false;

	cSignalConnectionManager signalConnectionManager;

	static void channelFinishedCallback (int channelId);
	static cSignal<void (int), std::recursive_mutex> channelFinished;
};

#endif // output_sound_soundchannelH
