
// ------------------------------------------------------------------------------------------------------------------------------
// ------------------                    KALO Library - TextToSpeech (TTS) with SpeechGen.IO                   ------------------ 
// ------------------           voice_SpeechGen() establishes a TTS connection to SpeechGen.IO server,         ------------------
// ------------------          creates an mp3 file on SpeechGen server and returns url of spoken voice         ------------------
// ------------------                                    KALO - Jan. 6, 2025                                   ------------------
// ------------------                                                                                          ------------------
// ------------------      Example call: 'url = voice_SpeechGen( TTS_text, "Gisela", "1", "1", "good" );'      ------------------
// ------------------        Hundreds of voices, see here: https://speechgen.io/index.php?r=api/voices         ------------------
// ------------------                                                                                          ------------------
// ------------------                > requires Arduino-ESP32 3.1.0 (ESP-IDF 5.3.0) or later <                 ------------------
// ------------------             (bugfix: Enabled support for non-AES ciphers in GCM operation)               ------------------
// ------------------           details: https://github.com/espressif/arduino-esp32/issues/10071               ------------------
// ------------------           details: https://github.com/espressif/esp32-arduino-lib-builder/pull/200       ------------------
// ------------------           details: https://github.com/espressif/esp-idf/issues/14298                     ------------------ 
// ------------------------------------------------------------------------------------------------------------------------------


#include <WiFiClientSecure.h>   

#ifndef DEBUG                   // user to define his favorite behaviour ('false' suppresses Serial.prints in library)
#  define DEBUG  true           // <- define your preference here  
#  define DebugPrint(x);        if(DEBUG){Serial.print(x);}   /* do not touch */
#  define DebugPrintln(x);      if(DEBUG){Serial.println(x);} /* do not touch */ 
#endif


// --- PRIVATE credentials -----  

#define SPEECHGEN_TOKEN         "..."       // ### INSERT your SPEECHGEN_TOKEN
#define SPEECHGEN_EMAIL         "..."       // ### INSERT your SPEECHGEN_EMAIL 

 

// ------------------------------------------------------------------------------------------------------------------------------
// String voice_SpeechGen( String request_text, String voice, String vpitch, String vspeed, String vemotions )
//
// 'text':    text to speak'voice', see here: https://speechgen.io/index.php?r=api/voices
// 'speed':   playback speed [default 1], range from  0.1 to 2.0
// 'pitch':   voice pitch [default 0], range from -20 to 20    
// 'emotion': emotion of voice [default 'good'], possible values: 'good' , 'evil' , 'neutral' 
//
// ... more details see here: https://speechgen.io/en/node/api/

String voice_SpeechGen( String request_text, String voice, String vpitch, String vspeed, String vemotions )
{    
    uint32_t t_start = millis(); 
    String json_Response = "";
    String url_Response = "";
    
    // initialize and keep an encapsulated WiFiClientSecure object (we also could use a global var instead)
    // We use static instance, so we could keep connection open (removing client_SpeechGenIO.stop below) if needed
    
    static WiFiClientSecure client_SpeechGenIO; 

    // ---------- Connect to SpeechGen Server (only if needed, e.g. on INIT or after .close() or after lost connection)

    if ( !client_SpeechGenIO.connected() )
    { DebugPrintln("> Initialize SpeechGen.IO connection ... ");
      client_SpeechGenIO.setInsecure();
      if (!client_SpeechGenIO.connect("speechgen.io", 443)) 
      { Serial.println("\nERROR - WifiClientSecure connection to SpeechGen.IO Server failed!");
        client_SpeechGenIO.stop(); /* might not have any effect, similar with client.clear() */
        return ("");   
      }
      DebugPrintln("Done. Connected to SpeechGen.IO Server.");
    }
    
    // == creating Request body:
    String Prompt = "{";
    Prompt += "\"token\":\"" + (String) SPEECHGEN_TOKEN + "\", ";
    Prompt += "\"email\":\"" + (String) SPEECHGEN_EMAIL + "\", ";
    Prompt += "\"voice\":\"" + voice  + "\", ";
    Prompt += "\"pitch\":\"" + vpitch + "\", ";
    Prompt += "\"speed\":\"" + vspeed + "\", ";
    Prompt += "\"emotion\":\"" + vemotions + "\", ";
    Prompt += "\"text\":\""  + request_text + "\"}";

    // sending header:
    client_SpeechGenIO.println("POST https://speechgen.io/index.php?r=api/text HTTP/1.1"); 
    /* client_SpeechGenIO.println("Connection: close"); */
    client_SpeechGenIO.println("Host: speechgen.io"); 
    client_SpeechGenIO.println("Content-Type: application/json; charset=utf-8"); 
    client_SpeechGenIO.println("Content-Length: " + String(Prompt.length())); 
    client_SpeechGenIO.println(); 
      
    //sending body:
    client_SpeechGenIO.println(Prompt); 
      
    // Receiving SpeechGen.IO response (with audio url) 
    while ( json_Response == "" )    
    { while (client_SpeechGenIO.available())    
      { char c = client_SpeechGenIO.read();
        json_Response += String(c);      
      }       
    }       

    // ---------- closing connection to SpeechGen.IO                     
    // Observation: We could keep connection open (for lower latency on next call), BUT mixed 'audio.openai_speech()' and 
    // 'audio.connecttohost()' calls in AUDIO.H seem to fail often on parrallel WiFiClientSecure objects -> So we close here
    // (same issue also if client_SpeechGenIO would be a global var, AUDIO.H still struggling) 
    
    client_SpeechGenIO.stop();      // remove this statement for best performance (but audio output via AUDIO.H might fail)        


    // -- Extracting url as return value, Debug prints & Latency [sec]
   
    url_Response = json_Response;
    int pos_start, pos_end;
    String json_Tag_Start = "\"file\":";    
    String json_Tag_End   = "\"file_cors\":";    
  
    pos_start = url_Response.indexOf(json_Tag_Start);      
    if (pos_start > 0) 
    {  pos_start += json_Tag_Start.length()+1;      
       pos_end = url_Response.indexOf(json_Tag_End, pos_start);   
    }  url_Response = url_Response.substring(pos_start, pos_end-2);
  
    // typical result: https:\/\/speechgen.io\/texttomp3\/20240613\/p_23207845_700.mp3
    // we remove the '\' (just a cosmetical issue, would also work with '\')  
    url_Response.replace( "\\", "" );    

    DebugPrintln( ">> SpeechGen PROMPT = [" + Prompt + "]");    
    /* DebugPrintln( ">> SpeechGen Response = [" + json_Response + "]");    // complete response */
    DebugPrintln( ">> SpeechGen url = [" + url_Response + "]");    
    DebugPrintln( ">> SpeechGen Total Latency [sec]: " + (String) ((float)( (millis() - t_start) )/1000) ); 
     
    return url_Response;
}   
 
