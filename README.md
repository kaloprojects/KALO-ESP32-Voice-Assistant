# Summary
Code snippets showing how to record I2S audio and store as .wav file on ESP32 with SD card, how to transcribe audio via Deepgram SpeechToText API, how to generate audio from text via TextToSpeech API from OpenAI a/o Google TTS. Triggering ESP32 actions via Voice.

The repository contains the Demo main sketch  'KALO_ESP32_Voice_Assistant.ino', demonstrating different use case of my libraries 'lib_audio_recording.ino' and 'lib_audio_transcription.ino'  

# Features
Explore the demo use case examples (1-6) in main sketch, summary:
- Recording Voice Audio with variable length (recording as long a button is pressed), storing as .wav file (with 44 byte header) on SD card  
- Replay your recorded audio (using Schreibfaul1 <audio.h> library) 
- Playing Audio streams (e.g. playing music via radio streams with Schreibfaul1 <audio.h> library)
- STT (SpeechToText), using Deepgram API service (registration needed)  
- TTS (TextToSpeech), supporting multilingual 6 voices via Open AI API (registration needed)
- TTS (TextToSpeech), using Google TTS API (no registration needed)  
- Triggering ESP actions via voice (e.g. triggering GPIO LED pins, addressing dedicated voices by calling their name, playing music on request)

# Hardware
- ESP32 development board (e.g. ESP32-WROOM-32)
- I2S digital microphone, e.g. INMP441 [I2S pins 22, 33, 35]          
- I2S audio amplifier, e.g. MAX98357A [I2S pins 25,26,27] with speaker
- Micro SD Card [VSPI Default pins 5,18,19,23] 
- RGB LED (status indicator) and Analog Poti (audio volume)

# Installation & Customizing
- No additional libraries needed, all header.h files are included in latest Arduino IDE (with ESP32 3.x) framework
- Copy all 3 .ino files into same folder (it is one sketch, split into 3 Arduino IDE tabs)
- Update your pin assignments in the header of all 3 .ino files
- Insert your credentials (ssid, password, OpenAI API key, Deepgram API key)
- Define your favorite recording settings (SAMPLE_RATE, BITS_PER_SAMPLE, GAIN_BOOSTER_I2S) in lib_audio_recording.ino header
- Define your Deepgram language in lib_audio_transcription.ino (optional_param += "&language=en-US")
- Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage)

# Updates
- 2024-07-08: First drop, already working, not finally cleaned up (just posted this drop on some folks request)

# Next steps
- Code cleanup, regular updates the upcoming days / weeks
- Improving performance to archive faster STT response (e.g. keeping connection Alive)
- Review & improve reliability of the WifiClientSecure connection
- Following up on 'Play 8bit audio' issue (Schreibfaul1 Audio.h library)
- Adding more use case examples in main sketch
- Including Speechgen.IO TTS API call (hundreds of additional voices)
