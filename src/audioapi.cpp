#include "audioapi.h"
#include "globals.h"
#include "helper.h"
#include <iostream>
#include <thread>
#include <shared_mutex>
#include <string>

// void SetPlayerVolume(int slot, float factor)
// {
//   std::unique_lock<std::shared_mutex> lock(g_Mutex);
//   g_PlayerVolume[slot] = factor;
// }

// void SetAllPlayerVolume(float factor)
// {
//   std::unique_lock<std::shared_mutex> lock(g_Mutex);
//   for (int i = 0; i < MAX_SLOT; i++)
//   {
//     g_PlayerVolume[i] = factor;
//   }
// }

// float GetPlayerVolume(int slot)
// {
//   std::shared_lock<std::shared_mutex> lock(g_Mutex);
//   return g_PlayerVolume[slot];
// }
namespace api
{
  void SetPlayerHearing(int slot, bool hearing)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    g_PlayerHearing[slot] = hearing;
  }

  void SetAllPlayerHearing(bool hearing)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    for (int i = 0; i < MAX_SLOT; i++)
    {
      g_PlayerHearing[i] = hearing;
    }
  }

  bool IsHearing(int slot)
  {
    std::shared_lock<std::shared_mutex> lock(g_Mutex);
    return g_PlayerHearing[slot];
  }
  void PlayToPlayer(int slot, std::string audioBuffer, std::string audioPath, float volume)
  {
    if (audioBuffer.size() == 0 && audioPath.size() == 0)
    {
      std::unique_lock<std::shared_mutex> lock(g_Mutex);
      g_PlayerAudioBuffer[slot].clear();
      g_PlayerProgress[slot] = 0;
      for (auto &callback : g_PlayEndListeners)
      {
        if (callback != nullptr)
          callback(slot);
      }
      return;
    }
    auto lambda = [slot](std::vector<SVCVoiceDataMessage> msgbuffer)
    {
      std::unique_lock<std::shared_mutex> lock(g_Mutex);
      g_PlayerAudioBuffer[slot] = msgbuffer;
      g_PlayerProgress[slot] = 0;
      for (auto &callback : g_PlayStartListeners)
      {
        if (callback != nullptr)
          callback(slot);
      }
    };
    std::thread process(ProcessVoiceData, audioBuffer, audioPath, lambda, volume);
    process.detach();
  }

  void Play(std::string audioBuffer, std::string audioPath, float volume)
  {
    //std::vector<SVCVoiceDataMessage> g_TempAudio;
    std::string g_TempAudio;
    
    if (audioBuffer.size() == 0 && audioPath.size() == 0)
    {
      std::unique_lock<std::shared_mutex> lock(g_Mutex);

      g_GlobalAudioBuffer.clear();
      g_GlobalProgress = 0;
      for (auto &callback : g_PlayEndListeners)
      {
        if (callback != nullptr)
          callback(-1);
      }
      return;
    }

    /*
    if (!g_GlobalAudioBuffer.empty())
    {
      g_TempAudio.insert(g_TempAudio.begin(), g_GlobalAudioBuffer.begin(), g_GlobalAudioBuffer.end());
    }
    */

    if (!g_GlobalAudioBuffer.empty())
    {
        // Store the existing global audio buffer in g_TempAudio
        for (const auto& message : g_GlobalAudioBuffer)
        {
            //auto data = g_GlobalAudioBuffer.data()->voice_data;
            g_TempAudio += message.voice_data;
        }
    }

    // Combine audioBuffer with g_TempAudio
    audioBuffer += g_TempAudio;

    auto lambda = [g_TempAudio](std::vector<SVCVoiceDataMessage> msgbuffer) mutable
    {
      std::unique_lock<std::shared_mutex> lock(g_Mutex);
      g_GlobalAudioBuffer = msgbuffer;

      /*
      if(!g_TempAudio.empty())
      {
        auto lastIndex = g_GlobalAudioBuffer.size() - 1;
        
        if(lastIndex < g_TempAudio.size())
        {
          g_GlobalAudioBuffer.insert(g_GlobalAudioBuffer.end(), g_TempAudio.begin() + lastIndex, g_TempAudio.end());
        }
      }
      */

      g_GlobalProgress = 0;
      for (auto &callback : g_PlayStartListeners)
      {
        if (callback != nullptr)
          callback(-1);
      }
    };
    std::thread process(ProcessVoiceData, !g_TempAudio.empty() ? g_TempAudio : audioBuffer, audioPath, lambda, volume);
    process.detach();
  }

  bool IsPlaying(int slot)
  {
    std::shared_lock<std::shared_mutex> lock(g_Mutex);
    return g_PlayerAudioBuffer[slot].size() > 0;
  }

  bool IsAllPlaying()
  {
    std::shared_lock<std::shared_mutex> lock(g_Mutex);
    return g_GlobalAudioBuffer.size() > 0;
  }
  int RegisterPlayStartListener(PLAY_START_CALLBACK callback)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    for (int i = 0; i < MAX_LISTENERS; i++)
    {
      if (g_PlayStartListeners[i] == nullptr)
      {
        g_PlayStartListeners[i] = callback;
        return i;
      }
    }
    return -1;
  }

  void UnregisterPlayStartListener(int id)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    g_PlayStartListeners[id] = nullptr;
  }

  int RegisterPlayEndListener(PLAY_END_CALLBACK callback)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    for (int i = 0; i < MAX_LISTENERS; i++)
    {
      if (g_PlayEndListeners[i] == nullptr)
      {
        g_PlayEndListeners[i] = callback;
        return i;
      }
    }
    return -1;
  }

  void UnregisterPlayEndListener(int id)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    g_PlayEndListeners[id] = nullptr;
  }

  int RegisterPlayListener(PLAY_CALLBACK callback)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    for (int i = 0; i < MAX_LISTENERS; i++)
    {
      if (g_PlayListeners[i] == nullptr)
      {
        g_PlayListeners[i] = callback;
        return i;
      }
    }
  }

  void UnregisterPlayListener(int id)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    g_PlayListeners[id] = nullptr;
  }

  void SetPlayer(int slot)
  {
    std::unique_lock<std::shared_mutex> lock(g_Mutex);
    g_Player = slot;
  }
}

