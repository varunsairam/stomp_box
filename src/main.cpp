// TODO: header

#include <SPI.h>
#include <SD.h>
#include <vector>
#include <Arduino.h>
#include "AudioTools.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

// Debugging ----
#include <esp_heap_caps.h>
void log_ram_usage(const char* msg) {
  size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
  LOGI("%s: Free heap: %u bytes, Largest block: %u bytes", msg, freeHeap, largestBlock);
}
// --------------

const int chipSelect=PIN_AUDIO_KIT_SD_CARD_CS;
const int buttonPin = 36; // Change to your actual GPIO pin
const int modePin = 13;
AudioBoardStream i2s(AudioKitEs8388V1); // final output of decoded stream
EncodedAudioStream decoder(&i2s, new MP3DecoderHelix()); // Decoding stream
StreamCopy copier; 

File audioFile;
std::vector<String> mp3Files; // List of mp3 filenames
int currentTrack = 0; // Index of current track
uint8_t* mp3Buffer = nullptr;
size_t mp3Size = 0;

void scan_mp3_files() {
  mp3Files.clear();
  File root = SD.open("/");
  if (!root) {
    LOGI("Failed to open SD root");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    String fname = String(file.name());
    if (fname.endsWith(".mp3") || fname.endsWith(".MP3")) {
      mp3Files.push_back(fname);
      LOGI("Found MP3: %s", fname.c_str());
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();
}


void load_mp3_to_ram(const String& filename) {
  log_ram_usage("Before loading track");
  if (mp3Buffer) {
    free(mp3Buffer);
    mp3Buffer = nullptr;
    mp3Size = 0;
    log_ram_usage("After freeing previous buffer");
  }
  audioFile = SD.open(('/'+filename).c_str());
  if (!audioFile) {
    LOGI("Failed to open %s", filename.c_str());
    return;
  }
  mp3Size = audioFile.size();
  mp3Buffer = (uint8_t*)malloc(mp3Size);
  if (mp3Buffer) {
    audioFile.read(mp3Buffer, mp3Size);
    LOGI("Loaded %s to RAM (%d bytes)", filename.c_str(), mp3Size);
    log_ram_usage("After loading track");
    // Configure I2S for this track
    AudioInfo info = decoder.audioInfo();
    i2s.end();
    auto config = i2s.defaultConfig(TX_MODE);
    config.sample_rate = info.sample_rate;
    config.channels = info.channels;
    config.bits_per_sample = info.bits_per_sample;
    i2s.begin(config);
  } else {
    LOGI("Failed to allocate RAM for %s", filename.c_str());
    log_ram_usage("After failed allocation");
  }
  audioFile.close();
}

void play_audio_from_ram() {
  if (!mp3Buffer) return;
  MemoryStream mp3Stream(mp3Buffer, mp3Size);
  StreamCopy copier(decoder, mp3Stream);
  while (copier.copy()) { ; }
}

void setup(){
  Serial.begin(115200);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);  

  pinMode(buttonPin, INPUT_PULLUP); // Use INPUT_PULLUP for active-low button
  pinMode(modePin, INPUT_PULLUP); // Use INPUT_PULLUP for track selection button
  
  // setup audiokit before SD!
  auto config = i2s.defaultConfig(TX_MODE);
  config.sd_active = true;
  i2s.begin(config);
  SD.begin(chipSelect);
  decoder.begin();

  scan_mp3_files();
  if (!mp3Files.empty()) {
    currentTrack = 0;
    load_mp3_to_ram(mp3Files[currentTrack]);
    LOGI("Ready. Current track: %s", mp3Files[currentTrack].c_str());
  } else {
    LOGI("No MP3 files found on SD card.");
  }
}

void loop(){
  if (digitalRead(buttonPin) == LOW) { // Button pressed
    LOGI("Button pressed, playing track: %s", mp3Files[currentTrack].c_str());
    play_audio_from_ram();
    while (digitalRead(buttonPin) == LOW) { delay(10); }
  }

  if (digitalRead(modePin) == LOW) { // Track select button pressed
    if (!mp3Files.empty()) {
      currentTrack = (currentTrack + 1) % mp3Files.size();
      LOGI("Track changed to: %s", mp3Files[currentTrack].c_str());
      load_mp3_to_ram(mp3Files[currentTrack]);
    }
    delay(500); // Debounce delay
  }
}