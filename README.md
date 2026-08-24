
**... the libraries in this repository are outdated, you will find latest lib_xy() versions [here](https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends)**

# Summary
Code snippets showing how to _record I2S audio_ and store as .wav file on ESP32 with SD card, how to _transcribe_ pre-recorded audio via _STT (SpeechToText)_ Deepgram API or (new) 60db.ai API, how to _generate audio_ from text via _TTS (TextToSpeech)_ API from Google TTS or OpenAI TTS or SpeechGen.IO or (new) 60db.ai. Triggering ESP32 actions via Voice.

The repository contains the Demo main sketch  'KALO_ESP32_Voice_Assistant.ino', demonstrating different use case of my libraries 'lib_audio_recording.ino', 'lib_audio_transcription.ino', 'lib_TTS_SpeechGen.ino' and (new) 'lib_60db.ino' 

# Features
Explore the demo use case examples in main sketch, summary:
- Recording and playing audio are working offline, online connection needed for STT, TTS and streaming services
- Recording Voice Audio with variable length (recording as long a button is pressed), storing as .wav file (with 44 byte header) on SD card  
- Replay your recorded audio (using Schreibfaul1 <audio.h> library) 
- Playing Audio streams (e.g. playing music via radio streams with <audio.h> library)
- Triggering ESP actions via voice (e.g. triggering GPIO LED pins, addressing dedicated voices by calling their name, playing music on request)
- STT (SpeechToText): Deepgram API service (registration needed)  
- STT (SpeechToText): #NEW#: 60db.ai API service (registration needed)  
- TTS (TextToSpeech): Google TTS free API (no registration needed)  
- TTS (TextToSpeech): Open AI API (6 multilingual voices, registration needed)
- TTS (TextToSpeech): SpeechGen.IO voices (many voices, not free, payment needed)  
- TTS (TextToSpeech): #NEW#: 60db.ai voices (cloned & professional voices, registration needed)  
- Provider selection: switch STT (Deepgram <-> 60db.ai) and default TTS (OpenAI <-> 60db.ai) via simple #define toggles  

# Hardware
- ESP32 development board (e.g. ESP32-WROOM-32), connected to Wifi
- I2S digital microphone, e.g. INMP441 [I2S pins 22, 33, 35]          
- I2S audio amplifier, e.g. MAX98357A [I2S pins 25,26,27] with speaker
- Micro SD Card [VSPI Default pins 5,18,19,23] 
- RGB LED (status indicator) and Analog Poti (audio volume)

# Installation & Customizing
- Required (Jan. 2025): Arduino IDE with ESP32 lib 3.1.x (based on ESP-IDF 5.3.x). Older 2.x ESP framework fails because new I2S driver missed
- Required (for playing Audio on ESP32): AUDIO.H library [ESP32-audioI2S.zip](https://github.com/schreibfaul1/ESP32-audioI2S). Install latest zip (3.0.11g from July 18, 2024 or newer)
- Copy all .ino files of 'KALO-ESP32-Voice-Assistant' into same folder (it is one sketch, split into multiple Arduino IDE tabs)
- Update your pin assignments & wlan settings (ssid, password) in the .ino header files
- Update headers with personal credentials (Deepgram API key, optional: OpenAI API key, SpeechGen Token, 60db.ai API key)
- For 60db.ai: insert your API key + a voice_id (UUID) in lib_60db.ino header; run Voices_60db() once to list your voice_id's
- Select the active engines in KALO_ESP32_Voice_Assistant.ino header: STT_ENGINE (ENGINE_DEEPGRAM/ENGINE_60DB) and TTS_ENGINE (ENGINE_OPENAI/ENGINE_60DB)
- Define your favorite recording settings (SAMPLE_RATE, BITS_PER_SAMPLE, GAIN_BOOSTER_I2S) in lib_audio_recording.ino header
- Define your language settings (Google TTS in KALO_ESP32_Voice_Assistant.ino, Deepgram STT in lib_audio_transcription.ino header, 60db STT in lib_60db.ino header)
- Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage)