// void CAudioInterface::SetPlayerVolume(int slot, float factor)
// {
//   SetPlayerVolume(slot, factor);
// }

// void CAudioInterface::SetAllPlayerVolume(float factor)
// {
//   SetAllPlayerVolume(factor);
// }
// float CAudioInterface::GetPlayerVolume(int slot)
// {
//   return GetPlayerVolume(slot);
// }
void CAudioInterface::SetPlayerHearing(int slot, bool hearing)
{
  api::SetPlayerHearing(slot, hearing);
}
void CAudioInterface::SetAllPlayerHearing(bool hearing)
{
  api::SetAllPlayerHearing(hearing);
}
bool CAudioInterface::IsHearing(int slot)
{
  return api::IsHearing(slot);
}
void CAudioInterface::PlayToPlayerFromBuffer(int slot, std::string audioBuffer, float volume)
{
  api::PlayToPlayer(slot, audioBuffer, "", volume);
}
void CAudioInterface::PlayToPlayerFromFile(int slot, std::string audioFile, float volume)
{
  api::PlayToPlayer(slot, "", audioFile, volume);
}

void CAudioInterface::PlayFromBuffer(std::string audioBuffer, float volume)
{
  api::Play(audioBuffer, "", volume);
}
void CAudioInterface::PlayFromFile(std::string audioFile, float volume)
{
  api::Play("", audioFile, volume);
}
bool CAudioInterface::IsPlaying(int slot)
{
  return api::IsPlaying(slot);
}
bool CAudioInterface::IsAllPlaying()
{
  return api::IsAllPlaying();
}
int CAudioInterface::RegisterPlayStartListener(PLAY_START_CALLBACK callback)
{
  return api::RegisterPlayStartListener(callback);
}
void CAudioInterface::UnregisterPlayStartListener(int id)
{
  api::UnregisterPlayStartListener(id);
}
int CAudioInterface::RegisterPlayEndListener(PLAY_END_CALLBACK callback)
{
  return api::RegisterPlayEndListener(callback);
}
void CAudioInterface::UnregisterPlayEndListener(int id)
{
  api::UnregisterPlayEndListener(id);
}
int CAudioInterface::RegisterPlayListener(PLAY_CALLBACK callback)
{
  return api::RegisterPlayListener(callback);
}
void CAudioInterface::UnregisterPlayListener(int id)
{
  api::UnregisterPlayListener(id);
}
void CAudioInterface::SetPlayer(int slot)
{
  api::SetPlayer(slot);
}

extern "C"
{
  void __cdecl NativeSetPlayerHearing(int slot, bool hearing)
  {
    api::SetPlayerHearing(slot, hearing);
  }

  void __cdecl NativeSetAllPlayerHearing(bool hearing)
  {
    api::SetAllPlayerHearing(hearing);
  }

  bool __cdecl NativeIsHearing(int slot)
  {
    return api::IsHearing(slot);
  }

  void __cdecl NativePlayToPlayer(int slot, const char *audioBuffer, int audioBufferSize, const char *audioPath, int audioPathSize, float volume)
  {
    auto data1 = std::string(audioBuffer, audioBufferSize);
    auto data2 = std::string(audioPath, audioPathSize);

    api::PlayToPlayer(slot, data1, data2, volume);
  }

  void __cdecl NativePlay(const char *audioBuffer, int audioBufferSize, const char *audioPath, int audioPathSize, float volume)
  {
    auto data1 = std::string(audioBuffer, audioBufferSize);
    auto data2 = std::string(audioPath, audioPathSize);

    api::Play(data1, data2, volume);
  }

  bool __cdecl NativeIsPlaying(int slot)
  {
    return api::IsPlaying(slot);
  }

  bool __cdecl NativeIsAllPlaying()
  {
    return api::IsAllPlaying();
  }

  int __cdecl NativeRegisterPlayStartListener(PLAY_START_CALLBACK callback)
  {
    return api::RegisterPlayStartListener(callback);
  }

  void __cdecl NativeUnregisterPlayStartListener(int id)
  {
    api::UnregisterPlayStartListener(id);
  }

  int __cdecl NativeRegisterPlayEndListener(PLAY_END_CALLBACK callback)
  {
    return api::RegisterPlayEndListener(callback);
  }

  void __cdecl NativeUnregisterPlayEndListener(int id)
  {
    api::UnregisterPlayEndListener(id);
  }
  int __cdecl NativeRegisterPlayListener(PLAY_CALLBACK callback)
  {
    return api::RegisterPlayListener(callback);
  }

  void __cdecl NativeUnregisterPlayListener(int id)
  {
    api::UnregisterPlayListener(id);
  }

  void __cdecl NativeSetPlayer(int slot)
  {
    api::SetPlayer(slot);
  }
}