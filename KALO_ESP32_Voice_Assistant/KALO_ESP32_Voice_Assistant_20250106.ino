
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                      VOICE Assistant - Demo (Code snippets, examples)                    ------------------
// ----------------                                    Update: Jan 6, 2025                                     ------------------
// ------------------                                                                                          ------------------
// ------------------               Voice RECORDING with variable length [KALO I2S code]                       ------------------
// ------------------              TextToSpeech [free Google TTS | Open AI | Speechgen.IO]                     ------------------
// ------------------                    SpeechToText [using Deepgram API service]                             ------------------
// ------------------                                                                                          ------------------
// ------------------                    HW: ESP32 with connected Micro SD Card                                ------------------
// ------------------                    SD Card: using VSPI Default pins 5,18,19,23                           ------------------
// ------------------                    Optional: I2S Amplifier (e.g. MAX98357), pins below                   ------------------
// ------------------------------------------------------------------------------------------------------------------------------


// *** HINT: in case of an 'Sketch too Large' Compiler Warning/ERROR in Arduino IDE (ESP32 Dev Module):
// -> select a larger 'Partition Scheme' via menu > tools: e.g. using 'No OTA (2MB APP / 2MB SPIFFS) ***


#define VERSION           "\n=== KALO ESP32 Voice Assistant (last update: Jan. 6, 2025) ======================"   

#include <WiFi.h>         // only included here
#include <SD.h>           // also needed in other tabs (.ino) 

#include <Audio.h>        // needed for PLAYING Audio (via I2S Amplifier, e.g. MAX98357) with ..
                          // Audio.h library from Schreibfaul1: https://github.com/schreibfaul1/ESP32-audioI2S
                          // > ensure you have actual version (July 18, 2024 or newer needed for 8bit wav files!)
                          // > [reason: bug fix for 8bit https://github.com/schreibfaul1/ESP32-audioI2S/issues/786]
                          

// --- PRIVATE credentials -----

const char* ssid =        "...";          // ### INSERT your wlan ssid
const char* password =    "...";          // ### INSERT your password
const char* OPENAI_KEY =  "...";          // ### INSERT your OpenAI key
// (60db.ai API key + voice_id are defined in lib_60db.ino header)

// --- user preferences --------

#define AUDIO_FILE        "/Audio.wav"    // mandatory, filename for the AUDIO recording
#define WELCOME_FILE      "/Welcome.wav"  // optionally, 'Hello' file will be played once on start (e.g. a gong or voice)
#define TTS_60DB_FILE     "/tts_60db.wav" // temp file: 60db synthesized speech is decoded to SD here, then played

#define TTS_GOOGLE_LANGUAGE   "en"        // needed for Google TTS voices only (not needed for multilingual OpenAI voices :)
                                          // examples: en-US, en-IN, en-BG, en-AU, nl-NL, nl-BE, de-DE, th-TH etc.
                                          // more infos: https://cloud.google.com/text-to-speech/docs/voices

// --- Provider selection (run 60db 'along with' Deepgram, pick the active engine here) --------------------------------
#define ENGINE_DEEPGRAM   1               // STT only
#define ENGINE_OPENAI     2               // TTS only
#define ENGINE_60DB       3               // STT + TTS (60db.ai)

#define STT_ENGINE        ENGINE_60DB     // ### choose SpeechToText engine: ENGINE_DEEPGRAM or ENGINE_60DB
#define TTS_ENGINE        ENGINE_60DB     // ### choose default speak-back engine: ENGINE_OPENAI or ENGINE_60DB

// --- PIN assignments ---------

#define pin_RECORD_BTN    36   
#define pin_VOL_POTI      34    

#define pin_LED_RED       15    
#define pin_LED_GREEN     2
#define pin_LED_BLUE      0      

#define pin_I2S_DOUT      25    // 3 pins for I2S Audio Output (Schreibfaul1 audio.h Library)
#define pin_I2S_LRC       26
#define pin_I2S_BCLK      27


// --- global Objects ----------

Audio audio_play;


// declaration of functions in other modules (not mandatory but ensures compiler checks correctly)
// splitting Sketch into multiple tabs see e.g. here: https://www.youtube.com/watch?v=HtYlQXt14zU

bool    I2S_Record_Init(); 
bool    Record_Start( String filename ); 
bool    Record_Available( String filename, float* audiolength_sec ); 