# 60db.ai integration (lib_60db.ino)
The new 'lib_60db.ino' library mirrors the KALO Deepgram coding style (raw WiFiClientSecure, manual HTTPS, json_object() parser) so it runs 'along with' Deepgram, fully selectable via #define toggles. It adds both STT and TTS:
- STT: `SpeechToText_60db(file)` -> `POST /stt` (multipart/form-data upload of the recorded .wav), parses the `"text"` field, returns the transcript
- TTS: `TextToSpeech_60db(text, "/tts_60db.wav")` -> `POST /tts-synthesize` (output_format = wav). The Base64 audio is stream-decoded straight to the SD card (never buffered fully in RAM), then played via Audio.h `connecttoFS()`
- Voices: `Voices_60db()` -> `GET /myvoices`, prints your available voice_id (UUID) list to Serial (run once to pick a voice)
- Reliability: a small de-chunking reader transparently unwraps HTTP 'Transfer-Encoding: chunked' responses (from the 60db API gateway), so large Base64 TTS payloads decode cleanly
- Host: api.60db.ai:443, Auth header: 'Bearer <api-key>'. API docs: https://docs.60db.ai
- Note: 60db's streaming TTS (NDJSON /tts-stream) and WebSocket API are intentionally not used here (they don't fit the Audio.h playback model on ESP32); the synchronous /tts-synthesize path is used instead

# Known issues
- Earlier WifiClientSecure connection issues seem solved (with KALO 2025-01-06 Update & arduino-esp32 3.1.x)
- Google TTS support short sentences only (Google limitation), non-free services (OpenAI and SpeechGen.IO) are not limited in length.
- TTS: Only OpenAI voices are multi-lingual (supporting multiple languages in same request), Google & SpeechGen.IO request language specific parameter/voices

# Updates
- 2026-06-12: NEW library 'lib_60db.ino' adding 60db.ai STT + TTS, running alongside Deepgram (selectable via #define toggle)
- 2025-01-06: NEW library for TTS SpeechGen.IO (hundreds of voices) 
- 2025-01-06: Cleaned code, connection reliability issues solved, response time improved
- 2024-07-22: Misc. enhancements, WifiClientSecure reliability workarounds, code cleaned up
- 2024-07-18: 'Play 8bit audio' issue solved (latest AUDIO.H support 8bit wav format)
- 2024-07-14: WifiClientSecure connection reliability improved (still not perfect)
- 2024-07-14: STT Deepgram response faster (typical response time on e.g. 5 sec voice record: ~ 2.5 sec).
- 2024-07-08: First drop, already working, not finally cleaned up (just posted this drop on some folks request)

# Next steps
- Currently no major updates planned, enjoy the libraries :)
- I will add another project (repository) soon: 'ESP32 Voice OpenAI ChatGPT device' (using same C libraries). <br>2025-01-28 Update: Done. See repository: https://github.com/kaloprojects/KALO-ESP32-Voice-ChatGPT


.
.
.

# Demo Videos
Short video clip, presenting Recording & SpeechToText & TextToSpeech (without Open AI, ESP32 is not 'answering', just parroting my voice with free Google TTS voice). Workflow: 
- Recording user voice, storing audio .wav file (8KHz/8bit) to SD card,
- STT: transcribe pre-recorded voice via Deepgram API,
- TTS: repeat spoken sentence with Goggle TTS voice (a/o triggering e.g. LED via voice):

[![Video Screenshot](https://github.com/user-attachments/assets/038905a8-3064-44c1-8eb7-e14cc6da94ab)](https://dark-controller.com/wp-content/uploads/2024/07/KALO_VoiceAssitant_Video01.mp4)


.
.
.

Featured video from other users & friends (using my libraries in their IoT projects):
- @techiesms (August 2024): [FASTEST! Speech to Text Conversion using ESP32 Board](https://www.youtube.com/watch?v=j0EEFXmikvk)
- @techiesms (August 2024): [Portable AI Voice Assistant using ESP32 & Gemini AI](https://www.youtube.com/watch?v=zvR9DTfMwPE)


