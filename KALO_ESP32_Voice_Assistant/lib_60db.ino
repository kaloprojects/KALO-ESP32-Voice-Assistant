
// ------------------------------------------------------------------------------------------------------------------------------
// ----------------            KALO Library - 60db.ai SpeechToText + TextToSpeech with ESP32 & SD Card           ----------------
// ----------------                                                                                              ----------------
// ----------------   Mirrors the KALO Deepgram lib style (raw WiFiClientSecure, manual HTTP, json_object()).    ----------------
// ----------------   Designed to run 'along with' Deepgram, selectable via #define toggle in the main sketch.   ----------------
// ----------------                                                                                              ----------------
// ----------------   STT  CALL: 'text  = SpeechToText_60db(SD_audio_file)'         [no Initialization needed]    ----------------
// ----------------   TTS  CALL: 'wfile = TextToSpeech_60db(text, "/tts_60db.wav")' [returns SD path or ""]       ----------------
// ----------------   Voices    : 'Voices_60db()'  (optional: prints your voice_id list to Serial)               ----------------
// ----------------                                                                                              ----------------
// ----------------   API docs: https://docs.60db.ai   |   Host: api.60db.ai:443   |   Auth: 'Bearer <key>'      ----------------
// ------------------------------------------------------------------------------------------------------------------------------


// --- includes ----------------

#include <WiFiClientSecure.h>
/* #include <SD.h>              // also needed, but already included in main.ino */


// --- defines & macros --------

#ifndef DEBUG                   // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG true            // <- define your preference here
#  define DebugPrint(x);        if(DEBUG){Serial.print(x);}   /* do not touch */
#  define DebugPrintln(x);      if(DEBUG){Serial.println(x);} /* do not touch */
#endif


// --- PRIVATE credentials -----

const char* db60ApiKey =        "...";    // ### INSERT your 60db.ai API key (https://60db.ai)  (used as 'Bearer <key>')


// --- user preferences --------

#define DB60_VOICE_ID     ""    // ### INSERT a voice_id (UUID) from GET /myvoices, e.g. "7c32dd99-...". Empty = system default
#define DB60_STT_LANGUAGE "en"  // ISO 639-1 code (e.g. "en", "de") forcing a language, or "auto" for 60db auto-detection
#define TIMEOUT_60DB      20    // max waiting time [sec] for a 60db response (TTS synthesis can take a few seconds)


// --- shared TLS client -------
// Re-using the same global 'client' declared in lib_audio_transcription.ino (one TLS context, opened/closed per call).
// Both STT and TTS here always client.stop() when done, so each call connects fresh to the correct host.

extern WiFiClientSecure client;


// helper declarations (defined further below)
String  json_object( String input, String element );   // defined in lib_audio_transcription.ino (re-used here)



// ##############################################################################################################################
// ###  Small HTTP helpers (shared by STT + TTS): connect, read headers, de-chunk the response body                          ###
// ##############################################################################################################################

// Connect (TLS) to api.60db.ai if not already connected. Returns true on success.
bool db60_connect()
{
  if ( client.connected() ) return true;
  DebugPrintln("> Initialize 60db Server connection ... ");
  client.setInsecure();
  if (!client.connect("api.60db.ai", 443))
  { Serial.println("\nERROR - WifiClientSecure connection to 60db Server failed!");
    client.stop();
    return false;
  }
  DebugPrintln("Done. Connected to 60db Server.");
  return true;
}


// Reads the HTTP status line + headers (until the blank \r\n\r\n). Leaves the BODY untouched in the socket.
// Fills 'headers' (raw header block), '*chunked' (Transfer-Encoding: chunked?) and '*content_len' (-1 if unknown).
// Returns the numeric HTTP status code (e.g. 200), or -1 on timeout.
int db60_read_headers( String &headers, bool *chunked, long *content_len, uint32_t deadline )
{
  headers = ""; *chunked = false; *content_len = -1;
  while ( millis() < deadline )
  { while ( client.available() )
    { char c = client.read();
      headers += c;
      if ( headers.endsWith("\r\n\r\n") )           // end of header block reached
      { String low = headers; low.toLowerCase();
        *chunked = (low.indexOf("transfer-encoding: chunked") != -1);
        int p = low.indexOf("content-length:");
        if (p != -1) { *content_len = headers.substring(p+15, headers.indexOf('\n', p)).toInt(); }
        int status = -1;
        int sp = headers.indexOf(' ');              // "HTTP/1.1 200 OK" -> grab the number after first space
        if (sp != -1) { status = headers.substring(sp+1, sp+5).toInt(); }
        return status;
      }
    }
    delay(2);
  }
  return -1;   // timeout
}