String  SpeechToText_Deepgram( String filename );
void    Deepgram_KeepAlive();

String  SpeechToText_60db( String filename );                       // 60db.ai STT  (lib_60db.ino)
String  TextToSpeech_60db( String text, String out_filename );      // 60db.ai TTS  (lib_60db.ino) -> returns SD path or ""
void    Voices_60db();                                              // 60db.ai: optional, print your voice_id list



// ------------------------------------------------------------------------------------------------------------------------------
void setup() 
{   
  // Initialize serial communication
  Serial.begin(115200); 
  Serial.setTimeout(100);    // 10 times faster reaction after CR entered (default is 1000ms)

  // Pin assignments:
  pinMode(pin_LED_RED, OUTPUT);  pinMode(pin_LED_GREEN, OUTPUT);  pinMode(pin_LED_BLUE, OUTPUT);
  pinMode(pin_RECORD_BTN, INPUT );  // use INPUT_PULLUP if no external Pull-Up connected ##
    
  // on INIT: walk 1 sec thru 3 RGB colors (RED -> GREEN -> BLUE), then stay on GREEN 
  led_RGB(LOW,HIGH,HIGH); delay (330);  led_RGB(HIGH,LOW,HIGH); delay (330);  led_RGB(HIGH,HIGH,LOW); delay (330); 
  led_RGB(HIGH,LOW,HIGH); // stay on GREEN  
  
  // Hello World
  Serial.println( VERSION );  
   
  // Connecting to WLAN
  WiFi.mode(WIFI_STA);                                 
  WiFi.begin(ssid, password);         
  Serial.print("Connecting WLAN " );
  while (WiFi.status() != WL_CONNECTED)                 
  { Serial.print(".");  delay(500); 
  } 
  Serial.println(". Done, device connected.");
  led_RGB( HIGH,LOW,HIGH );   // GREEN 

  // Initialize SD card
  if (!SD.begin()) 
  { Serial.println("ERROR - SD Card initialization failed!"); 
    return; 
  }
  
  // initialize KALO I2S Recording Services (don't forget!)
  I2S_Record_Init();        
    
  // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT);
  
  // Say 'Hello' - Playing a optional WELCOME_FILE wav file once
  if ( SD.exists( WELCOME_FILE ) )
  {  audio_play.setVolume( map(analogRead(pin_VOL_POTI),0,4095,0,21) );  
     audio_play.connecttoFS( SD, WELCOME_FILE );  
     // using this 'isRunning()' trick to wait in setup() until PLAY is done:
     while (audio_play.isRunning())
     { audio_play.loop();
     }     
  }
  
  // INIT done, starting user interaction
  Serial.println( "> HOLD button for recording AUDIO .. RELEASE button for REPLAY & Deepgram transcription" );  
}



