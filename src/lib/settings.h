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
#ifndef SETTINGS_H
#define SETTINGS_H

#include "game/data/player/playersettings.h"
#include "game/networkaddress.h"
#include "utility/serialization/serialization.h"
#include "utility/signal/signal.h"

#include <filesystem>
#include <mutex>
#include <string>

struct sVideoSettings
{
	std::optional<cPosition> resolution;
	int colourDepth = 32;
	int displayIndex = 0;
	bool windowMode = true;

	template <typename Archive>
	void serialize (Archive& archive)
	{
		// clang-format off
		// See https://github.com/llvm/llvm-project/issues/44312
		archive & NVP (resolution);
		archive & NVP (colourDepth);
		archive & NVP (displayIndex);
		archive & NVP (windowMode);
		// clang-format on
	}
};

/**
 * cSettings class stores all settings for the game and handles reading
 * and writing them from and to the configuration file.
 * To do this it needs to find out the location of the configuration
 * for the specific platform.
 *
 * For each element of the settings that has to be stored
 * in the configuration file there are getters and setters defined.
 * The setters will automatically save the changed values to the configuration.
 * If the configuration does not exist or is incomplete
 * it will be generated and the needed node will be added.
 *
 * This is a singleton class.
 */
class cSettings
{
public:
	/**
	 * Provides access to the one and only instance of this class.
	 * On the first call this method will initialize the instance.
	 * @return The static instance.
	 */
	static cSettings& getInstance();

	void saveInFile() const;

	/**
	 * Checks whether the class has been initialized already and successful
	 * (what means the configuration file has been read).
	 * @return True if the settings have been initialized.
	 */
	bool isInitialized() const { return initialized; }

	// Game

	bool shouldAutosave() const { return inGame.autosave; }
	void setAutosave (bool autosave) { inGame.autosave = autosave; }

	bool isAnimations() const { return inGame.animations; }
	void setAnimations (bool animations);

	bool isShadows() const { return inGame.shadows; }
	void setShadows (bool shadows) { inGame.shadows = shadows; }

	bool isAlphaEffects() const { return inGame.alphaEffects; }
	void setAlphaEffects (bool alphaEffects) { inGame.alphaEffects = alphaEffects; }

	bool shouldShowDescription() const { return inGame.showDescription; }
	void setShowDescription (bool showDescription) { inGame.showDescription = showDescription; }

	bool isDamageEffects() const { return inGame.damageEffects; }
	void setDamageEffects (bool damageEffects) { inGame.damageEffects = damageEffects; }

	bool isDamageEffectsVehicles() const { return inGame.damageEffectsVehicles; }
	void setDamageEffectsVehicles (bool damageEffectsVehicles) { inGame.damageEffectsVehicles = damageEffectsVehicles; }

	bool isMakeTracks() const { return inGame.makeTracks; }
	void setMakeTracks (bool makeTracks) { inGame.makeTracks = makeTracks; }

	int getScrollSpeed() const { return inGame.scrollSpeed; }
	void setScrollSpeed (int scrollSpeed) { inGame.scrollSpeed = scrollSpeed; }

	const std::filesystem::path& getDataDir() const;
	const std::filesystem::path& getLogPath() const;
	const std::filesystem::path& getNetLogPath() const;
	const std::filesystem::path& getMaxrHomeDir() const;

	const sNetworkAddress& getNetworkAddress() const { return network; }
	void setNetworkAddress (const sNetworkAddress& network) { this->network = network; }

	const sPlayerSettings& getPlayerSettings() const { return player; }
	void setPlayerSettings (const sPlayerSettings& player) { this->player = player; }

	const sVideoSettings& getVideoSettings() const { return video; }
	sVideoSettings& getVideoSettings() { return video; }

	// Sound

	bool isSoundEnabled() const { return sound.enabled; }
	void setSoundEnabled (bool enabled) { sound.enabled = enabled; }

	int getMusicVol() const { return sound.musicVol; }
	void setMusicVol (int musicVol) { sound.musicVol = musicVol; }

	int getSoundVol() const { return sound.soundVol; }
	void setSoundVol (int soundVol) { sound.soundVol = soundVol; }

	int getVoiceVol() const { return sound.voiceVol; }
	void setVoiceVol (int voiceVol) { sound.voiceVol = voiceVol; }

	int getChunkSize() const { return sound.chunkSize; }
	void setChunkSize (int chunkSize) { sound.chunkSize = chunkSize; }

	int getFrequency() const { return sound.frequency; }
	void setFrequence (int frequency) { sound.frequency = frequency; }

