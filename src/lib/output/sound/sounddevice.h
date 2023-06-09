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
#ifndef output_sound_sounddeviceH
#define output_sound_sounddeviceH

#include "output/sound/soundchannelgroup.h"

#include <SDL_mixer.h>
#include <chrono>
#include <filesystem>
#include <memory>
#include <vector>

class cPosition;
class cSoundChunk;
class cSoundChannel;

class cSoundDevice
{
public:
	cSoundDevice();

	static cSoundDevice& getInstance();

	void initialize (int frequency, int chunkSize);
	void close();

	cSoundChannel* getFreeSoundEffectChannel();
	cSoundChannel* getFreeVoiceChannel();

	bool playSoundEffect (const cSoundChunk&);
	bool playVoice (const cSoundChunk&);

	void startMusic (const std::filesystem::path& fileName);
	void startRandomMusic();
	void stopMusic();

	void setSoundEffectVolume (int volume);
	void setVoiceVolume (int volume);
	void setMusicVolume (int volume);

private:
	const static int soundEffectGroupTag;
	const static int voiceGroupTag;

	const static int soundEffectGroupSize;
	const static int voiceGroupSize;

	struct SdlMixMusikDeleter
	{
		void operator() (Mix_Music*) const;
	};

	using SaveSdlMixMusicPointer = std::unique_ptr<Mix_Music, SdlMixMusikDeleter>;

	SaveSdlMixMusicPointer musicStream;

	cSoundChannelGroup soundEffectChannelGroup;
	cSoundChannelGroup voiceChannelGroup;
};

#endif // output_sound_sounddeviceH
