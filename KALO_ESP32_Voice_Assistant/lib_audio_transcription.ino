
// ------------------------------------------------------------------------------------------------------------------------------
// ----------------           KALO Library - Deepgram SpeechToText API call with ESP32 & SD Card                 ---------------- 
// ----------------                                      July 8, 2024                                            ----------------
// ----------------                                                                                              ---------------- 
// ----------------            Coded by KALO (with support from Sandra, Deepgram team, June 2024)                ----------------
// ----------------      worflow: sending https POST message request, sending message WAV bytes in chunks        ----------------
// ----------------                                                                                              ----------------   
// ----------------   CALL: 'text_response = SpeechToText_Deepgram(SD_audio_file)' [no Initialization needed]    ----------------
// ------------------------------------------------------------------------------------------------------------------------------


// *** HINT: in case of an 'Sketch too Large' Compiler Warning/ERROR in Arduino IDE (ESP32 Dev Module:
// -> select a larger 'Partition Scheme' via menu > tools: e.g. using 'No OTA (2MB APP / 2MB SPIFFS) ***

// Keep in mind: Deepgram SpeechToText services are AI based, means 'whole sentences with context' typically have a much 
// higher recognition quality (confidence) than a sending single words or short commands only (my observation).



// --- includes ----------------

#include <WiFiClientSecure.h>   // only here needed
/* #include <SD.h>              // library also needed, but already included in main.ino: */


// --- defines & macros --------

#ifndef DEBUG                   // user can define favorite behavior ('true' displays addition info)
#  define DEBUG true            // <- define your preference here  
#  define DebugPrint(x);        if(DEBUG){Serial.print(x);}   /* do not touch */
#  define DebugPrintln(x);      if(DEBUG){Serial.println(x);} /* do not touch */ 
#endif

#define TIMEOUT_DEEPGRAM   10   // max. waiting time [sec] for Deepgram transcrition response       


// --- global vars -------------

const char* deepgramApiKey =    "...";   // ## INSERT your Deepgram credentials !



// ----------------------------------------------------------------------------------------------------------------------------

