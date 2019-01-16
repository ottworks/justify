#include "justify.h"
#include "IPlug_include_in_plug_src.h"
#include "resource.h"

#include "IControl.h"
#include "IKeyboardControl.h"


const int kNumPrograms = 8;

#define PITCH 440.
#define TABLE_SIZE 512

#ifndef M_PI
#define M_PI 3.14159265
#endif

#define GAIN_FACTOR 0.2;

static double transpositions[] = {
  1. / 1.,
  16. / 15.,
  9. / 8.,
  6. / 5.,
  5. / 4.,
  4. / 3.,
  45. / 32.,
  3. / 2.,
  8. / 5.,
  27. / 16.,
  16. / 9.,
  15. / 8.,
};

enum EParams
{
  kAttack = 0,
  kDecay,
  kSustain,
  kRelease,
  kNumParams
};

justify::justify(IPlugInstanceInfo instanceInfo)
  : IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo),
    mSampleRate(44100.),
    mNumHeldKeys(0),
    mKey(-1),
    mActiveVoices(0),
    mBendRange(48.0)

{
  TRACE;

  memset(mKeyStatus, 0, 128 * sizeof(bool));

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kAttack)->InitDouble("Amp Attack", ATTACK_DEFAULT, TIME_MIN, TIME_MAX, 0.001);
  GetParam(kDecay)->InitDouble("Amp Decay", DECAY_DEFAULT, TIME_MIN, TIME_MAX, 0.001);
  GetParam(kSustain)->InitDouble("Amp Sustain", 1., 0., 1., 0.001);
  GetParam(kRelease)->InitDouble("Amp Release", RELEASE_DEFAULT, TIME_MIN, TIME_MAX, 0.001);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

justify::~justify()
{
}

int justify::FindFreeVoice()
{
  int v;

  for(v = 0; v < MAX_VOICES; v++)
  {
    if(!mVS[v].GetBusy())
      return v;
  }

  int quietestVoice = 0;
  double level = 2.;

  for(v = 0; v < MAX_VOICES; v++)
  {
    double summed = mVS[v].mEnv_ctx.mPrev;

    if (summed < level)
    {
      level = summed;
      quietestVoice = v;
    }

  }

  DBGMSG("stealing voice %i\n", quietestVoice);
  return quietestVoice;
}

void justify::NoteOnOff(IMidiMsg* pMsg)
{
  int v;

  int status = pMsg->StatusMsg();
  int velocity = pMsg->Velocity();
  int note = pMsg->NoteNumber();

}

void justify::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{

}

void justify::Reset()
{
  TRACE;
  IMutexLock lock(this);

  mSampleRate = GetSampleRate();
  mMidiQueue.Resize(GetBlockSize());
}

void justify::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kAttack:
      break;
    case kDecay:
      break;
    case kSustain:
      break;
    case kRelease:
      break;
    default:
      break;
  }
}

void justify::ProcessMidiMsg(IMidiMsg* pMsg)
{
  int status = pMsg->StatusMsg();
  int velocity = pMsg->Velocity();
  
  switch (status)
  {
    case IMidiMsg::kControlChange:
      
      break;
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
      // filter only note messages
      if (status == IMidiMsg::kNoteOn && velocity)
      {
        mKeyStatus[pMsg->NoteNumber()] = true;
        mNumHeldKeys += 1;

        if (mNumHeldKeys == 1)
        {
          mKey = pMsg->NoteNumber();
          IMidiMsg pb;
          pb.MakePitchWheelMsg(0.0, pMsg->Channel());

          SendMidiMsg(&pb);
        }
        else
        {
          int semitones = pMsg->NoteNumber() - mKey;
          double octaves = 1;

          while (semitones >= 12)
          {
            semitones -= 12;
            octaves *= 2;
          }
          while (semitones < 0)
          {
            semitones += 12;
            octaves /= 2;
          }

          double root_freq = 440 * powf(2, (mKey - 69) / 12.f);
          double microtone_freq = root_freq * transpositions[semitones] * octaves;
          double microtone_note = 12 * log2(microtone_freq / 440.0) + 69;

          IMidiMsg pb;
          pb.MakePitchWheelMsg((microtone_note - pMsg->NoteNumber()) / mBendRange, pMsg->Channel());

          SendMidiMsg(&pb);
        }
      }
      else
      {
        mKeyStatus[pMsg->NoteNumber()] = false;
        mNumHeldKeys -= 1;

        if (mNumHeldKeys == 0)
        {
          mKey = -1;
        }
      }
      break;
    default:
      return; // if !note message, nothing gets added to the queue
  }
  
  mMidiQueue.Add(pMsg);

  SendMidiMsg(pMsg);
}

// Should return non-zero if one or more keys are playing.
int justify::GetNumKeys()
{
  IMutexLock lock(this);
  return mNumHeldKeys;
}

// Should return true if the specified key is playing.
bool justify::GetKeyStatus(int key)
{
  IMutexLock lock(this);
  return mKeyStatus[key];
}

//Called by the standalone wrapper if someone clicks about
bool justify::HostRequestingAboutBox()
{
  IMutexLock lock(this);
  return true;
}
