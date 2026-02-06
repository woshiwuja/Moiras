#pragma once

#include "../game/game_object.h"
#include "imgui.h"
#include <raylib.h>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>

namespace moiras
{

  class AudioManager : public GameObject
  {
  private:
    // Store Music in unique_ptrs so pointers remain stable across map rehashes
    std::unordered_map<std::string, std::unique_ptr<Music>> musicTracks;
    std::unordered_map<std::string, Sound> sounds;
    std::string currentTrackName;
    bool paused = false;
    float volume = 0.8f;

    // Sorted track names cache for consistent GUI ordering
    std::vector<std::string> trackNames;

    void rebuildTrackNames()
    {
      trackNames.clear();
      trackNames.reserve(musicTracks.size());
      for (const auto &[name, _] : musicTracks)
      {
        trackNames.push_back(name);
      }
      std::sort(trackNames.begin(), trackNames.end());
    }

    Music *getCurrentTrack()
    {
      if (currentTrackName.empty())
        return nullptr;
      auto it = musicTracks.find(currentTrackName);
      if (it != musicTracks.end())
        return it->second.get();
      currentTrackName.clear();
      return nullptr;
    }

  public:
    AudioManager()
    {
      // Larger buffer reduces risk of underruns that cause crackling
      SetAudioStreamBufferSizeDefault(4096);
      InitAudioDevice();
    }

    ~AudioManager()
    {
      for (auto &[name, sound] : sounds)
      {
        UnloadSound(sound);
      }
      for (auto &[name, music] : musicTracks)
      {
        UnloadMusicStream(*music);
      }
      CloseAudioDevice();
    }

    void loadSound(const std::string &name, const std::string &path)
    {
      auto sound = LoadSound(path.c_str());
      sounds[name] = sound;
    }

    void setVolume(float newVolume)
    {
      volume = newVolume;
      // Apply to all loaded tracks immediately
      for (auto &[name, music] : musicTracks)
      {
        SetMusicVolume(*music, volume);
      }
    }

    void loadMusic(const std::string &name, const std::string &filename)
    {
      auto music = std::make_unique<Music>(LoadMusicStream(filename.c_str()));
      music->looping = true;
      SetMusicVolume(*music, volume);
      musicTracks[name] = std::move(music);
      rebuildTrackNames();
    }

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

            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".mp3" || extension == ".wav" || extension == ".ogg" || extension == ".flac")
            {
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
    }

    void playMusic(const std::string &name)
    {
      auto it = musicTracks.find(name);
      if (it != musicTracks.end())
      {
        // Stop current track if switching
        Music *current = getCurrentTrack();
        if (current && currentTrackName != name)
        {
          StopMusicStream(*current);
        }

        PlayMusicStream(*it->second);
        SetMusicVolume(*it->second, volume);
        currentTrackName = name;
        paused = false;
      }
    }

    void stopMusic()
    {
      Music *current = getCurrentTrack();
      if (current)
      {
        StopMusicStream(*current);
      }
      currentTrackName.clear();
      paused = false;
    }

    void togglePause()
    {
      Music *current = getCurrentTrack();
      if (!current)
        return;

      if (paused)
      {
        ResumeMusicStream(*current);
        paused = false;
      }
      else
      {
        PauseMusicStream(*current);
        paused = true;
      }
    }

    void gui() override
    {
      ImGui::Begin("Audio Player");

      if (musicTracks.empty())
      {
        ImGui::Text("No music loaded.");
        ImGui::End();
        return;
      }

      Music *current = getCurrentTrack();

      // -- Now playing section --
      if (current)
      {
        float timePlayed = GetMusicTimePlayed(*current);
        float timeLength = GetMusicTimeLength(*current);

        ImGui::Text("Now playing: %s", currentTrackName.c_str());

        // Seekable progress slider with time overlay
        float seekPos = timePlayed;
        int playedMin = (int)timePlayed / 60;
        int playedSec = (int)timePlayed % 60;
        int totalMin = (int)timeLength / 60;
        int totalSec = (int)timeLength % 60;
        char timeOverlay[64];
        snprintf(timeOverlay, sizeof(timeOverlay), "%d:%02d / %d:%02d", playedMin, playedSec, totalMin, totalSec);
        ImGui::PushItemWidth(-1);
        if (ImGui::SliderFloat("##seek", &seekPos, 0.0f, timeLength, timeOverlay))
        {
          SeekMusicStream(*current, seekPos);
        }
        ImGui::PopItemWidth();

        // Transport controls
        if (paused)
        {
          if (ImGui::Button("Resume"))
          {
            togglePause();
          }
        }
        else
        {
          if (ImGui::Button("Pause"))
          {
            togglePause();
          }
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
          stopMusic();
        }
      }
      else
      {
        ImGui::Text("No track playing.");
      }

      ImGui::Separator();

      // -- Volume control --
      float volumePercent = volume * 100.0f;
      if (ImGui::SliderFloat("Volume", &volumePercent, 0.0f, 100.0f, "%.0f%%"))
      {
        volume = volumePercent / 100.0f;
        for (auto &[name, music] : musicTracks)
        {
          SetMusicVolume(*music, volume);
        }
      }

      ImGui::Separator();

      // -- Track list --
      ImGui::Text("Tracks:");
      for (const auto &name : trackNames)
      {
        bool isCurrentTrack = (name == currentTrackName);

        if (isCurrentTrack)
        {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        }

        ImGui::PushID(name.c_str());
        if (ImGui::Button(name.c_str(), ImVec2(-1, 0)))
        {
          playMusic(name);
        }
        ImGui::PopID();

        if (isCurrentTrack)
        {
          ImGui::PopStyleColor(2);
        }
      }

      ImGui::End();
    }

    void update() override
    {
      Music *current = getCurrentTrack();
      if (current)
      {
        UpdateMusicStream(*current);
      }
    }
  };

} // namespace moiras
