#pragma once

#include "../game/game_object.h"
#include "imgui.h"
#include <raylib.h>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
namespace moiras
{

  class AudioManager : public GameObject
  {
  private:
    std::unordered_map<std::string, Sound> sounds;
    std::unordered_map<std::string, Music> musicTracks;
    float timePlayed = 0.0f;
    bool paused = false;
    float volume = 0.8f;

  public:
    Music *currentMusicTrack;
    AudioManager() { InitAudioDevice(); }
    ~AudioManager()
    {
      for (auto &[name, sound] : sounds)
      { // C++17 structured bindings
        UnloadSound(sound);
      }
      for (auto &[name, music] : musicTracks)
      { // C++17 structured bindings
        UnloadMusicStream(music);
      }
      CloseAudioDevice();
    }
    void loadSound(const std::string &name, const std::string &path)
    {
      auto sound = LoadSound(path.c_str());
      sounds[name] = sound;
    };

    void setVolume (float otherVolume){
      volume = otherVolume;
    }
    void loadMusic(const std::string &name, const std::string &filename)
    {
      auto music = LoadMusicStream(filename.c_str());
      SetMusicVolume(music, volume);
      musicTracks[name] = music;
    };
    void loadMusicFolder(const std::string &folderPath)
    {
      namespace fs = std::filesystem;
      try
      {
        if (!fs::exists(folderPath) || !fs::is_directory(folderPath))
        {
          TraceLog(LOG_WARNING, "Audio folder path does not exist: %s", folderPath.c_str());
          return;
        }

        for (const auto &entry : fs::directory_iterator(folderPath))
        {
          if (entry.is_regular_file())
          {
            std::string filePath = entry.path().string();
            std::string extension = entry.path().extension().string();

            // Convert extension to lowercase for safety
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            // Filter for common audio formats
            if (extension == ".mp3" || extension == ".wav" || extension == ".ogg" || extension == ".flac")
            {
              // Using the filename (without extension) as the ID/Key for the map
              std::string musicId = entry.path().stem().string();

              loadMusic(musicId, filePath);
              TraceLog(LOG_INFO, "Auto-loaded music: %s from %s", musicId.c_str(), filePath.c_str());
            }
          }
        }
      }
      catch (const std::exception &e)
      {
        TraceLog(LOG_ERROR, "Failed to iterate music folder: %s", e.what());
      }
    }
    void playSound(const std::string &name)
    {
      auto it = sounds.find(name);
      if (it != sounds.end())
      {
        PlaySound(it->second);
      }
    };
    void playMusic(const std::string &name)
    {
      auto it = musicTracks.find(name);
      if (it != musicTracks.end())
      {
        PlayMusicStream(it->second);
        currentMusicTrack = &it->second;
      }
    };
    void stopMusic(const std::string &name)
    {
      auto it = musicTracks.find(name);
      if (it != musicTracks.end())
      {
        StopMusicStream(it->second);
        currentMusicTrack = nullptr;
      }
    };
    void gui() override
    {
      ImGui::Begin("Audio Player");
      if (musicTracks.empty())
      {
        ImGui::Text("Nessun suono caricato.");
      }
      else
      {
        for (auto &[name, music] : musicTracks)
        {
          if (ImGui::Button(("Play " + name).c_str()))
          {
            PlayMusicStream(music);
            currentMusicTrack = &music;
          }
          if (ImGui::Button(("Pause" + name).c_str()))
          {
            PauseMusicStream(music);
          }
          if (ImGui::Button(("Stop" + name).c_str()))
          {
            if(currentMusicTrack != nullptr){
            StopMusicStream(*currentMusicTrack);
            currentMusicTrack = nullptr;
            }
          }
          ImGui::SameLine();
          ImGui::Text("(%s)", name.c_str());
        }
      }
      ImGui::End();
    };
    void update() override
    {
      if (currentMusicTrack != nullptr)
      {
        UpdateMusicStream(*currentMusicTrack);
      };
    }
  }; // namespace moiras
}