	bool isMusicMute() const { return sound.musicMute; }
	void setMusicMute (bool musicMute) { sound.musicMute = musicMute; }

	bool isSoundMute() const { return sound.soundMute; }
	void setSoundMute (bool soundMute) { sound.soundMute = soundMute; }

	bool isVoiceMute() const { return sound.voiceMute; }
	void setVoiceMute (bool voiceMute) { sound.voiceMute = voiceMute; }

	bool is3DSound() const { return sound.sound3d; }
	void set3DSound (bool sound3d) { sound.sound3d = sound3d; }

	// Global

	bool isDebug() const { return global.debug; }
	void setDebug (bool debug);

	bool shouldShowIntro() const { return global.showIntro; }
	void setShowIntro (bool showIntro) { global.showIntro = showIntro; }

	bool shouldUseFastMode() const { return global.fastMode; }
	bool shouldDoPrescale() const { return global.preScale; }

	const std::string& getLanguage() const { return global.language; }
	void setLanguage (const char* language) { global.language = language; }

	const std::string& getVoiceLanguage() const { return global.voiceLanguage; }

	unsigned int getCacheSize() const { return global.cacheSize; }
	void setCacheSize (unsigned int cacheSize) { global.cacheSize = cacheSize; }

	// Paths
	std::filesystem::path getFontPath() const;
	std::filesystem::path getFxPath() const;
	std::filesystem::path getGfxPath() const;
	std::filesystem::path getLangPath() const;
	std::filesystem::path getMapsPath() const;
	std::filesystem::path getSavesPath() const;
	std::filesystem::path getSoundsPath() const;
	std::filesystem::path getVoicesPath() const;
	std::filesystem::path getMusicPath() const;
	std::filesystem::path getVehiclesPath() const;
	std::filesystem::path getBuildingsPath() const;
	std::filesystem::path getMvePath() const;

	/**
	* Gets the map folder of the user's custom maps.
	* @author pagra
	* @return an absolute path to the user's maps directory or empty string, if no user maps folder is defined on the system
	*/
	std::filesystem::path getUserMapsDir() const;

	/**
	 * Gets the folder, where screenshots made by the user should be saved.
	 * @author pagra
	 * @return an absolute path to the user's screenshots directory or empty string, if no user screenshots folder is defined on the system
	 */
	std::filesystem::path getUserScreenshotsDir() const;

	template <typename Archive>
	void serialize (Archive& archive)
	{
		// clang-format off
		// See https://github.com/llvm/llvm-project/issues/44312
		archive & NVP (global);
		archive & NVP (network);
		archive & NVP (player);
		archive & NVP (sound);
		archive & NVP (path);
		archive & NVP (inGame);
		archive & NVP (video);
		// clang-format on
	}

	mutable cSignal<void()> animationsChanged;
	// TODO: add signals for other settings
private:
	/**
	 * Private constructor for singleton design pattern.
	 */
	cSettings();
	/**
	 * Private copy constructor for singleton design pattern.
	 * Does not need to be implemented!
	 */
	cSettings (const cSettings&) = delete;

	/**
	 * Gets the platform dependent user paths for the configuration file
	 * and reads the file.
	 * If the file (or parts of it) does not exist
	 * default values will be used and written to the (may generated) file.
	 */
	void initialize();

	void loadFromJsonFile (const std::filesystem::path&);

	/**
	 * Sets the platform dependent config, log and save paths.
	 */
	void setPaths();

	void setDataDir (const std::filesystem::path&);

private:
	struct sGlobalSettings
	{
		/** enable debug */
		bool debug = false;
		/** enable intro on start */
		bool showIntro = true;
		/** start in fastmode */
		bool fastMode = false;
		/** prescale gfx */
		bool preScale = false;
		/** translation file */
		std::string language = "en";
		/** language code for voice files */
		std::string voiceLanguage;
		/** cache size */
		unsigned int cacheSize = 400;

		template <typename Archive>
		void serialize (Archive& archive)
		{
			// clang-format off
			// See https://github.com/llvm/llvm-project/issues/44312
			archive & NVP (debug);
			archive & NVP (showIntro);
			archive & NVP (fastMode);
			archive & NVP (preScale);
			archive & NVP (language);
			archive & NVP (voiceLanguage);
			archive & NVP (cacheSize);
			// clang-format on
		}
	};

	struct sSoundSettings
	{
		/** sound enabled */
		bool enabled = true;
		/** volume music */
		int musicVol = 128;
		/** volume sound effects */
		int soundVol = 128;
		/** volume voices */
		int voiceVol = 128;
		/** chunk size */
		int chunkSize = 2048;
		/** frequency */
		int frequency = 44100;
		/** mute music */
		bool musicMute = false;
		/** mute sound effects */
		bool soundMute = false;
		/** mute voices */
		bool voiceMute = false;
		/** in-game sound effects should respect position*/
		bool sound3d = true;