// Stateful body reader that transparently de-chunks 'Transfer-Encoding: chunked' responses.
// Use: init once, then call next() repeatedly; it returns the next clean BODY byte, or -1 when the body is complete.
struct db60_BodyReader
{
  bool     chunked;
  long     content_len;     // -1 if unknown (read until socket closed)
  long     body_seen;       // bytes returned so far (non-chunked accounting)
  long     chunk_left;      // bytes remaining in current chunk (chunked accounting)
  bool     done;
  uint32_t deadline;

  void init( bool _chunked, long _content_len, uint32_t _deadline )
  { chunked = _chunked; content_len = _content_len; deadline = _deadline;
    body_seen = 0; chunk_left = 0; done = false;
  }

  int wait_read()   // block (until deadline) for one raw socket byte; -1 on timeout/close
  { while ( millis() < deadline )
    { if ( client.available() ) return client.read();
      if ( !client.connected() && !client.available() ) return -1;
      delay(1);
    }
    return -1;
  }

  String read_line()   // read raw bytes until '\n' (used for chunk-size lines); trailing \r stripped
  { String line = ""; int c;
    while ( (c = wait_read()) != -1 ) { if (c == '\n') break; if (c != '\r') line += (char)c; }
    return line;
  }

  int next()
  { if (done) return -1;

    if (!chunked)                                   // ---- plain body: honor Content-Length, else read until close
    { if (content_len >= 0 && body_seen >= content_len) { done = true; return -1; }
      int c = wait_read();
      if (c == -1) { done = true; return -1; }
      body_seen++;
      return c;
    }

    // ---- chunked body: <hexsize>\r\n <data> \r\n ... 0\r\n\r\n
    if (chunk_left == 0)
    { String sz = read_line();
      sz.trim();
      if (sz.length() == 0) { sz = read_line(); sz.trim(); }   // tolerate a stray CRLF between chunks
      chunk_left = strtol( sz.c_str(), NULL, 16 );
      if (chunk_left == 0) { done = true; return -1; }          // last chunk
    }
    int c = wait_read();
    if (c == -1) { done = true; return -1; }
    chunk_left--;
    if (chunk_left == 0) { read_line(); }                       // consume trailing CRLF after the chunk data
    return c;
  }
};


// Base64 symbol -> 6-bit value, or -1 for non-alphabet characters
int db60_b64val( char c )
{ if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}


// Minimal JSON string escaper for the TTS 'text' field (quotes, backslash, common control chars)
String db60_json_escape( String s )
{ String out = "";
  for (uint16_t i = 0; i < s.length(); i++)
  { char c = s.charAt(i);
    switch (c)
    { case '\"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:   out += c;
    }
  }
  return out;
}



// ##############################################################################################################################
// ###  SpeechToText  ->  POST /stt   (multipart/form-data: file=<wav>, language=<..>)   returns JSON { "text": ... }        ###
// ##############################################################################################################################

