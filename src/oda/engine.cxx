
#include <oda/engine.h>
#include <oda/audioserver.h>
#include <oda/dspserver.h>
#include <oda/dspunit.h>
#include <oda/portable.h>
#include <oda/soundtrackevent.h>

#include ODA_OPENAL_DIR(al.h)
#include ODA_OPENAL_DIR(alc.h)

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

namespace oda {

// unnamed namespace
namespace {

using std::make_shared;
using std::ofstream;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::transform;
using std::unique_ptr;
using std::vector;
using std::weak_ptr;

//#define ODA_LOG

ALCdevice                         *device = nullptr;
ALCcontext                        *context = nullptr;
unique_ptr<AudioServer>           audioserver;
vector<weak_ptr<SoundtrackEvent>> events__;
double                            lag__ = 0.0;
bool                              playing_started = false;

#ifdef ODA_LOG
ofstream            out;
void printSample(ostream &out, float sample) {
  int n = static_cast<int>(sample*40.f)+40;
  for (int i = 0; i < n; ++i)
    out << "#";
  out << std::endl;
}
#endif

} // unnamed namespace

const size_t Engine::TICK_BUFFER_SIZE = 64;

Engine::Engine() {}

Status Engine::start(const vector<string>& patch_paths) {
  // Do not start if the context was already created
  if (started())
    return Status::INVALID("Already started");
  // Open device
  device = alcOpenDevice(nullptr);
  if (!device)
    return Status::FAILURE("Could not open a device");
  // Create and set context
  context = alcCreateContext(device, nullptr);
  if (!context || alcMakeContextCurrent(context) == ALC_FALSE) {
    if (context) {
      alcDestroyContext(context);
      context = nullptr;
    }
    alcCloseDevice(device);
    device = nullptr;
    return Status::FAILURE("Could not set a context");
  }
  // Start DSP server
  Status dsp_start = DSPServer().start(patch_paths);
  if (!dsp_start.ok()) {
    alcDestroyContext(context);
    alcCloseDevice(device);
    context = nullptr;
    device = nullptr;
    return Status::FAILURE("Engine internal: " + dsp_start.description());
  }
  // Create audio audioserver
  audioserver.reset(new AudioServer);
  playing_started = false;
  lag__ = 0.0;
#ifdef ODA_LOG
  out.open("out");
#endif
  // Tell which device was opened
  return Status::OK(alcGetString(device, ALC_DEVICE_SPECIFIER));
}

bool Engine::started() const {
  return context && device;
}

void Engine::registerPath(const string &path) {
  DSPServer().addPath(path);
}

void Engine::finish() {
  // Do not finish if it was not started yet
  if (!context) return;
  // Finish DSP server
  DSPServer().finish();
  // Destroy audio audioserver
  audioserver->stopSource(0);
  audioserver.reset();
  // Unset and destroy context
  alcMakeContextCurrent(nullptr);
  alcDestroyContext(context);
  context = nullptr;
  // Close device
  alcCloseDevice(device);
  device = nullptr;
}

void Engine::tick() {
  DSPServer dsp;
  // How many dsp ticks are needed for N seconds
  audioserver->update();
  dsp.cleanUp();
  dsp.handleCommands();
  while (audioserver->availableBuffers()) {
    int ticks = TICK_BUFFER_SIZE/dsp.tick_size();
    // Transfer signal from dsp server to audio server
    vector<float> signal;
    dsp.process(ticks, &signal);
    vector<int16_t> audio(dsp.tick_size()*ticks);
    for (size_t i = 0; i < signal.size(); ++i)
      audio[i] = static_cast<int16_t>(signal[i]*32767.f/2.f);
    audioserver->streamData(&audio);
    if (!playing_started) {
      //audioserver->playSource(0);
      playing_started = true;
    }
#ifdef ODA_LOG
    out << "Buffer update: " << ticks*dsp.tick_size() << std::endl;
    for (size_t i = 0; i < audio.size(); ++i)
      printSample(out, audio[i]/32767.f);
#endif
  }
}

void Engine::tick(double dt) {
  DSPServer dsp;
  const double TICK = 1.0*TICK_BUFFER_SIZE/dsp.sample_rate();
  lag__ += dt;
  // How many dsp ticks are needed for N seconds
  audioserver->update();
  dsp.cleanUp();
  dsp.handleCommands();
  while (lag__ >= TICK && audioserver->availableBuffers()) {
    dsp.processTick();
    shared_ptr<SoundtrackEvent> event;
    for (weak_ptr<SoundtrackEvent> &weak_event : events__)
      if ((event = weak_event.lock())) {
        event->processAudio();
      }
    lag__ -= TICK;
  }
}

Status Engine::eventInstance(const string &path_to_dspunit,
                             shared_ptr<SoundtrackEvent> *event_out) {
  shared_ptr<DSPUnit> dspunit = DSPServer().loadUnit(path_to_dspunit);
  if (!dspunit->status().ok())
    return Status::FAILURE("Could not load DSP Unit: "
                           + dspunit->status().description());
  shared_ptr<AudioUnit> audiounit = audioserver->loadUnit();
  if (!audiounit->status().ok())
    return Status::FAILURE("Could not load Audio Unit: "
                           + audiounit->status().description());
  *event_out = make_shared<SoundtrackEvent>(dspunit, audiounit);
  events__.emplace_back(*event_out);
  return Status::OK("Soundtrack event successfully created");
}

} // namespace oda

