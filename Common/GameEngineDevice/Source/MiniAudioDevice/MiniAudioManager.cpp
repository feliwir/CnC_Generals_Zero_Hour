/*
**	Command & Conquer Generals(tm)
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 Stephan Vedder
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  (c) 2001-2003 Electronic Arts Inc.                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// FILE: MiniAudioManager.cpp
/*---------------------------------------------------------------------------*/
/* Project:    RTS3                                                          */
/* File name:  MiniAudioManager.cpp                                         */
/* Created:    Stephan Vedder, June 2026                                     */
/* Desc:       Implementation for the MiniAudioManager, interfacing with    */
/*             the Miniaudio Sound System.                                   */
/*---------------------------------------------------------------------------*/

#include "Lib/BaseType.h"
#define MINIAUDIO_IMPLEMENTATION
#include "MiniAudioDevice/MiniAudioManager.h"

#include "Common/AudioAffect.h"
#include "Common/AudioHandleSpecialValues.h"
#include "Common/AudioRequest.h"
#include "Common/AudioSettings.h"
#include "Common/AsciiString.h"
#include "Common/AudioEventInfo.h"
#include "Common/FileSystem.h"
#include "Common/GameCommon.h"
#include "Common/GameSounds.h"
#include "Common/CRCDebug.h"
#include "Common/GlobalData.h"

#include "GameClient/DebugDisplay.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/VideoPlayer.h"
#include "GameClient/View.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/TerrainLogic.h"

#include "Common/File.h"

#ifdef _INTERNAL
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

enum { INFINITE_LOOP_COUNT = 1000000 };

// Callback functions for miniaudio's virtual file system
static ma_result vfsFileOpen(ma_vfs *pVFS, const char *filename, ma_uint32 mode, ma_vfs_file *pFile);
static ma_result vfsFileClose(ma_vfs *pVFS, ma_vfs_file file);
static ma_result vfsFileInfo(ma_vfs *pVFS, ma_vfs_file file, ma_file_info *pInfo);
static ma_result vfsFileRead(ma_vfs *pVFS, ma_vfs_file file, void *pBuffer, size_t bytesToRead, size_t *pBytesRead);
static ma_result vfsFileTell(ma_vfs *pVFS, ma_vfs_file file, ma_int64 *pCursor);
static ma_result vfsFileSeek(ma_vfs *pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin);

//-------------------------------------------------------------------------------------------------
MiniAudioManager::MiniAudioManager() :
	m_playbackDeviceCount(0),
	m_selectedPlaybackDevice(PROVIDER_ERROR),
	m_selectedSpeakerType(0),
	m_lastSelectedPlaybackDevice(PROVIDER_ERROR),
	m_binkHandle(NULL),
	m_pref3DProvider(AsciiString::TheEmptyString),
	m_prefSpeaker(AsciiString::TheEmptyString),
	m_engineInitialized(false),
	m_contextInitialized(false)
{
}

//-------------------------------------------------------------------------------------------------
MiniAudioManager::~MiniAudioManager()
{
	DEBUG_ASSERTCRASH(m_binkHandle == NULL, ("Leaked a Bink handle. Chuybregts"));
	releaseHandleForBink();
	closeDevice();
	
	DEBUG_ASSERTCRASH(this == TheAudio, ("Umm...\n"));
	TheAudio = NULL;
}

//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) || defined(_INTERNAL)
AudioHandle MiniAudioManager::addAudioEvent( const AudioEventRTS *eventToAdd )
{
	if (TheGlobalData->m_preloadReport) {
		if (!eventToAdd->getEventName().isEmpty()) {
			m_allEventsLoaded.insert(eventToAdd->getEventName());
		}
	}

	return AudioManager::addAudioEvent(eventToAdd);
}
#endif

