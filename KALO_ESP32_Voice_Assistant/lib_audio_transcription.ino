
// ------------------------------------------------------------------------------------------------------------------------------
// ----------------           KALO Library - Deepgram SpeechToText API call with ESP32 & SD Card                 ---------------- 
// ----------------                                      July 14, 2024                                           ----------------
// ----------------                                                                                              ---------------- 
// ----------------            Coded by KALO (with support from Sandra, Deepgram team, June 2024)                ----------------
// ----------------      workflow: sending https POST message request, sending message WAV bytes in chunks       ----------------
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

#ifndef DEBUG                   // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG true            // <- define your preference here  
#  define DebugPrint(x);        if(DEBUG){Serial.print(x);}   /* do not touch */
#  define DebugPrintln(x);      if(DEBUG){Serial.println(x);} /* do not touch */ 
#endif


// --- PRIVATE credentials & user favorites -----  

const char* deepgramApiKey =     "...";   // ## INSERT your Deepgram credentials !

#define STT_LANGUAGE       "en"  // forcing single language: e.g. "de" (German), reason: improving recognition quality
                                 // keep EMPTY ("") if you want Deepgram to detect & understand 'your' language automatically, 
                                 // language abbreviation examples: "en", "en-US", "en-IN", "de" etc.
                                 // all see here: https://developers.deepgram.com/docs/models-languages-overview

#define STT_KEYWORDS            "&keywords=KALO&keywords=techiesms"      // optional, forcing Deepgram to listen exactly

#define TIMEOUT_DEEPGRAM   8    // max. waiting time [sec] for Deepgram transcription response     



// --- global vars -------------
WiFiClientSecure client;       



// ----------------------------------------------------------------------------------------------------------------------------

String SpeechToText_Deepgram( String audio_filename )
{ 
  uint32_t t_start = millis(); 
  
  // ---------- Connect to Deepgram Server (only if needed, e.g. on INIT and after lost connection)

  if (!client.connected())
  {
    DebugPrintln("> Initialize Deepgram Server connection ... ");
    client.setInsecure();
    if (!client.connect("api.deepgram.com", 443)) 
    { Serial.println("\nERROR - WifiClientSecure connection to Deepgram server failed!");
      return ("");
    }
    DebugPrintln("Done. Connected to Deepgram Server.");
  }
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
  optional_param += (STT_LANGUAGE != "") ? ("&language="+(String)STT_LANGUAGE) : ("&detect_language=true");  // see #defines  
  optional_param += "&smart_format=true";         // applies formatting (Punctuation, Paragraphs, upper/lower etc ..) 
  optional_param += "&numerals=true";             // converts numbers from written to numerical format (works with 'en' only)
  optional_param += STT_KEYWORDS;                 // optionally too: keyword boosting on multiple custom vocabulary words
  
  client.println("POST /v1/listen" + optional_param + " HTTP/1.1"); 
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String(deepgramApiKey));
  client.println("Content-Type: audio/wav");
  client.println("Content-Length: " + String(audio_size));
  client.println();   // header complete, now sending binary body (wav bytes) .. 
  
  DebugPrintln("> POST Request to Deepgram Server started, sending WAV data now ..." );
  uint32_t t_headersent = millis();     

  
  // ---------- Reading the AUDIO wav file, sending in CHUNKS (closing file after done)
  // idea found here (WiFiClientSecure.h issue): https://www.esp32.com/viewtopic.php?t=4675
  
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
  uint32_t t_wavbodysent = millis();  


  // ---------- Waiting & Receiving Deepgram Server response (stop waiting latest after TIMEOUT_DEEPGRAM [secs])
 
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
  { Serial.print("ERROR - forced TIMEOUT after " + (String) TIMEOUT_DEEPGRAM + " seconds");
    Serial.println(", is your Deepgram API Key valid ?\n");
  }  
  uint32_t t_response = millis();  


  // ---------- closing connection to Deepgram 
  client.stop();     // observation: unfortunately needed, otherwise the 'audio_play.openai_speech() in AUDIO.H not working !
                     // so we close for now, but will be opened again on next call (or regularly via in Deepgram_KeepAlive())
                     
    
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
  
  /* DebugPrintln( "-> Total Response: \n" + response + "\n");   // ### uncomment to see the complete server response */ 
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



// ----------------------------------------------------------------------------------------------------------------------------

void Deepgram_KeepAlive()       // should be called each 5 seconds about (to overcome the default autoclosing after 10 secs)
{
  uint32_t t_start = millis(); 
  
  // ---------- Connect to Deepgram Server (if not existing or failed before)
  DebugPrint( "*** PING Deepgram 'KeepAlive' - " );
  if ( !client.connected() )
  { DebugPrint("NEW Reconnection - ");
    client.setInsecure();
    if (!client.connect("api.deepgram.com", 443)) 
    { Serial.println("\n*** PING Error - Server Connection failed.");
      client.flush(); client.stop();  // for worst case (not sure this helps)   
      return;
    }  
    DebugPrint("Done. Succesfully connected. ");
  } 

  /* Deepgram does support a dedicated KeepAlive feature, but unfortunately not for pre-recorded audio
  // for streaming we could use this snippet:
  String payload = "{\"type\": \"KeepAlive\"}";   
  client.println("POST /v1/listen HTTP/1.1");
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String(deepgramApiKey));
  client.println("Content-Type: application/json; charset=utf-8");
  client.println("Content-Length: " + payload.length());
  client.println();   
  client.println( payload );
  // so for 'prerecorded audio' we use our own workflow below: ... */
   
  // we build and send a dummy 16Khz/8bit audio wav with header (20 values only, 0x80 for silence)
  // [0x40,0x00,0x00,0x00] = filesize 64 bytes,  [0x14,0x00,0x00,0x00] = 20 wav values (~ 1 millisec audio only)
  
  uint8_t empty_wav[] = {
  0x52,0x49,0x46,0x46, 0x40,0x00,0x00,0x00, 0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20, 
  0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x80,0x3E,0x00,0x00,0x80,0x3E,0x00,0x00,
  0x01,0x00,0x08,0x00,0x64,0x61,0x74,0x61, 0x14,0x00,0x00,0x00, 0x80,0x80,0x80,0x80, 
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80 }; 
  
  client.println("POST /v1/listen HTTP/1.1"); 
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String(deepgramApiKey));
  client.println("Content-Type: audio/wav");
  client.println("Content-Length: " + String(sizeof(empty_wav)));
  client.println(); // header complete, now sending wav bytes .. 
  client.write(empty_wav, sizeof(empty_wav)); 

  String response = "";
  while (response == "")    
  { while (client.available()) { char c = client.read(); response += String(c); }        
  } 
  
  /*DebugPrintln( response );  */
  DebugPrintln( "Total Latency [sec]: " + (String) ((float)((millis()-t_start))/1000) );   
  
}
