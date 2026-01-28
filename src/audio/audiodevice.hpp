#pragma once

#include "../game/game_object.h"
#include "imgui.h"
#include <raylib.h>
#include <unordered_map>
namespace moiras {

class AudioManager : public GameObject {
private:
  std::unordered_map<std::string, Sound> sounds;
  std::unordered_map<std::string, Music> musicTracks;
  float timePlayed = 0.0f;
  bool paused = false;
  float volume = 0.8f;

public:
  Music *currentMusicTrack;
  AudioManager() { InitAudioDevice(); }
  ~AudioManager() {
    for (auto &[name, sound] : sounds) { // C++17 structured bindings
      UnloadSound(sound);
    }
    for (auto &[name, music] : musicTracks) { // C++17 structured bindings
      UnloadMusicStream(music);
    }
    CloseAudioDevice();
  }
  void loadSound(const std::string &name, const std::string &path) {
    auto sound = LoadSound(path.c_str());
    sounds[name] = sound;
  };

  void loadMusic(const std::string &name, const std::string &path) {
    auto music = LoadMusicStream(path.c_str());
    SetMusicVolume(music, volume);
    musicTracks[name] = music;
  };
  void playSound(const std::string &name) {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
      PlaySound(it->second);
    }
  };
  void playMusic(const std::string &name) {
    auto it = musicTracks.find(name);
    if (it != musicTracks.end()) {
      PlayMusicStream(it->second);
      currentMusicTrack = &it->second;
    }
  };
  void stopMusic(const std::string &name) {
    auto it = musicTracks.find(name);
    if (it != musicTracks.end()) {
      StopMusicStream(it->second);
    }
  };
  void gui() override {
    ImGui::Begin("Audio Player");
    if (musicTracks.empty()) {
      ImGui::Text("Nessun suono caricato.");
    } else {
      for (auto &[name, music] : musicTracks) {
        if (ImGui::Button(("Play " + name).c_str())) {
          PlayMusicStream(music);
        }
        ImGui::SameLine();
        ImGui::Text("(%s)", name.c_str());
      }
    }
    ImGui::End();
  }
  void update() override { UpdateMusicStream(*currentMusicTrack); }
};

} // namespace moiras