#if defined(_DEBUG) || defined(_INTERNAL)
//-------------------------------------------------------------------------------------------------
void MiniAudioManager::audioDebugDisplay( DebugDisplayInterface *dd, void *, FILE *fp )
{
	std::list<PlayingAudio *>::iterator it;
	
	Coord3D lookPos;
	TheTacticalView->getPosition( &lookPos );
	lookPos.z = TheTerrainLogic->getGroundHeight( lookPos.x, lookPos.y );
	const Coord3D *mikePos = TheAudio->getListenerPosition();
	Coord3D distanceVector = TheTacticalView->get3DCameraPosition();
	distanceVector.sub( mikePos );

	Int now = TheGameLogic->getFrame();
	static Int lastCheck = now;
	const Int frames = 60;

	if( dd )
	{
		dd->printf("MiniAudio version: %s    ", ma_version_string());
		// dd->printf("Memory Usage : %d/%d\n", m_audioCache->getCurrentlyUsedSize(), m_audioCache->getMaxSize());
		dd->printf("Sound: %s    ", (isOn(AudioAffect_Sound) ? "Yes" : "No"));
		dd->printf("3DSound: %s    ", (isOn(AudioAffect_Sound3D) ? "Yes" : "No"));
		dd->printf("Speech: %s    ", (isOn(AudioAffect_Speech) ? "Yes" : "No"));
		dd->printf("Music: %s\n", (isOn(AudioAffect_Music) ? "Yes" : "No"));
		dd->printf("Channels Available: ");
		dd->printf("%d Sounds    ", m_sound->getAvailableSamples());

		dd->printf("Volume: ");
		dd->printf("Sound: %d    ", REAL_TO_INT(m_soundVolume * 100.0f));
		dd->printf("3DSound: %d    ", REAL_TO_INT(m_sound3DVolume * 100.0f));
		dd->printf("Speech: %d    ", REAL_TO_INT(m_speechVolume * 100.0f));
		dd->printf("Music: %d\n", REAL_TO_INT(m_musicVolume * 100.0f));
		dd->printf("Current 3D Provider: %s    ", 
			
		TheAudio->getProviderName(getSelectedProvider()).str());
		dd->printf("Current Speaker Type: %s\n", TheAudio->translateUnsignedIntToSpeakerType(TheAudio->getSpeakerType()).str());

		dd->printf( "Looking at: (%d,%d,%d) -- Microphone at: (%d,%d,%d)\n", 
				(Int)lookPos.x, (Int)lookPos.y, (Int)lookPos.z, (Int)mikePos->x, (Int)mikePos->y, (Int)mikePos->z );
		dd->printf( "Camera distance from microphone: %d -- Zoom Volume: %d%%\n", 
				(Int)distanceVector.length(), (Int)(TheAudio->getZoomVolume()*100.0f) );

		dd->printf("-----------------------------------------------------------\n");
		dd->printf("Playing Audio\n");
	}
	if( fp )
	{
		fprintf( fp, "MiniAudio version: %s    ", ma_version_string() );
		// fprintf( fp, "Memory Usage : %d/%d\n", m_audioCache->getCurrentlyUsedSize(), m_audioCache->getMaxSize() );
		fprintf( fp, "Sound: %s    ", (isOn(AudioAffect_Sound) ? "Yes" : "No") );
		fprintf( fp, "3DSound: %s    ", (isOn(AudioAffect_Sound3D) ? "Yes" : "No") );
		fprintf( fp, "Speech: %s    ", (isOn(AudioAffect_Speech) ? "Yes" : "No") );
		fprintf( fp, "Music: %s\n", (isOn(AudioAffect_Music) ? "Yes" : "No") );
		fprintf( fp, "Channels Available: " );
		fprintf( fp, "%d Sounds    ", m_sound->getAvailableSamples() );
		fprintf( fp, "%d 3D Sounds\n", m_sound->getAvailable3DSamples() );
		fprintf( fp, "Volume: ");
		fprintf( fp, "Sound: %d    ", REAL_TO_INT(m_soundVolume * 100.0f) );
		fprintf( fp, "3DSound: %d    ", REAL_TO_INT(m_sound3DVolume * 100.0f) );
		fprintf( fp, "Speech: %d    ", REAL_TO_INT(m_speechVolume * 100.0f) );
		fprintf( fp, "Music: %d\n", REAL_TO_INT(m_musicVolume * 100.0f) );
		fprintf( fp, "Current 3D Provider: %s    ", TheAudio->getProviderName(getSelectedProvider()).str());
		fprintf( fp, "Current Speaker Type: %s\n", TheAudio->translateUnsignedIntToSpeakerType(TheAudio->getSpeakerType()).str());

		fprintf( fp, "Looking at: (%d,%d,%d) -- Microphone at: (%d,%d,%d)\n", 
				(Int)lookPos.x, (Int)lookPos.y, (Int)lookPos.z, (Int)mikePos->x, (Int)mikePos->y, (Int)mikePos->z );
		fprintf( fp, "Camera distance from microphone: %d -- Zoom Volume: %d%%\n", 
				(Int)distanceVector.length(), (Int)(TheAudio->getZoomVolume()*100.0f) );

		fprintf( fp, "-----------------------------------------------------------\n" );
		fprintf( fp, "Playing Audio\n" );
	}

	PlayingAudio *playing = NULL;
	Real volume = 0.0f;
	AsciiString filenameNoSlashes;


	// 2-D & 3-D Sounds
	if( dd )
	{
		dd->printf("-----------------------------------------------------Sounds\n");
		int i = 0;
		for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
			playing = *it;
			if (!playing) {
				dd->printf("%d: Silence\n", i);
				continue;
			}

			filenameNoSlashes = playing->m_audioEventRTS->getFilename();
			filenameNoSlashes = filenameNoSlashes.reverseFind('\\') + 1;
			
			// Calculate Sample volume
			volume = 100.0f;
			volume *= getEffectiveVolume(playing->m_audioEventRTS);

			dd->printf("%2d: %-20s - (%s) Volume: %d (2D)\n", i++, playing->m_audioEventRTS->getEventName().str(), filenameNoSlashes.str(), REAL_TO_INT(volume));
		}
	}
	if( fp )
	{
		fprintf( fp, "-----------------------------------------------------Sounds\n" );
		int i = 0;
		for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
			playing = *it;
			if( !playing ) 
			{
				continue;
			}
			filenameNoSlashes = playing->m_audioEventRTS->getFilename();
			filenameNoSlashes = filenameNoSlashes.reverseFind( '\\' ) + 1;
			
			// Calculate Sample volume
			volume = 100.0f;
			volume *= getEffectiveVolume( playing->m_audioEventRTS );

			fprintf( fp, "%2d: %-20s - (%s) Volume: %d (2D)\n", i++, playing->m_audioEventRTS->getEventName().str(), filenameNoSlashes.str(), REAL_TO_INT( volume ) );
		}
	}

	const Coord3D *microphonePos = TheAudio->getListenerPosition();
}
#endif

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::init()
{
	AudioManager::init();
#ifdef INTENSE_DEBUG
	DEBUG_LOG(("Sound has temporarily been disabled in debug builds only. jkmcd\n"));
	// for now, _DEBUG builds only should have no sound. ask jkmcd or srj about this.
	return;
#endif

	// We should now know how many samples we want to load
	openDevice();
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::postProcessLoad()
{
	AudioManager::postProcessLoad();
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::reset()
{
#if defined(_DEBUG) || defined(_INTERNAL)
	dumpAllAssetsUsed();
	m_allEventsLoaded.clear();
#endif

	AudioManager::reset();
	stopAllAudioImmediately();
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::update()
{
	AudioManager::update();
	setDeviceListenerPosition();
	processRequestList();
	processPlayingList();
	processFadingList();
	processStoppedList();	
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::stopAudio( AudioAffect which )
{
	if (BitTest(which, AudioAffect_Sound)) {
		ma_sound_group_stop(&m_soundGroup);
	}

	if (BitTest(which, AudioAffect_Sound3D)) {
		ma_sound_group_stop(&m_sound3DGroup);
	}

	if (BitTest(which, AudioAffect_Speech)) {
		ma_sound_group_stop(&m_speechGroup);
	}

	if (BitTest(which, AudioAffect_Music)) {
		ma_sound_group_stop(&m_musicGroup);
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::pauseAudio( AudioAffect which )
{
	if (BitTest(which, AudioAffect_Sound)) {
		ma_sound_group_stop(&m_soundGroup);
	}

	if (BitTest(which, AudioAffect_Sound3D)) {
		ma_sound_group_stop(&m_sound3DGroup);
	}

	if (BitTest(which, AudioAffect_Speech)) {
		ma_sound_group_stop(&m_speechGroup);
	}

	if (BitTest(which, AudioAffect_Music)) {
		ma_sound_group_stop(&m_musicGroup);
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::resumeAudio( AudioAffect which )
{
	if (BitTest(which, AudioAffect_Sound)) {
		ma_sound_group_start(&m_soundGroup);
	}

	if (BitTest(which, AudioAffect_Sound3D)) {
		ma_sound_group_start(&m_sound3DGroup);
	}

	if (BitTest(which, AudioAffect_Speech)) {
		ma_sound_group_start(&m_speechGroup);
	}

	if (BitTest(which, AudioAffect_Music)) {
		ma_sound_group_start(&m_musicGroup);
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::pauseAmbient( Bool shouldPause )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::stopAllAmbientsBy( Object *obj )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::stopAllAmbientsBy( Drawable *draw )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::playAudioEvent( AudioEventRTS *event )
{
#ifdef INTENSIVE_AUDIO_DEBUG
	DEBUG_LOG(("MILES (%d) - Processing play request: %d (%s)", TheGameLogic->getFrame(), event->getPlayingHandle(), event->getEventName().str()));
#endif
	const AudioEventInfo *info = event->getAudioEventInfo();
	if (!info) {
		return;
	}

	std::list<PlayingAudio *>::iterator it;
	PlayingAudio *playing = NULL;

	AudioHandle handleToKill = event->getHandleToKill();

	AsciiString fileToPlay = event->getFilename();
	PlayingAudio *audio = allocatePlayingAudio();
	audio->m_audioEventRTS = event; 

	Bool foundSoundToReplace = false;
	if (handleToKill) {
		for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
			playing = (*it);
			if (!playing) {
				continue;
			}

			if (playing->m_audioEventRTS && playing->m_audioEventRTS->getPlayingHandle() == handleToKill) 
			{
				//Release this streaming channel immediately because we are going to play another sound in it's place.
				releasePlayingAudio(playing);
				m_playingSounds.erase(it);
				foundSoundToReplace = true;
				break;
			}
		}
	}

	ma_sound_group *groupToUse = NULL;
	ma_uint32 flags = 0;
	switch(info->m_soundType)
	{
		case AT_Music:
			groupToUse = &m_musicGroup;
			flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH;
			break;
		case AT_Streaming:
			groupToUse = &m_speechGroup;
			flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH;
			break;
		case AT_SoundEffect:
			if (event->isPositionalAudio()) {
				groupToUse = &m_sound3DGroup;
			} else {
				groupToUse = &m_soundGroup;
				flags = MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH;
			}
			break;
	}

	ma_sound* sound = NULL;
	if (!handleToKill || foundSoundToReplace) {
		sound = (ma_sound*)malloc(sizeof(ma_sound));
		ma_result result = ma_sound_init_from_file(&m_engine, fileToPlay.str(), flags, groupToUse, NULL, sound);
		if (result != MA_SUCCESS) {
			DEBUG_LOG(("Failed to initialize sound from file: %s", fileToPlay.str()));
			releasePlayingAudio(audio);
			return;
		}
	} else {
		DEBUG_LOG(("Skipping sound!"));
		releasePlayingAudio(audio);
		return;
	}

	switch(info->m_soundType)
	{
		case AT_Music:
		case AT_Streaming:
		{
		#ifdef INTENSIVE_AUDIO_DEBUG
			DEBUG_LOG(("- Stream\n"));
		#endif
			
			if ((info->m_soundType == AT_Streaming) && event->getUninterruptable()) {
				stopAllSpeech();
			}

			// Put this on here, so that the audio event RTS will be cleaned up regardless.
			audio->m_sound = sound;
			audio->m_type = PAT_Stream;

			if ((info->m_soundType == AT_Streaming) && event->getUninterruptable()) {
				setDisallowSpeech(TRUE);
			}
			break;
		}

		case AT_SoundEffect:
		{
		#ifdef INTENSIVE_AUDIO_DEBUG
			DEBUG_LOG(("- Sound"));
		#endif


			if (event->isPositionalAudio()) {
				// Sounds that are non-global are positional 3-D sounds. Deal with them accordingly
			#ifdef INTENSIVE_AUDIO_DEBUG
				DEBUG_LOG((" Positional"));
			#endif
				// Push it onto the list of playing things
				audio->m_sound = sound;
				audio->m_type = PAT_3DSample;

				// Set the position values of the sample here
				if (event->getAudioEventInfo()->m_type & ST_GLOBAL) {
					ma_sound_set_min_distance(sound, TheAudio->getAudioSettings()->m_globalMinRange );
					ma_sound_set_max_distance(sound, TheAudio->getAudioSettings()->m_globalMaxRange );
				} else {
					ma_sound_set_min_distance(sound, event->getAudioEventInfo()->m_minDistance );
					ma_sound_set_max_distance(sound, event->getAudioEventInfo()->m_maxDistance );
				}
				const Coord3D* pos = event->getCurrentPosition();
				ma_sound_set_position(sound, pos->x, pos->y, pos->z);
				m_sound->notifyOf3DSampleStart();

				ma_vec3f relPos;
				ma_spatializer_get_relative_position_and_direction(&sound->engineNode.spatializer, &m_engine.listeners[0], &relPos, NULL);
			} 
			else 
			{
				// Push it onto the list of playing things
				audio->m_sound = sound;
				audio->m_type = PAT_Sample;
				m_sound->notifyOf2DSampleStart();
				#ifdef INTENSIVE_AUDIO_DEBUG
					DEBUG_LOG((" Playing.\n"));
				#endif
			}
			break;
		}
	}

	ma_sound_set_volume(sound, event->getVolume());
	ma_result result = ma_sound_start(sound);
	if (result != MA_SUCCESS) {
		DEBUG_LOG(("Failed to start sound. Error code: %d", result));
		releasePlayingAudio(audio);
		return;
	}
	m_playingSounds.push_back(audio);
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::stopAudioEvent( AudioHandle handle )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::killAudioEventImmediately( AudioHandle audioEvent )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::pauseAudioEvent( AudioHandle handle )
{
	// pause audio
}

//-------------------------------------------------------------------------------------------------
PlayingAudio *MiniAudioManager::allocatePlayingAudio( void )
{
	PlayingAudio *aud = NEW PlayingAudio;	// poolify
	return aud;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::releaseMilesHandles( PlayingAudio *release )
{
	if (release->m_sound) {
		ma_sound_uninit(release->m_sound);
		free(release->m_sound);
	}
	release->m_type = PAT_INVALID;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::releasePlayingAudio( PlayingAudio *release )
{
	if (release->m_audioEventRTS->getAudioEventInfo()->m_soundType == AT_SoundEffect && release->m_sound) {
		if (release->m_type == PAT_Sample) {
			m_sound->notifyOf2DSampleCompletion();
		} else {
			m_sound->notifyOf3DSampleCompletion();
		}
	}
	releaseMilesHandles(release);	// forces stop of this audio
	if (release->m_cleanupAudioEventRTS) {
		// This causes a double free in USA campaign and probably other places
		//releaseAudioEventRTS(release->m_audioEventRTS);
	}
	delete release;
	release = NULL;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::stopAllAudioImmediately( void )
{
	std::list<PlayingAudio *>::iterator it;
	PlayingAudio *playing;

	for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ) {
		playing = *it;
		if (!playing) {
			continue;
		}

		releasePlayingAudio(playing);
		it = m_playingSounds.erase(it);
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::freeAllMilesHandles( void )
{
	ma_engine_stop(&m_engine);
}


//-------------------------------------------------------------------------------------------------
void MiniAudioManager::adjustPlayingVolume( PlayingAudio *audio )
{
	Real desiredVolume = audio->m_audioEventRTS->getVolume() * audio->m_audioEventRTS->getVolumeShift();
	if (audio->m_sound) {
		ma_sound_set_volume(audio->m_sound, desiredVolume);
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::stopAllSpeech( void )
{
	std::list<PlayingAudio *>::iterator it;
	PlayingAudio *playing;
	for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ) {
		playing = (*it);
		if (!playing) {
			continue;
		}

		if (playing->m_audioEventRTS->getAudioEventInfo()->m_soundType == AT_Streaming) {
			releasePlayingAudio(playing);
			it = m_playingSounds.erase(it);
		} else {
			++it;
		}
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::nextMusicTrack( void )
{
	AsciiString trackName;
	std::list<PlayingAudio *>::iterator it;
	PlayingAudio *playing;
	for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
		playing = *it;
		if (playing && playing->m_audioEventRTS->getAudioEventInfo()->m_soundType == AT_Music) {
			trackName = playing->m_audioEventRTS->getEventName();
		}
	}

	// Stop currently playing music
	TheAudio->removeAudioEvent(AHSV_StopTheMusic);

	trackName = nextTrackName(trackName);
	AudioEventRTS newTrack(trackName);
	TheAudio->addAudioEvent(&newTrack);
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::prevMusicTrack( void )
{
	AsciiString trackName;
	std::list<PlayingAudio *>::iterator it;
	PlayingAudio *playing;
	for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
		playing = *it;
		if (playing && playing->m_audioEventRTS->getAudioEventInfo()->m_soundType == AT_Music) {
			trackName = playing->m_audioEventRTS->getEventName();
		}
	}

	// Stop currently playing music 
	TheAudio->removeAudioEvent(AHSV_StopTheMusic);

	trackName = prevTrackName(trackName);
	AudioEventRTS newTrack(trackName);
	TheAudio->addAudioEvent(&newTrack);
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::isMusicPlaying( void ) const
{
	return ma_sound_group_is_playing(&m_musicGroup) == MA_TRUE;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::hasMusicTrackCompleted( const AsciiString& trackName, Int numberOfTimes ) const
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
AsciiString MiniAudioManager::getMusicTrackName( void ) const
{
	// First check the requests. If there's one there, then report that as the currently playing track.
	std::list<AudioRequest *>::const_iterator ait;
	for (ait = m_audioRequests.begin(); ait != m_audioRequests.end(); ++ait) {
		if ((*ait)->m_request != AR_Play) {
			continue;
		}

		if (!(*ait)->m_usePendingEvent) {
			continue;
		}

		if ((*ait)->m_pendingEvent->getAudioEventInfo()->m_soundType == AT_Music) {
			return (*ait)->m_pendingEvent->getEventName();
		}
	}

	std::list<PlayingAudio *>::const_iterator it;
	PlayingAudio *playing;
	for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
		playing = *it;
		if (playing && playing->m_audioEventRTS->getAudioEventInfo()->m_soundType == AT_Music) {
			return playing->m_audioEventRTS->getEventName();
		}
	}

	return AsciiString::TheEmptyString;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::openDevice( void )
{
	if (!TheGlobalData->m_audioOn) {
		return;
	}

	ma_result result;
    ma_resource_manager_config resourceManagerConfig;
	ma_context_config contextConfig;
	ma_engine_config engineConfig;
	static ma_vfs_callbacks vfs = {};
	vfs.onOpen = vfsFileOpen;
	vfs.onClose = vfsFileClose;
	vfs.onInfo = vfsFileInfo;
	vfs.onRead = vfsFileRead;
	vfs.onTell = vfsFileTell;
	vfs.onSeek = vfsFileSeek;

	// Use a custom resource manager, so we can load from our virtual file system.
	resourceManagerConfig = ma_resource_manager_config_init();
	resourceManagerConfig.pVFS = &vfs;
	result = ma_resource_manager_init(&resourceManagerConfig, &m_resourceManager);
	if (result != MA_SUCCESS) {
		DEBUG_LOG(("Failed to initialize miniaudio resource manager. Error code: %d\n", result));
		// if we couldn't initialize any devices, turn sound off (fail silently)
		setOn( false, AudioAffect_All );
		return;  // Failed to initialize the resource manager.
	}

	result = ma_log_init(NULL, &m_log);
	if (result != MA_SUCCESS) {
		DEBUG_LOG(("Failed to initialize miniaudio log. Error code: %d\n", result));
		// if we couldn't initialize any devices, turn sound off (fail silently)
		setOn( false, AudioAffect_All );
		return;  // Failed to initialize the log.
	}

	auto on_log = [](void* pUserData, ma_uint32 logLevel, const char* message) {
		DEBUG_LOG(("miniaudio: %s - %s", ma_log_level_to_string(logLevel), message));
	};
	ma_log_register_callback(&m_log, ma_log_callback_init(on_log, NULL));
	contextConfig = ma_context_config_init();
	contextConfig.pLog = &m_log;

	/* We're going to want a context so we can enumerate our playback devices. */
    result = ma_context_init(NULL, 0, &contextConfig, &m_context);
    if (result != MA_SUCCESS) {
        DEBUG_LOG(("Failed to initialize miniaudio context. Error code: %d\n", result));
		setOn( false, AudioAffect_All );
        return;
    }
	m_contextInitialized = true;


    /*
    Now that we have a context we will want to enumerate over each device so we can display them to the user and give
    them a chance to select the output devices they want to use.
    */
    result = ma_context_get_devices(&m_context, &m_playbackDevices, &m_playbackDeviceCount, NULL, NULL);
    if (result != MA_SUCCESS) {
        DEBUG_LOG(("Failed to enumerate miniaudio playback devices. Error code: %d\n", result));
        ma_context_uninit(&m_context);
		setOn( false, AudioAffect_All );
        return;
    }

	engineConfig = ma_engine_config_init();
	engineConfig.pResourceManager = &m_resourceManager;

	result = ma_engine_init(&engineConfig, &m_engine);
	if (result != MA_SUCCESS) {
		DEBUG_LOG(("Failed to initialize the Miniaudio engine. Error code: %d\n", result));
		// if we couldn't initialize any devices, turn sound off (fail silently)
		setOn( false, AudioAffect_All );
		return;  // Failed to initialize the engine.
	}
	m_engineInitialized = true;

	ma_sound_group_init(&m_engine, 0, NULL, &m_musicGroup);
	ma_sound_group_init(&m_engine, 0, NULL, &m_soundGroup);
	ma_sound_group_init(&m_engine, 0, NULL, &m_sound3DGroup);
	ma_sound_group_init(&m_engine, 0, NULL, &m_speechGroup);
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::closeDevice( void )
{
	// Guard against teardown when the device was never (fully) opened -- e.g.
	// a MiniAudioManager constructed purely to satisfy TheAudio in unit tests.
	// Calling ma_*_uninit on uninitialized structs walks garbage and crashes.
	if (m_engineInitialized) {
		ma_engine_uninit(&m_engine);
		m_engineInitialized = false;
	}
	if (m_contextInitialized) {
		ma_context_uninit(&m_context);
		m_contextInitialized = false;
	}
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::isCurrentlyPlaying( AudioHandle handle )
{
	std::list<PlayingAudio *>::iterator it;
	PlayingAudio *playing;

	for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
		playing = *it;
		if (playing && playing->m_audioEventRTS->getPlayingHandle() == handle) {
			return true;
		}
	}

	// if something is requested, it is also considered playing
	std::list<AudioRequest *>::iterator ait;
	AudioRequest *req = NULL;
	for (ait = m_audioRequests.begin(); ait != m_audioRequests.end(); ++ait) {
		req = *ait;
		if (req && req->m_usePendingEvent && req->m_pendingEvent->getPlayingHandle() == handle) {
			return true;
		}
	}

	return false;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::notifyOfAudioCompletion( UnsignedInt audioCompleted, UnsignedInt flags )
{
	PlayingAudio *playing = findPlayingAudioFrom(audioCompleted, flags);
	if (!playing) {
		DEBUG_CRASH(("Audio has completed playing, but we can't seem to find it. - jkmcd"));
		return;
	}

	if (getDisallowSpeech() && playing->m_audioEventRTS->getAudioEventInfo()->m_soundType == AT_Streaming) {
		setDisallowSpeech(FALSE);
	}

	if (playing->m_audioEventRTS->getAudioEventInfo()->m_control & AC_LOOP) {
		if (playing->m_audioEventRTS->getNextPlayPortion() == PP_Attack) {
			playing->m_audioEventRTS->setNextPlayPortion(PP_Sound);
		}
		if (playing->m_audioEventRTS->getNextPlayPortion() == PP_Sound) {
			// First, decrease the loop count.
			playing->m_audioEventRTS->decreaseLoopCount();

			// Now, try to start the next loop
			if (startNextLoop(playing)) {
				return;
			}
		}
	}

}

//-------------------------------------------------------------------------------------------------
PlayingAudio *MiniAudioManager::findPlayingAudioFrom( UnsignedIntPtr audioCompleted, UnsignedInt flags )
{
	std::list<PlayingAudio *>::iterator it;
	PlayingAudio *playing;

	ma_sound* sound = (ma_sound*) audioCompleted;
	for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
		playing = *it;
		if (playing && playing->m_sound == sound) {
			return playing;
		}
	}
	return NULL;
}

//-------------------------------------------------------------------------------------------------
UnsignedInt MiniAudioManager::getProviderCount( void ) const
{
	return m_playbackDeviceCount;
}

//-------------------------------------------------------------------------------------------------
AsciiString MiniAudioManager::getProviderName( UnsignedInt providerNum ) const
{
	if (isOn(AudioAffect_Sound3D) && providerNum < m_playbackDeviceCount) {
		return m_playbackDevices[providerNum].name;
	}

	return AsciiString::TheEmptyString;
}

//-------------------------------------------------------------------------------------------------
UnsignedInt MiniAudioManager::getProviderIndex( AsciiString providerName ) const
{
	for (UnsignedInt i = 0; i < m_playbackDeviceCount; ++i) {
		if (providerName == m_playbackDevices[i].name) {
			return i;
		}
	}
	return PROVIDER_ERROR;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::selectProvider( UnsignedInt providerNdx )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::unselectProvider( void )
{
}

//-------------------------------------------------------------------------------------------------
UnsignedInt MiniAudioManager::getSelectedProvider( void ) const
{
	return m_selectedPlaybackDevice;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::setSpeakerType( UnsignedInt speakerType )
{
}

//-------------------------------------------------------------------------------------------------
UnsignedInt MiniAudioManager::getSpeakerType( void )
{
	return 0;
}

//-------------------------------------------------------------------------------------------------
UnsignedInt MiniAudioManager::getNum2DSamples( void ) const
{
	return 0;
}

//-------------------------------------------------------------------------------------------------
UnsignedInt MiniAudioManager::getNum3DSamples( void ) const
{
	return 0;
}

//-------------------------------------------------------------------------------------------------
UnsignedInt MiniAudioManager::getNumStreams( void ) const
{
	return 0;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::doesViolateLimit( AudioEventRTS *event ) const
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::isPlayingAlready( AudioEventRTS *event ) const
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::isObjectPlayingVoice( UnsignedInt objID ) const
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::has3DSensitiveStreamsPlaying( void ) const
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
AudioEventRTS *MiniAudioManager::findLowestPrioritySound( AudioEventRTS *event )
{
	return NULL;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::isPlayingLowerPriority( AudioEventRTS *event ) const
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::killLowestPrioritySoundImmediately( AudioEventRTS *event )
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::adjustVolumeOfPlayingAudio( AsciiString eventName, Real newVolume )
{
	Real pan;
	std::list<PlayingAudio *>::iterator it;

	PlayingAudio *playing = NULL;
	for (it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
		playing = *it;
		if (playing && playing->m_audioEventRTS->getEventName() == eventName) {
			// Adjust it
			playing->m_audioEventRTS->setVolume(newVolume);
			Real desiredVolume = playing->m_audioEventRTS->getVolume() * playing->m_audioEventRTS->getVolumeShift();
			ma_sound_set_volume(playing->m_sound, desiredVolume);
		}
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::removePlayingAudio( AsciiString eventName )
{
	std::list<PlayingAudio *>::iterator it;

	PlayingAudio *playing = NULL;
	for( it = m_playingSounds.begin(); it != m_playingSounds.end(); ) 
	{
		playing = *it;
		if( playing && playing->m_audioEventRTS->getEventName() == eventName ) 
		{
			releasePlayingAudio( playing );
			it = m_playingSounds.erase(it);
		}
		else
		{
			it++;
		}
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::removeAllDisabledAudio()
{
	std::list<PlayingAudio *>::iterator it;

	PlayingAudio *playing = NULL;
	for( it = m_playingSounds.begin(); it != m_playingSounds.end(); ) 
	{
		playing = *it;
		if( playing && playing->m_audioEventRTS->getVolume() == 0.0f ) 
		{
			releasePlayingAudio( playing );
			it = m_playingSounds.erase(it);
		}
		else
		{
			it++;
		}
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::processRequestList( void )
{
	std::list<AudioRequest*>::iterator it;
	for (it = m_audioRequests.begin(); it != m_audioRequests.end(); /* empty */) {
		AudioRequest *req = (*it);
		if (req == NULL) {
			continue;
		}

		if (!shouldProcessRequestThisFrame(req)) {
			adjustRequest(req);
			++it;
			continue;
		}

		if (!req->m_requiresCheckForSample || checkForSample(req)) {
			processRequest(req);
		}
		req->deleteInstance();
		it = m_audioRequests.erase(it);
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::processPlayingList( void )
{
	if(m_volumeHasChanged) {
		ma_sound_group_set_volume(&m_musicGroup, m_musicVolume);
		ma_sound_group_set_volume(&m_speechGroup, m_speechVolume);
		ma_sound_group_set_volume(&m_soundGroup, m_soundVolume);
		ma_sound_group_set_volume(&m_sound3DGroup, m_sound3DVolume);
		m_volumeHasChanged = false;
	}
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::processFadingList( void )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::processStoppedList( void )
{
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::shouldProcessRequestThisFrame( AudioRequest *req ) const
{
	if (!req->m_usePendingEvent) {
		return true;
	}

	if (req->m_pendingEvent->getDelay() < MSEC_PER_LOGICFRAME_REAL) {
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::adjustRequest( AudioRequest *req )
{
	if (!req->m_usePendingEvent) {
		return;
	}

	req->m_pendingEvent->decrementDelay(MSEC_PER_LOGICFRAME_REAL);
	req->m_requiresCheckForSample = true;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::checkForSample( AudioRequest *req )
{
	if (!req->m_usePendingEvent) {
		return true;
	}

	if (req->m_pendingEvent->getAudioEventInfo()->m_type != AT_SoundEffect) {
		return true;
	}

	return m_sound->canPlayNow(req->m_pendingEvent);
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::setHardwareAccelerated( Bool accel )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::setSpeakerSurround( Bool surround )
{
}

//-------------------------------------------------------------------------------------------------
Real MiniAudioManager::getFileLengthMS( AsciiString strToLoad ) const
{
	float lengthInSeconds = 0.0f;
	ma_sound sound;
	ma_sound_init_from_file((ma_engine*)&m_engine, strToLoad.str(), 0, NULL, NULL, &sound);
	ma_sound_get_length_in_seconds(&sound, &lengthInSeconds);
	return lengthInSeconds * 1000.0f;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::closeAnySamplesUsingFile( const void *fileToClose )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::setDeviceListenerPosition( void )
{
	ma_engine_listener_set_direction(&m_engine, 0, m_listenerOrientation.x, m_listenerOrientation.y, m_listenerOrientation.z);
	ma_engine_listener_set_position(&m_engine, 0, m_listenerPosition.x, m_listenerPosition.y, m_listenerPosition.z);
}

//-------------------------------------------------------------------------------------------------
const Coord3D *MiniAudioManager::getCurrentPositionFromEvent( AudioEventRTS *event )
{
	if (!event->isPositionalAudio()) {
		return NULL;
	}

	return event->getCurrentPosition();
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::isOnScreen( const Coord3D *pos ) const
{
	static ICoord2D dummy;
	// WorldToScreen will return True if the point is onscreen and false if it is offscreen.
	return TheTacticalView->worldToScreen(pos, &dummy);
}

//-------------------------------------------------------------------------------------------------
Real MiniAudioManager::getEffectiveVolume( AudioEventRTS *event ) const
{
	Real volume = 1.0f;
	volume *= (event->getVolume() * event->getVolumeShift());
	if (event->getAudioEventInfo()->m_soundType == AT_Music) 
	{
		volume *= m_musicVolume;
	} 
	else if (event->getAudioEventInfo()->m_soundType == AT_Streaming) 
	{
		volume *= m_speechVolume;
	} 
	else 
	{
		if (event->isPositionalAudio()) 
		{
			volume *= m_sound3DVolume;
			Coord3D distance = m_listenerPosition;
			const Coord3D *pos = event->getCurrentPosition();
			if (pos) 
			{
				distance.sub(pos);
				Real objMinDistance;
				Real objMaxDistance;

				if (event->getAudioEventInfo()->m_type & ST_GLOBAL) 
				{
					objMinDistance = TheAudio->getAudioSettings()->m_globalMinRange;
					objMaxDistance = TheAudio->getAudioSettings()->m_globalMaxRange;
				} 
				else 
				{
					objMinDistance = event->getAudioEventInfo()->m_minDistance;
					objMaxDistance = event->getAudioEventInfo()->m_maxDistance;
				}

				Real objDistance = distance.length();
				if( objDistance > objMinDistance ) 
				{
					volume *= 1 / (objDistance / objMinDistance);
				}
				if( objDistance >= objMaxDistance ) 
				{
					volume = 0.0f;
				}
				//else if( objDistance > objMinDistance )
				//{
				//	volume *= 1.0f - (objDistance - objMinDistance) / (objMaxDistance - objMinDistance);
				//}
			}
		} 
		else 
		{
			volume *= m_soundVolume;
		}
	}

	return volume;
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::startNextLoop( PlayingAudio *looping )
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::buildProviderList( void )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::createListener( void )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::initDelayFilter( void )
{
}

//-------------------------------------------------------------------------------------------------
Bool MiniAudioManager::isValidProvider( void )
{
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::initSamplePools( void )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::processRequest( AudioRequest *req )
{
	switch (req->m_request)
	{
		case AR_Play:
		{
			playAudioEvent(req->m_pendingEvent);
			break;
		}
		case AR_Pause:
		{
			pauseAudioEvent(req->m_handleToInteractOn);
			break;
		}
		case AR_Stop:
		{
			stopAudioEvent(req->m_handleToInteractOn);
			break;
		}
	}
}

//-------------------------------------------------------------------------------------------------
void *MiniAudioManager::getHandleForBink( void )
{
	return NULL;
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::releaseHandleForBink( void )
{
}

//-------------------------------------------------------------------------------------------------
void MiniAudioManager::friend_forcePlayAudioEventRTS( const AudioEventRTS *eventToPlay )
{
	playAudioEvent(const_cast<AudioEventRTS *>(eventToPlay));
}


static ma_result vfsFileOpen(ma_vfs *pVFS, const char *filename, ma_uint32 mode, ma_vfs_file *pFile)
{
	Int access = 0;
	if (mode & MA_OPEN_MODE_READ) {
		access |= File::READ;
	}
	if (mode & MA_OPEN_MODE_WRITE) {
		access |= File::WRITE;
	}

	File *file = TheFileSystem->openFile(filename, access);
	if (!file)
	{
		return MA_DOES_NOT_EXIST;
	}

	*pFile = file;
	return MA_SUCCESS;
}

static ma_result vfsFileClose(ma_vfs *pVFS, ma_vfs_file file)
{
	File *f = (File *)file;
	if (!f) {
		return MA_INVALID_FILE;
	}

	f->close();
	return MA_SUCCESS;
}

static ma_result vfsFileInfo(ma_vfs *pVFS, ma_vfs_file file, ma_file_info *pInfo)
{
	File *f = (File *)file;
	if (!f) {
		return MA_INVALID_FILE;
	}

	pInfo->sizeInBytes = f->size();
	return MA_SUCCESS;
}

static ma_result vfsFileRead(ma_vfs *pVFS, ma_vfs_file file, void *pBuffer, size_t bytesToRead, size_t *pBytesRead)
{
	File *f = (File *)file;
	if (!f) {
		return MA_INVALID_FILE;
	}

	size_t bytesActuallyRead = f->read(pBuffer, bytesToRead);
	if (pBytesRead) {
		*pBytesRead = bytesActuallyRead;
	}

	return MA_SUCCESS;
}

static ma_result vfsFileTell(ma_vfs *pVFS, ma_vfs_file file, ma_int64 *pCursor)
{
	File *f = (File *)file;
	if (!f) {
		return MA_INVALID_FILE;
	}

	*pCursor = f->position();
	return MA_SUCCESS;
}

static ma_result vfsFileSeek(ma_vfs *pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
{
	File *f = (File *)file;
	if (!f) {
		return MA_INVALID_FILE;
	}

	switch (origin) {
		case ma_seek_origin_start:
			f->seek(offset, File::START);
			break;
		case ma_seek_origin_current:
			f->seek(offset, File::CURRENT);
			break;
		case ma_seek_origin_end:
			f->seek(offset, File::END);
			break;
		default:
			return MA_INVALID_FILE;
	}

	return MA_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) || defined(_INTERNAL)
void MiniAudioManager::dumpAllAssetsUsed()
{
}
#endif