String SpeechToText_60db( String audio_filename )
{
  uint32_t t_start = millis();

  // ---------- Connect to 60db Server (only if needed)
  if ( !db60_connect() ) return ("");
  uint32_t t_connected = millis();

  // ---------- Check AUDIO file exists, get size
  File audioFile = SD.open( audio_filename );
  if (!audioFile) { Serial.println("ERROR - Failed to open file for reading"); return (""); }
  size_t audio_size = audioFile.size();
  audioFile.close();
  DebugPrintln("> Audio File [" + audio_filename + "] found, size: " + (String) audio_size );

  // ---------- flush any pending inbound bytes before a fresh request
  while (client.available()) { client.read(); }

  // ---------- Build the multipart/form-data envelope (head before the wav bytes, tail after)
  String boundary = "----60dbESP32Boundary7f3Hd9KaLo";
  String head = "--" + boundary + "\r\n"
                "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n"
                "Content-Type: audio/wav\r\n\r\n";
  String tail = "\r\n";
  if ( String(DB60_STT_LANGUAGE) != "" )
  { tail += "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"language\"\r\n\r\n"
            + String(DB60_STT_LANGUAGE) + "\r\n";
  }
  tail += "--" + boundary + "--\r\n";
  size_t content_length = head.length() + audio_size + tail.length();

  // ---------- Send HTTPS request header
  client.println("POST /stt HTTP/1.1");
  client.println("Host: api.60db.ai");
  client.println("Authorization: Bearer " + String(db60ApiKey));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(content_length));
  client.println("Connection: close");
  client.println();                 // header complete

  // ---------- Send body: multipart head, then wav bytes (in chunks), then multipart tail
  client.print( head );
  File file = SD.open( audio_filename, FILE_READ );
  const size_t bufferSize = 1024;
  uint8_t buffer[bufferSize];
  size_t  bytesRead;
  while (file.available())
  { bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead > 0) { client.write(buffer, bytesRead); }
  }
  file.close();
  client.print( tail );
  DebugPrintln("> All bytes sent, waiting 60db transcription");
  uint32_t t_bodysent = millis();

  // ---------- Read response (headers + de-chunked body into a small String)
  uint32_t deadline = millis() + TIMEOUT_60DB * 1000;
  String headers; bool chunked; long clen;
  int status = db60_read_headers( headers, &chunked, &clen, deadline );

  String body = "";
  db60_BodyReader rd; rd.init( chunked, clen, deadline );
  for (int c; (c = rd.next()) != -1 && body.length() < 16000; ) { body += (char)c; }

  client.stop();    // close (same reason as Deepgram lib: lets Audio.h re-use TLS afterwards)
  uint32_t t_response = millis();

  if (status != 200)
  { Serial.println("\n*** 60db STT ERROR - HTTP status " + (String)status + " ***");
    DebugPrintln("Response body: " + body);
    return ("");
  }

  // ---------- Parse JSON: 60db transcript field is "text"
  String transcription = json_object( body, "\"text\":" );

  DebugPrintln( "---------------------------------------------------" );
  DebugPrintln( "-> Latency CONNECT:        " + (String)((float)(t_connected-t_start)/1000) );
  DebugPrintln( "-> Latency SENDING body:   " + (String)((float)(t_bodysent-t_connected)/1000) );
  DebugPrintln( "-> Latency 60db response:  " + (String)((float)(t_response-t_bodysent)/1000) );
  DebugPrintln( "=> TOTAL Duration [sec]: . " + (String)((float)(t_response-t_start)/1000) );
  DebugPrintln( "=> HTTP status: " + (String)status + (chunked ? "  (chunked)" : "") );
  DebugPrintln( "=> Transcription: [" + transcription + "]" );
  DebugPrintln( "---------------------------------------------------\n" );

  return transcription;
}



// ##############################################################################################################################
// ###  TextToSpeech ->  POST /tts-synthesize  (JSON)  ->  base64 WAV streamed/decoded straight to an SD file                ###
// ##############################################################################################################################
// Returns the SD path of the written .wav on success (play it via audio_play.connecttoFS(SD, path)), or "" on error.

