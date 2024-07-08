# Code Samples
This repository contains the main sketch KALO_ESP32_Voice_Assistant.ino, demonstrating different use case examples, using my libraries lib_audio_recording.ino and lib_audio_transcription.ino.  

# Features
See use cases (Demo 1-6) in main sketch 

# Hardware
- ESP32 development board (e.g. ESP32-WROOM-32)
- INMP441 I2S digital microphone (I2S pins 22, 33, 35]          
- MAX98357A I2S digital audio amplifier [I2S pins 25,26,27]
- Micro SD Card [VSPI Default pins 5,18,19,23] 
- RGB LED (status indicator) and Analog Poti (audio volume)

# Installation & Customizing
- No additional libraries needed, all used header.h files should be included in latest Arduino IDE (with ESP32 3.x) framework.
- Copy all 3 .ino files into same folder (it is one sketch split into 3 Arduino IDE tabs)
- Update your pin assignments in the header of all 3 .ino files
- Insert your credentials (ssid, password, OpenAI- API key, Deepgram API key)
- Define your favorite recording settings (SAMPLE_RATE, BITS_PER_SAMPLE, GAIN_BOOSTER_I2S)
- Toggle DEBUG flag to true (displaying Serial.print details) or false (for final usage)

# Updates
- 2024-07-08: First drop, already working, not finally cleaned up (just posted this drop on some folks request)

# Next steps
- Code cleanup, regular updates the upcoming days / weeks
- Improving performance to archive faster STT response (e.g. keeping connection Alive)
- Review & improve reliability of the WifiClientSecure connection
- Following up on 'Play 8bit audio' issue (up with Schreibfaul1 Audio.h library)
- Adding more use case examples in main sketch
