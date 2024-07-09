
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                      VOICE Assistant - Demo (Code snippets, examples)                    ------------------
// ----------------                                      July 8, 2024                                          ------------------
// ------------------                                                                                          ------------------
// ------------------              Voice RECORDING with variable length [native I2S code]                      ------------------
// ------------------          TextToSpeech [using Open AI TTS or Google TTS, AUDIO.H library]                 ------------------
// ------------------                   SpeechToText [using Deepgram API service]                              ------------------
// ------------------                                                                                          ------------------
// ------------------                    HW: ESP32 with connected Micro SD Card                                ------------------
// ------------------                    SD Card: using VSPI Default pins 5,18,19,23                           ------------------
// ------------------                    Optional: I2S Amplifier (e.g. MAX98357), pins below                   ------------------
// ------------------------------------------------------------------------------------------------------------------------------


// *** HINT: in case of an 'Sketch too Large' Compiler Warning/ERROR in Arduino IDE (ESP32 Dev Module):
// -> select a larger 'Partition Scheme' via menu > tools: e.g. using 'No OTA (2MB APP / 2MB SPIFFS) ***


#define VERSION           "=== KALO Voice Assistant (last update: July 8, 2024) ========================"   

#include <WiFi.h>         // only included here
#include <SD.h>           // also needed in other tabs (.ino) 
#include <Audio.h>        // only needed for PLAYING Audio (via I2S Amplifier, e.g. MAX98357) with ..
                          // .. Audio.h library from Schreibfaul1: https://github.com/schreibfaul1/ESP32-audioI2S


// --- PRIVATE credentials -----

const char* ssid =        "...";          // ## INSERT your wlan ssid 
const char* password =    "...";          // ## INSERT your password  
const char* OPENAI_KEY =  "...";          // ## optionally (needed for Open AI voices): INSERT your OpenAI key
                                                                                     

#define AUDIO_FILE        "/Audio.wav"    // mandatory, filename for the AUDIO recording
#define WELCOME_FILE      "/Welcome.wav"  // optionally, 'Hello' file will be played once on start (e.g. a gong or voice)


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