String TextToSpeech_60db( String text, String out_filename )
{
  uint32_t t_start = millis();
  if ( text == "" ) return ("");

  // ---------- Connect to 60db Server (only if needed)
  if ( !db60_connect() ) return ("");

  // ---------- flush any pending inbound bytes
  while (client.available()) { client.read(); }

  // ---------- Build JSON payload (request WAV so it is directly playable by Audio.h, no transcoding needed)
  String payload = "{";
  payload += "\"text\":\"" + db60_json_escape(text) + "\",";
  if ( String(DB60_VOICE_ID) != "" ) { payload += "\"voice_id\":\"" + String(DB60_VOICE_ID) + "\","; }
  payload += "\"output_format\":\"wav\",";
  payload += "\"enhance\":true,";
  payload += "\"speed\":1,\"stability\":50,\"similarity\":75";
  payload += "}";

  // ---------- Send HTTPS request
  client.println("POST /tts-synthesize HTTP/1.1");
  client.println("Host: api.60db.ai");
  client.println("Authorization: Bearer " + String(db60ApiKey));
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(payload.length()));
  client.println("Connection: close");
  client.println();
  client.print( payload );
  DebugPrintln("> 60db TTS request sent, waiting & decoding audio ...");

  // ---------- Read headers
  uint32_t deadline = millis() + TIMEOUT_60DB * 1000;
  String headers; bool chunked; long clen;
  int status = db60_read_headers( headers, &chunked, &clen, deadline );

  db60_BodyReader rd; rd.init( chunked, clen, deadline );

  if (status != 200)
  { String body = "";
    for (int c; (c = rd.next()) != -1 && body.length() < 4000; ) { body += (char)c; }
    client.stop();
    Serial.println("\n*** 60db TTS ERROR - HTTP status " + (String)status + " ***");
    DebugPrintln("Response body: " + body);
    return ("");
  }

  // ---------- Prepare destination SD file
  if ( SD.exists(out_filename) ) SD.remove(out_filename);
  File out = SD.open(out_filename, FILE_WRITE);
  if (!out) { client.stop(); Serial.println("ERROR - cannot create " + out_filename); return (""); }

  // ---------- Phase 1: scan body until the marker  "audio_base64":"  (the prefix JSON is small)
  const char* marker = "\"audio_base64\":\"";
  String pre = ""; bool found = false;
  for (int c; (c = rd.next()) != -1; )
  { pre += (char)c;
    if (pre.endsWith(marker)) { found = true; break; }
    if (pre.length() > 4096) { pre = pre.substring(pre.length() - 32); }   // keep memory bounded while searching
  }
  if (!found)
  { out.close(); SD.remove(out_filename); client.stop();
    Serial.println("\n*** 60db TTS ERROR - 'audio_base64' not found in response ***");
    return ("");
  }

  // ---------- Phase 2: stream base64 chars -> decode 4:3 -> write WAV bytes to SD (stop at closing quote)
  uint8_t wbuf[512]; size_t wlen = 0;       // write buffer for SD performance
  int     quad[4];   int qn = 0; int pad = 0;
  long    audio_bytes = 0;
  for (int c; (c = rd.next()) != -1; )
  { if (c == '\"') break;                    // closing quote of the base64 string -> done
    if (c == '\r' || c == '\n') continue;    // ignore whitespace/newlines inside the value
    int v;
    if (c == '=') { v = 0; pad++; }
    else { v = db60_b64val((char)c); if (v < 0) continue; }
    quad[qn++] = v;
    if (qn == 4)
    { uint8_t b0 = (quad[0] << 2) | (quad[1] >> 4);
      uint8_t b1 = ((quad[1] & 0x0F) << 4) | (quad[2] >> 2);
      uint8_t b2 = ((quad[2] & 0x03) << 6) | quad[3];
      wbuf[wlen++] = b0; audio_bytes++;
      if (pad < 2) { wbuf[wlen++] = b1; audio_bytes++; }
      if (pad < 1) { wbuf[wlen++] = b2; audio_bytes++; }
      if (wlen >= sizeof(wbuf) - 3) { out.write(wbuf, wlen); wlen = 0; }
      qn = 0; pad = 0;
    }
  }
  if (wlen > 0) { out.write(wbuf, wlen); }
  out.close();
  client.stop();

  uint32_t t_done = millis();
  DebugPrintln( "> 60db TTS done -> '" + out_filename + "', " + (String)audio_bytes
                + " bytes, " + (String)((float)(t_done-t_start)/1000) + " sec" );

  if (audio_bytes == 0) { SD.remove(out_filename); return (""); }
  return out_filename;
}



// ##############################################################################################################################
// ###  OPTIONAL: GET /myvoices  -> print your available voice_id list to Serial (run once to grab a voice_id UUID)          ###
// ##############################################################################################################################

void Voices_60db()
{
  if ( !db60_connect() ) return;
  while (client.available()) { client.read(); }

  client.println("GET /myvoices HTTP/1.1");
  client.println("Host: api.60db.ai");
  client.println("Authorization: Bearer " + String(db60ApiKey));
  client.println("Connection: close");
  client.println();

  uint32_t deadline = millis() + TIMEOUT_60DB * 1000;
  String headers; bool chunked; long clen;
  int status = db60_read_headers( headers, &chunked, &clen, deadline );

  String body = "";
  db60_BodyReader rd; rd.init( chunked, clen, deadline );
  for (int c; (c = rd.next()) != -1 && body.length() < 16000; ) { body += (char)c; }
  client.stop();

  DebugPrintln("---- 60db /myvoices (HTTP " + (String)status + ") ----");
  DebugPrintln(body);
  DebugPrintln("-------------------------------------------");
}
