
# Default is Linux pulseaudio
trix_linux_pulse: microphone_pulse speaker_pulse streamer_pulse list_pulse

microphone_pulse:
	g++ -Wall -D__LINUX_PULSE__  -o bin/trix_microphone src/TrixMicrophone.cpp  libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -lpthread -lortp -lpulse-simple -lpulse -lopus 

streamer_pulse:
	g++ -Wall -D__LINUX_PULSE__  -o bin/trix_streamer src/TrixStreamer.cpp  libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -lpthread -lortp -lpulse-simple -lpulse -lopus 

speaker_pulse:
	g++ -Wall -D__LINUX_PULSE__  -o bin/trix_speaker src/TrixSpeaker.cpp   libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -lpthread -lortp -lpulse-simple -lpulse -lopus

list_pulse:
	g++ -Wall -D__LINUX_PULSE__  -o bin/trix_list src/TrixList.cpp   libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -lpthread -lortp -lpulse-simple -lpulse -lopus


# Linux alsa
trix_linux_alsa: microphone_alsa streamer_alsa streamer_alsa list_alsa

microphone_alsa:	
	g++ -Wall -D__LINUX_ALSA__ -o bin/trix_microphone src/TrixMicrophone.cpp  libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -lasound -lpthread -lortp -lopus 

streamer_alsa:	
	g++ -Wall -D__LINUX_ALSA__ -o bin/trix_streamer src/TrixStreamer.cpp  libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -lasound -lpthread -lortp -lopus 

speaker_alsa:	
	g++ -Wall -D__LINUX_ALSA__ -o bin/trix_speaker src/TrixSpeaker.cpp  libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -lasound -lpthread -lortp -lopus 

list_alsa:
	g++ -Wall -D__LINUX_ALSA__ -o bin/trix_list src/TrixList.cpp   libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -lasound -lpthread -lortp -lopus 

# macOS Core Audio
trix_macos_core_audio: microphone_core_audio streamer_core_audio speaker_core_audio list_core_audio

microphone_core_audio:
	g++ -Wall -D__MACOSX_CORE__ -o bin/trix_microphone src/TrixMicrophone.cpp  libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -framework CoreAudio -framework CoreFoundation -lpthread -lortp -lopus
streamer_core_audio:
	g++ -Wall -D__MACOSX_CORE__ -o bin/trix_streamer src/TrixStreamer.cpp  libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -framework CoreAudio -framework CoreFoundation -lpthread -lortp -lopus
speaker_core_audio:
	g++ -Wall -D__MACOSX_CORE__ -o bin/trix_speaker src/TrixSpeaker.cpp  libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -framework CoreAudio -framework CoreFoundation -lpthread -lortp -lopus
list_core_audio:
	g++ -Wall -D__MACOSX_CORE__ -o bin/trix_list src/TrixList.cpp   libs/rtaudio-5.0.0/RtAudio.cpp  -Ilibs/rtaudio-5.0.0 -Ilibs/rtaudio-5.0.0 -framework CoreAudio -framework CoreFoundation -lpthread -lortp -lopus


clean:
	rm -f bin/trix*