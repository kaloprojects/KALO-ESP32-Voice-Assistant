# Summary
Code snippets showing how to _record I2S audio_ and store as .wav file on ESP32 with SD card, how to transcribe pre-recorded audio via _STT (SpeechToText)_ Deepgram API, how to generate audio from text via _TTS (TextToSpeech)_ API from OpenAI a/o Google TTS. Triggering ESP32 actions via Voice.

The repository contains the Demo main sketch  'KALO_ESP32_Voice_Assistant.ino', demonstrating different use case of my libraries 'lib_audio_recording.ino' and 'lib_audio_transcription.ino'  

# Features
Explore the demo use case examples (1-6) in main sketch, summary:
- Recording and playing audio are working offline, online connection needed for STT, TTS and streaming services
- Recording Voice Audio with variable length (recording as long a button is pressed), storing as .wav file (with 44 byte header) on SD card  
- Replay your recorded audio (using Schreibfaul1 <audio.h> library) 
- Playing Audio streams (e.g. playing music via radio streams with <audio.h> library)
- STT (SpeechToText), using Deepgram API service (registration needed)  
- TTS (TextToSpeech), supporting multilingual 6 voices via Open AI API (registration needed)
- TTS (TextToSpeech), using Google TTS API (no registration needed)  
- Triggering ESP actions via voice (e.g. triggering GPIO LED pins, addressing dedicated voices by calling their name, playing music on request)

# Hardware
- ESP32 development board (e.g. ESP32-WROOM-32), connected to Wifi
- I2S digital microphone, e.g. INMP441 [I2S pins 22, 33, 35]          
- I2S audio amplifier, e.g. MAX98357A [I2S pins 25,26,27] with speaker
- Micro SD Card [VSPI Default pins 5,18,19,23] 
- RGB LED (status indicator) and Analog Poti (audio volume)

# Installation & Customizing
- Required: Arduino IDE with ESP32 libray 3.0.x (based on ESP-IDF 5.1). Older 2.x ESP framework fail because new I2S driver missed
- Required (for playing Audio on ESP32): AUDIO.H library [ESP32-audioI2S.zip](https://github.com/schreibfaul1/ESP32-audioI2S). Install latest zip  (3.0.11g from July 18, 2024 or newer)
- Copy all 3 .ino files of 'KALO-ESP32-Voice-Assistant' into same folder (it is one sketch, split into 3 Arduino IDE tabs)
- Update your pin assignments in the header of all 3 .ino files
- Insert your credentials (ssid, password, OpenAI API key, Deepgram API key)
- Define your favorite recording settings (SAMPLE_RATE, BITS_PER_SAMPLE, GAIN_BOOSTER_I2S) in lib_audio_recording.ino header
- Define your language settings (Google TTS in KALO_ESP32_Voice_Assistant.ino, Deepgram STT in lib_audio_transcription.ino header)
- Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage)

# Known issues
- WifiClientSecure connection not reliable (assuming RAM heap issue in WifiClientSecure.h library), rarely freezing (e.g. after 10 mins)

# Updates
- 2024-07-22: Misc. enhancements, STT connection reliablility improved further, code cleaned up
- 2024-07-14: Updated version:
  - WifiClientSecure connection reliablility improved (still not perfect) 
  - STT Deepgram response faster (new total response time average on e.g. 5 sec voice record: ~ 2.5 sec). Recommendation: It's worth trying 8Khz/8bit once, STT response ~1 sec (Note: Using complete sentences instead of single words improves recognition quality)
  - user language settings (STT & TTS) added, bug fixing etc.
- 2024-07-08: First drop, already working, not finally cleaned up (just posted this drop on some folks request)

# Next steps
- Code cleanup, regular updates .. ongoing
- Review & improve reliability of WifiClientSecure connection .. ongoing
- Fixing 'Play 8bit audio' issue - Done (2024-07-18), latest AUDIO.H (since 2024-07-18) supports 8bit wav format
- Adding more use case examples in main sketch
- Including _SpeechGen.IO_ TTS API call (hundreds of additional voices). Coded already, unfortunaltly failed since ESP 3.x framework update
- Including a _OpenAI API library_ with demo code, using an ESP32 as _Voice ChatGPT_ device


.
.
.

# Demo Videos
Short video clip, presenting Recording & SpeechToText & TextToSpeech (without Open AI, ESP32 is not 'answering', just parroting my voice). Workflow: 
- Recording user voice, storing audio .wav file (8KHz/8bit) to SD card,
- STT: transcribe pre-recorded voice via Deepgram API,
- TTS: repeat spoken sentence with Goggle TTS voice (a/o triggering e.g. LED via voice):

[![Video Screenshot](https://github.com/user-attachments/assets/038905a8-3064-44c1-8eb7-e14cc6da94ab)](https://dark-controller.com/wp-content/uploads/2024/07/KALO_VoiceAssitant_Video01.mp4)

<br>
Featured video from other users & friends:<br>
@techiesms: using my Deepgram transcription STT library in his IoT projects: <br>https://www.youtube.com/watch?v=j0EEFXmikvk- 