		template <typename Archive>
		void serialize (Archive& archive)
		{
			// clang-format off
			// See https://github.com/llvm/llvm-project/issues/44312
			archive & NVP (enabled);
			archive & NVP (musicVol);
			archive & NVP (soundVol);
			archive & NVP (voiceVol);
			archive & NVP (chunkSize);
			archive & NVP (frequency);
			archive & NVP (musicMute);
			archive & NVP (soundMute);
			archive & NVP (voiceMute);
			archive & NVP (sound3d);
			// clang-format on
		}
	};

	struct sPathSettings
	{
		std::filesystem::path font = "fonts"; // Path to the fonts
		std::filesystem::path fx = "fx"; // Path to the effects
		std::filesystem::path gfx = "gfx"; // Path to the graphics
		std::filesystem::path languages = "languages"; // Path to language files
		std::filesystem::path maps = "maps"; // Path to the maps
		std::filesystem::path saves = "saves"; // Path to the saves
		std::filesystem::path sounds = "sounds"; // Path to the sound-files
		std::filesystem::path voices = "voices"; // Path to the voice-files
		std::filesystem::path music = "music"; // Path to the music-files
		std::filesystem::path vehicles = "vehicles"; // Path to the vehicles
		std::filesystem::path buildings = "buildings"; // Path to the buildings
		std::filesystem::path mve = "mve"; // Path to the in-game movies (*.mve)

		template <typename Archive>
		void serialize (Archive& archive)
		{
			// clang-format off
			// See https://github.com/llvm/llvm-project/issues/44312
			archive & NVP (font);
			archive & NVP (fx);
			archive & NVP (gfx);
			archive & NVP (languages);
			archive & NVP (maps);
			archive & NVP (saves);
			archive & NVP (sounds);
			archive & NVP (voices);
			archive & NVP (music);
			archive & NVP (vehicles);
			archive & NVP (buildings);
			archive & NVP (mve);
			// clang-format on
		}
	};

	struct sInGameSettings
	{
		/** enable autosave */
		bool autosave = true;
		/** enable animations */
		bool animations = true;
		/** enable shadows */
		bool shadows = true;
		/** enable alpha effects */
		bool alphaEffects = true;
		/** enable descriptions (e.g. in build menus) */
		bool showDescription = true;
		/** enable damage effects (smoke'n stuff) */
		bool damageEffects = true;
		/** enable damage effects for vehicles (smoke'n stuff) */
		bool damageEffectsVehicles = true;
		/** enable tracks (units leave tracks on the floor) */
		bool makeTracks = true;
		/** scrollspeed on map */
		int scrollSpeed = 32;

		template <typename Archive>
		void serialize (Archive& archive)
		{
			// clang-format off
			// See https://github.com/llvm/llvm-project/issues/44312
			archive & NVP (autosave);
			archive & NVP (animations);
			archive & NVP (shadows);
			archive & NVP (alphaEffects);
			archive & NVP (showDescription);
			archive & NVP (damageEffects);
			archive & NVP (damageEffectsVehicles);
			archive & NVP (makeTracks);
			archive & NVP (scrollSpeed);
			// clang-format on
		}
	};

	/**
	 * The static instance of this object.
	 */
	static cSettings instance;

	/**
	 * True if the object has been initialized already.
	 */
	bool initialized = false;
	/**
	 * True if initialization is running at the moment.
	 * Used to prevent the class to start multiple initializations.
	 */
	bool initializing = false;

	mutable std::recursive_mutex docMutex;

	/** dataDir is where the data files are stored */
	std::filesystem::path dataDir;
	/** log is where the log goes - set in setPaths() */
	std::filesystem::path logPath;
	/** netLog is where the netlog goes - set in setPaths() */
	std::filesystem::path netLogPath;
	/** home is where the user has his $HOME dir - set in setPaths() */
	std::filesystem::path homeDir;
	/** screeniesDir is where screenshot will be done - set in setPaths() **/
	std::filesystem::path screeniesDir;
	/** userMapsDir is where user maps are - set in setPaths() **/
	std::filesystem::path userMapsDir;

	sGlobalSettings global;
	sNetworkAddress network;
	sPlayerSettings player;
	sSoundSettings sound;
	sPathSettings path;
	sInGameSettings inGame;
	sVideoSettings video;
};

#endif // SETTINGS_H