String SpeechToText_Deepgram( String audio_filename )
{ 
  uint32_t t_start = millis(); 

  // ---------- Connect to Deepgram Server

  // my observations: in rare cases the 'client.connect()' call fails/freezes, seems to be an WifiClientSecure.h issue (?) 
  // So we declare a 'local client object' for now (increasing reliability) and added some lines of codes more.
  // Future plan: keep connection open always (no repeating connecting needed, also reducing latency further)
  // https://github.com/espressif/arduino-esp32/tree/ba8024c0d28d97cd02052bd178bf528fd2a3e576/libraries/WiFiClientSecure/src
  // https://forum.arduino.cc/t/bearssl-wificlientsecure-sometimes-freeze-at-client-connect/607962/2

  WiFiClientSecure client;              // local var, initialized on each call
  if (!client.connected())
  {  /* client.flush(); client.stop();  // tested, but did improve stability */
     client.setInsecure();              // encrypted without CA    
     client.setTimeout(4000);           // seems to be ignored either
     DebugPrintln("> Connecting to Deepgram Server ...");
     if (!client.connect("api.deepgram.com", 443))   
     {  Serial.println("ERROR - WifiClientSecure connection to Deepgram server failed!");
        return ("");  // return empty string (no transcription) 
     }   
  }
  DebugPrintln("> Connected to Deepgram Server.");
  uint32_t t_connected = millis();  
  
    
  // ---------- Check if AUDIO file exists, check file size 
  
  File audioFile = SD.open( audio_filename );    
  if (!audioFile) {
    Serial.println("ERROR - Failed to open file for reading");
    return ("");
  }
  size_t audio_size = audioFile.size();
  audioFile.close();
  DebugPrintln("> Audio File [" + audio_filename + "] found, size: " + (String) audio_size );


  // ---------- Sending HTTPS request header to Deepgram Server

  String optional_param;                          // see: https://developers.deepgram.com/docs/stt-streaming-feature-overview
  optional_param =  "?model=nova-2-general";      // Deepgram recommended model (high readability, lowest word error rates)
  optional_param += "&language=en-US";            // or e.g. 'en-IN' or 'de' etc. or alternative: "&detect_language=true"; 
                                                  // https://developers.deepgram.com/docs/models-languages-overview
  optional_param += "&smart_format=true";         // applies formatting (Punctuation, Paragraphs, upper/lower etc ..) 
  optional_param += "&numerals=true";             // converts numbers from written to numerical format (works with 'en' only)
  optional_param += "&keywords=KALO&keywords=techiesms";   // keyword boosting for (one or multiple) custom vocabulary words 

  client.println("POST /v1/listen" + optional_param + " HTTP/1.1"); 
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String(deepgramApiKey));
  client.println("Content-Type: audio/wav");
  client.println("Content-Length: " + String(audio_size));
  client.println();   // header complete, now sending binary body (wav bytes) .. 
  
  
  // ---------- Reading the AUDIO wav file, sending in CHUNKS (closing file after done)
  // idea found here (WiFiClientSecure.h issue): https://www.esp32.com/viewtopic.php?t=4675
  
  uint32_t t_headersent = millis();   
  DebugPrintln("> POST Request to Deepgram Server started, sending WAV data now ..." );
  File file = SD.open( audio_filename, FILE_READ );
  const size_t bufferSize = 1024; // we use a 1KB buffer 
  uint8_t buffer[bufferSize];
  size_t bytesRead;
  while (file.available()) 
  { bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead > 0) client.write(buffer, bytesRead);   // sending WAV AUDIO data       
  }
  file.close();
  DebugPrintln("> All bytes sent, waiting Deepgram transcription");


  // ---------- Waiting & Receiving Deepgram Server response (stop waiting latest after TIMEOUT_DEEPGRAM [secs])

  uint32_t t_wavbodysent = millis();  
  String response = "";
  while ( response == "" && millis() < (t_wavbodysent + TIMEOUT_DEEPGRAM*1000) )    
  { while (client.available())    
    { char c = client.read();
      response += String(c);      
    }
    // printing dots '.' each 100ms while waiting response 
    DebugPrint(".");  delay(100);           
  } 
  DebugPrintln();
  if (millis() >= (t_wavbodysent + TIMEOUT_DEEPGRAM*1000))
  { Serial.println("ERROR - forced TIMEOUT after " + (String) TIMEOUT_DEEPGRAM + " seconds");
  }  
  uint32_t t_response = millis();  


  // ---------- closing connection to Deepgram 
  client.stop();     // might be removed in future (keeping 'WifiClientSecure' AND Deepgram connection open always)
  
  
  // ---------- Parsing json response, extracting transcription (and detected language)
  
  int pos_start, pos_end;
  String language, transcription;

  String json_DetectLang_Start = "\"detected_language\":";
  String json_DetectLang_End   = "\"language_confidence\":";
  pos_start = response.indexOf(json_DetectLang_Start);      
  if (pos_start > 0) 
  {  pos_start += json_DetectLang_Start.length()+1;      
     pos_end = response.indexOf(json_DetectLang_End, pos_start);   
     if (pos_end > pos_start) {language = response.substring(pos_start,pos_end-2);} 
  } 
  
  String json_Transcript_Start = "\"transcript\":";
  String json_Transcript_End   = "\"confidence\":";
  pos_start = response.indexOf(json_Transcript_Start);      
  if (pos_start > 0) 
  {  pos_start += json_Transcript_Start.length()+1;      
     pos_end = response.indexOf(json_Transcript_End, pos_start);   
     if (pos_end > pos_start) {transcription = response.substring(pos_start,pos_end-2);}
  }  

  DebugPrintln( "-------------------------------------------------" );
  /* DebugPrintln( "-> Total Response: \n" + response + "\n");  // uncomment if you want to see the complete server response */ 
  DebugPrintln( "-> Latency Server (Re)CONNECT [t_connected]: " + (String) ((float)((t_connected-t_start))/1000) );;   
  DebugPrintln( "-> Latency sending HEADER [t_headersent]:    " + (String) ((float)((t_headersent-t_connected))/1000) );   
  DebugPrintln( "-> Latency sending WAV file [t_wavbodysent]: " + (String) ((float)((t_wavbodysent-t_headersent))/1000) );   
  DebugPrintln( "-> Latency DEEPGRAM response [t_response]:   " + (String) ((float)((t_response-t_wavbodysent))/1000) );   
  DebugPrintln( "=> TOTAL Duration [sec]: ................... " + (String) ((float)((t_response-t_start))/1000) ); 
  if (language != "") {DebugPrintln( "=> Detected language: [" + language + "]" ); }
  DebugPrintln( "=> Transcription: #" + transcription + "#" );
  DebugPrintln( "-------------------------------------------------\n" );

  
  // ---------- return transcription String 
  return transcription;    
}




/* Future plans(idea: ping Deepgram e.g. every 3 seconds to keep connection open always (faster transcriptions & more reliable) 
// details: https://developers.deepgram.com/docs/keep-alive

void Deepgram_Ping_KeepAlive()
{ ...
  String ping = "{\"type\": \"KeepAlive\"}"; 
  client.println("POST /v1/listen HTTP/1.1");
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String(deepgramApiKey));
  client.println("Content-Type: application/json; charset=utf-8");
  client.println("Content-Length: " + ping.length());
  client.println();
  client.println(ping);
  ...
} */
