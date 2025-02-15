#pragma once

#include "serversideclient.h"
#include <string>
#include <vector>
#include <shared_mutex>
#include <networksystem/inetworkmessages.h>
#include <networksystem/inetworkserializer.h>
#include "../protobuf/generated/netmessages.pb.h"
#include <queue>
#include <string>

struct Sound {
  std::string audioBuffer;
  std::string audioPath;
  float volume;
  float duration;
};

extern std::queue<Sound> g_SoundQueue;
extern float g_CurrentSoundProgress;
extern bool g_IsPaused;

const int MAX_LISTENERS = 1024;

typedef void (*PLAY_START_CALLBACK)(int);
typedef void (*PLAY_END_CALLBACK)(int);
typedef void (*PLAY_CALLBACK)(int, int);

// plugin
extern int FRAMESIZE;
extern int SAMPLERATE;
extern int g_SectionNumber;

struct SVCVoiceDataMessage
{
  std::string voice_data;
  CNetMessagePB<CSVCMsg_VoiceData> *msg = nullptr;

  void Destroy();
};

inline void SVCVoiceDataMessage::Destroy()
{
  if (msg)
  {
    delete msg;
  }
}

extern std::shared_mutex g_Mutex;
extern int MAX_SLOT;

extern std::vector<SVCVoiceDataMessage> g_PlayerAudioBuffer[];
extern std::vector<SVCVoiceDataMessage> g_GlobalAudioBuffer;
extern bool g_PlayerHearing[];
extern float g_PlayerVolume[]; // not implemented yet
extern int g_PlayerProgress[];
extern int g_GlobalProgress;
extern PLAY_START_CALLBACK g_PlayStartListeners[MAX_LISTENERS];
extern PLAY_END_CALLBACK g_PlayEndListeners[MAX_LISTENERS];
extern PLAY_CALLBACK g_PlayListeners[MAX_LISTENERS];
extern CServerSideClient *g_AudioPlayerClient;

extern int g_Player;

void InitializeGlobals();