// ------------------------------------------------------------------------------------------------------------------------------
void loop() 
{
  if (digitalRead(pin_RECORD_BTN) == LOW)     // Recording started (ongoing)
  { 
    led_RGB(LOW,HIGH,HIGH);  //  RED means 'Recording ongoing'
    delay(30);  // unbouncing & suppressing button 'click' noise in begin of audio recording
    
    // Before we start any recording we stop any earlier Audio Output or streaming (e.g. radio)
    if (audio_play.isRunning())
    {  audio_play.connecttohost("");    // 'audio_play.stopSong()' wouldn't be enough (STT wouldn't reconnect)
    }
           
    //Start Recording
    Record_Start(AUDIO_FILE);     
  }

  if (digitalRead(pin_RECORD_BTN) == HIGH)    // Recording not started yet .. OR stopped now (on release button)
  { 
    led_RGB(HIGH,LOW,HIGH);  // GREEN means: 'Ready for recording'
        
    float recorded_seconds; 
    if (Record_Available( AUDIO_FILE, &recorded_seconds ))  //  true once when recording finalized (.wav file available)
    { 
      if ( recorded_seconds > 0.4 )   // ignore short btn TOUCH (e.g. <0.4 secs, used for 'audio_play.stopSong' only)
      {
        // ## Demo 1 - PLAY your own recorded AUDIO file (from SD card)
        // Hint to 8bit: you need AUDIO.H library from July 18,2024 or later (otherwise 8bit produce only loud (!) noise)
        // we commented out Demo 1 to jump to Demo 2 directly  .. uncomment once if you want to listen to your record !
        /*audio_play.connecttoFS(SD, AUDIO_FILE );              // play your own recorded audio  
        while (audio_play.isRunning()) {audio_play.loop();}     // wait here until done (just for Demo purposes)  */
        
        // ## Demo 2 [SpeechToText] - Transcript the Audio (waiting here until done)
        led_RGB(HIGH,HIGH,LOW);  // BLUE means: 'STT server creates transcription'

        #if STT_ENGINE == ENGINE_60DB
          String transcription = SpeechToText_60db( AUDIO_FILE );      // 60db.ai STT
        #else
          String transcription = SpeechToText_Deepgram( AUDIO_FILE );  // Deepgram STT
        #endif
        
        led_RGB(HIGH,LOW,HIGH);  // GREEN means: 'Ready for recording'
        Serial.println(transcription);
        
        if (transcription != "")      // we found spoken text .. now starting Demo examples:
        {
           // New text received -> short WHITE FLASH (200ms) on LED to indicate 'NEW text recognized)
           led_RGB(LOW,LOW,LOW); delay(200);                              // white flash
           led_RGB(HIGH,HIGH,HIGH); delay(100); led_RGB(HIGH,LOW,HIGH);   // short switch off, back to green
                      
           // ## Demo 3 [TextToSpeech Google TTS] - Repeat your spoken sentence with Google TTS voice 
           // [Google TTS voices] call: "audio_play.connecttospeech( text.c_str(), language );"
           // !! Be aware (observation): Free Google TTS supports short sentences only ! (OpenAI and SpeechGen are NOT limited)
           
           // Demo example: .. e.g. using this voice only if your recorded sentence includes the word "Google" ...
           if (transcription.indexOf("Google") != -1)
           {  Serial.println("Google TTS speaking: [" + transcription +"]");
              
              // Play TTS Google
              audio_play.connecttospeech( transcription.c_str(), TTS_GOOGLE_LANGUAGE);    
              
              while (audio_play.isRunning()) // wait here until finished (for Demo purposes, before playing again with OpenAI)
              { audio_play.loop();  
              } 
           }   

           // ## NEW ##: Demo 4 [TextToSpeech with Speechgen.IO] - Hundreds of voices, not free (one-time payments for packages)
           // Large list of voices, details here: https://speechgen.io/en/voices/
           // Voices support pitch & speed, but (in opposite to OpenAI) not multilingual (language specific, similar Google TTS)
           // keep in mind: payment needed, enter your SPEECHGEN_TOKEN + SPEECHGEN_EMAIL in lib_TTS-SpeechGen header (#define)

           // Demo example: .. e.g. using this voice only if your recorded sentence includes the word "Child" ...
           if (transcription.indexOf("Child") != -1 || transcription.indexOf("child") != -1)
           {
              String Example_Voice_US = "Anny";    // Child voice, use 'Gisela' instead to get same voice for German language
              String voice_pitch = "1";  // voice pitch [default 0], range from -20 to 20    
              String voice_speed = "1";  // voice playback speed [default 1], range from  0.1 to 2.0
              String emotion = "good";   // possible values: 'good' [default], 'evil', 'neutral' (hint: I couldn't hear difference)
              String mp3_url;

              // Play TTS Speechgen.IO: Generate mp3 file on SpeechGen.IO Server with KALO lib function 'voice_SpeechGen(..)' 
              mp3_url = voice_SpeechGen( transcription, Example_Voice_US.c_str(), voice_pitch, voice_speed, emotion );  
           
              // Play remote file (using AUDIO.H function for Streaming Server)
              audio_play.connecttohost( mp3_url.c_str() );  
              
              while (audio_play.isRunning()) // wait here until finished (for Demo purposes, before playing again with OpenAI)
              { audio_play.loop();  
              }               
           }
           
           // ## Demo 5 [TextToSpeech OpenAI] - Repeat your sentence with 'human sounding' voices (6 voices by random)
           // [OpenAI voices] call: "audio_play.openai_speech(OPENAI_KEY, model, text.c_str(), voice, format, speed);"
           // Example: audio_play.openai_speech(OPENAI_KEY, "tts-1", "How are you ?", "shimmer", "mp3", "1");         
           // All voices are multilingual (!), available voices: alloy, echo, fable, onyx, nova, shimmer
           // More info: https://platform.openai.com/docs/guides/text-to-speech/text-to-speech
           // keep in mind: OpenAI registration needed, enter your personal key in header #define OPENAI_KEY
    
           // Demo example [by DEFAULT always]: .. speaking back the transcription with the selected TTS_ENGINE
        #if TTS_ENGINE == ENGINE_60DB
           // 60db.ai TTS: synthesize -> base64 WAV decoded to SD -> play via Audio.h (consistent with the record/play path)
           Serial.println( "60db speaking: [" + transcription + "]" );
           if ( TextToSpeech_60db( transcription, TTS_60DB_FILE ) != "" )
           {  audio_play.connecttoFS( SD, TTS_60DB_FILE );   // non-blocking, played by audio_play.loop() at end of loop()
           }
        #else
           // OpenAI TTS: 'human sounding' multilingual voices (select one of the 6 voices by random)
           String Voices[6] = { "alloy", "echo", "fable", "onyx", "nova", "shimmer" };
           int random_voice = random(6);
           Serial.println( "OpenAI '" + Voices[random_voice] + "' speaking: [" + transcription +"]");

           // Play TTS OpenAI
           audio_play.openai_speech(OPENAI_KEY, "tts-1", transcription.c_str(), Voices[random_voice], "mp3", "1");
        #endif
           
           
           // ## Demo 6 - Trigger any ESP32 actions via voice (example: say ".. give me a rainbow ..")
           // -> triggering the internal LED's
           if (transcription.indexOf("rainbow") != -1 || /*German*/ transcription.indexOf("Regenbogen") != -1)
           {  for(int i=0;i<4;i++) for(int r=0;r<2;r++) for(int g=0;g<2;g++) for(int b=0;b<2;b++) {led_RGB(r,g,b); delay(100);}
           }
           
           // ## Demo 7 - Playing Internet live streams, triggered by keyword (example: speak a sentence with "Radio" included)
           // -> Play any Internet live streams
           if (transcription.indexOf("radio") != -1 || /*German*/ transcription.indexOf("Radio") != -1)
           { audio_play.connecttohost( "https://liveradio.swr.de/sw282p3/swr3/play.mp3" );                          
           }
           
        }
      }
    }      
  }  

  // check whether user changed Audio Volume Poti (only 5 times per second to avoid unnecessary calls or flickering)
  static long millis_before = millis();  
  static int  volume_before;
  if( millis() > (millis_before + 200))  // each 200ms
  { millis_before = millis(); 
    int volume = map( analogRead(pin_VOL_POTI), 0, 4095, 0, 21 );
    if (volume != volume_before)
    {  Serial.println( "Volume: " + (String) volume);
       audio_play.setVolume(volume);  // values from 0 to 21
       volume_before = volume;
    }    
  } 
 
  // Schreibfaul1 loop für Play Audio (details here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.loop();  
  vTaskDelay(1);
   
  
  /* [Optional]: Stabilize WiFiClientSecure.h + Improve Speed of STT Deepgram response (~1 sec faster)
  // Idea: Connect once, then sending each 5 seconds dummy bytes (to overcome Deepgram auto-closing 10 secs after last request)
  // ### meanwhile KeepAlive() no longer needed (WiFiClientSecure is stable, Deepgram speed improved), just kept as notice ### 

  if (digitalRead(pin_RECORD_BTN) == HIGH && !audio_play.isRunning() )  // but don't do it during recording or playing
  { static uint32_t millis_ping_before;  
    if( millis() > (millis_ping_before + 5000))  
    { millis_ping_before = millis(); 
      led_RGB(HIGH,HIGH,HIGH);    // short LED OFF means: 'Reconnection server, can't record in moment'
      Deepgram_KeepAlive();
    }
  }*/

}



// ------------------------------------------------------------------------------------------------------------------------------

void led_RGB( bool red, bool green, bool blue ) 
{ static bool red_before, green_before, blue_before;  
  // writing to real pin only if changed (increasing performance for frequently repeated calls)
  if (red   != red_before)   { digitalWrite(pin_LED_RED,red); red_before=red; } 
  if (green != green_before) { digitalWrite(pin_LED_GREEN,green); green_before=green; } 
  if (blue  != blue_before)  { digitalWrite(pin_LED_BLUE,blue); blue_before=blue; } 
}