// ------------------------------------------------------------------------------------------------------------------------------
void setup() 
{   
  // Initialize serial communication
  Serial.begin(115200); 
  Serial.setTimeout(100);    // 10 times faster reaction after CR entered (default is 1000ms)

  // Pin assignments:
  pinMode(pin_LED_RED, OUTPUT);  pinMode(pin_LED_GREEN, OUTPUT);  pinMode(pin_LED_BLUE, OUTPUT);
  pinMode(pin_RECORD_BTN, INPUT );  // use INPUT_PULLUP if no external PullUp connected ##
    
  // on INIT: walk 1 sec thru 3 RGB colors (RED -> GREEN -> BLUE), then stay on GREEN 
  led_RGB(LOW,HIGH,HIGH); delay (330);  led_RGB(HIGH,LOW,HIGH); delay (330);  led_RGB(HIGH,HIGH,LOW); delay (330); 
  led_RGB(HIGH,LOW,HIGH); // stay on GREEN  
  
  // Hello World
  Serial.println( VERSION  );  
   
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
    led_RGB(LOW,HIGH,HIGH);  // RED means recording
    delay(30);  // unbouncing & supressing button 'click' noise in begin of audio recording
    
    // Before we start any recording we stop any earlier Audio Out streams (forcing silent speaker)
    audio_play.stopSong();
        
    //Start Recording
    Record_Start(AUDIO_FILE);     
  }

  if (digitalRead(pin_RECORD_BTN) == HIGH)    // Recording not started yet .. OR stopped now (on release button)
  { 
    led_RGB(HIGH,LOW,HIGH);  // GREEN
        
    float recorded_seconds; 
    if (Record_Available( AUDIO_FILE, &recorded_seconds ))  //  true once when recording finalized (.wav file available)
    { 
      if ( recorded_seconds > 0.4 )   // short btn TOUCH (e.g. <0.4 secs) will be ignored (used for 'audio_play.stopSong' only)
      {
        // ## Demo 1 - PLAY recorded AUDIO file from SD file
        // Keep in mind: 8bit files can't be played currently with AUDIO.H, assuming an issue in audio.h library (?)
        // (8bit waves produce only loud (!) noise/distortions only with current AUDIO.H)
        
        // -> that's why commented out below (we use 8 bit wav files for fast file transfer to SpeechToTex service)
        // Proposal: try 16bit once (via #define BITS_PER_SAMPLE 16 in lib_audio_recording) to check your hardware .. 
        // .. uncomment once and listen to the record 
         
        /*audio_play.connecttoFS(SD, AUDIO_FILE );              // play your own recorded audio  
        while (audio_play.isRunning()) {audio_play.loop();}     // wait here until done (just for Demo purposes) */ 

        
        // ## Demo 2 [SpeechToText] - Transcript the Audio (waiting here until done) 
        led_RGB(HIGH,HIGH,LOW);  // BLUE
        
        String transcription = SpeechToText_Deepgram( AUDIO_FILE );  
        
        led_RGB(HIGH,LOW,HIGH);  // GREEN
        Serial.println(transcription);
        
        if (transcription != "")
        {
           // New text received -> short WHITE FLASH on LED as indicator (short OFF, then back to GREEN)
           led_RGB(LOW,LOW,LOW); delay(200); led_RGB(HIGH,HIGH,HIGH); delay(100); led_RGB(HIGH,LOW,HIGH);    
           
           // ## Demo 3 [TextToSpeech Google TTS] - Reapeat your own recorded sentence with standard Google TTS voices 
           // [Google TTS voices] call: "audio_play.connecttospeech( text.c_str(), language );"
           // Example: audio_play.connecttospeech( "How are you ?", "en-US");      
           // Languages, examples: Englisch: en-US, en-IN, en-BG, en-AU.., NL=nl-NL, nl-BE, de-DE, Thai=th-TH
           // more infos: https://cloud.google.com/text-to-speech/docs/voices
       
           // Just for Demo 3 purposes here: using this voice only if your recorded sentence includes the word "Google"
           // otherwise we use the more human sounding voices from Open AI only (Demo 4 below)
           
           if (transcription.indexOf("Google") != -1)
           {  Serial.println("Google TTS speaking: [" + transcription +"]");
              audio_play.connecttospeech( transcription.c_str(), "de-DE");   
              while (audio_play.isRunning()) // wait here until finished (just for Demo purposes, before we play Demo 4)
              { audio_play.loop();  
              } 
           }   
           
           // ## Demo 4 [TextToSpeech OpenAI] - Reapeat your sentence with 'human sounding' voices (6 voices by random)
           // [OpenAI voices] call: "audio_play.openai_speech(OPENAI_KEY, model, text.c_str(), voice, format, speed);"
           // Example: audio_play.openai_speech(OPENAI_KEY, "tts-1", "How are you ?", "shimmer", "mp3", "1");         
           // All voices are multilingual (!), available voices: alloy, echo, fable, onyx, nova, shimmer
           // More info: https://platform.openai.com/docs/guides/text-to-speech/text-to-speech
           // keep in mind: registration needed, enter your personal key in header #define OPENAI_KEY
    
           String Voices[6] = { "alloy", "echo", "fable", "onyx", "nova", "shimmer" };
           int random_voice = random(5);
           Serial.println( "OpenAI '" + Voices[random_voice] + "' speaking: [" + transcription +"]");
           audio_play.openai_speech(OPENAI_KEY, "tts-1", transcription.c_str(), Voices[random_voice], "mp3", "1");  


           // ## Demo 5 - Trigger any ESP32 actions via voice (example: say ".. give me a rainbow ..")
           // -> triggering the internal LED's
           if (transcription.indexOf("rainbow") != -1 || /*German*/ transcription.indexOf("Regenbogen") != -1)
           {  for(int i=0;i<4;i++) for(int r=0;r<2;r++) for(int g=0;g<2;g++) for(int b=0;b<2;b++) {led_RGB(r,g,b); delay(100);}
           }

           // ## Demo 6 - Playing Internet live streams, triggered by keyword (example: speak a sentence with "Radio" included)
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
 
  // Schreibfaul1 loop für Play Audio
  audio_play.loop();
    
}




// ------------------------------------------------------------------------------------------------------------------------------

void led_RGB( bool red, bool green, bool blue ) 
{ static bool red_before, green_before, blue_before;  
  // writing to real pin only if changed (increasing performance for frequently repeated calls)
  if (red   != red_before)   { digitalWrite(pin_LED_RED,red); red_before=red; } 
  if (green != green_before) { digitalWrite(pin_LED_GREEN,green); green_before=green; } 
  if (blue  != blue_before)  { digitalWrite(pin_LED_BLUE,blue); blue_before=blue; } 
}
