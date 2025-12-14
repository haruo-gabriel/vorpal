
#include <vorpal/engine.h>
#include <vorpal/audioserver.h>
#include <vorpal/dspserver.h>
#include <vorpal/dspunit.h>
#include <vorpal/portable.h>
#include <vorpal/soundtrackevent.h>

/* #include ODA_OPENAL_DIR(al.h)
#include ODA_OPENAL_DIR(alc.h) */

#include <AL/al.h>
#include <AL/alc.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

namespace vorpal {

// unnamed namespace
namespace {

using std::make_shared;
using std::map;
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
map<int, vector<weak_ptr<SoundtrackEvent>>> events_by_inst__;
double                            lag__ = 0.0;
long long unsigned                tick_counter__ = 0;
bool                              playing_started = false;

ofstream            out;
void printSample(ostream &out, float sample) {
  int n = static_cast<int>(sample*40.f)+40;
  for (int i = 0; i < n; ++i)
    out << "#";
  out << std::endl;
}

size_t totalEventCount() {
  size_t count = 0;
  for (const auto& pair : events_by_inst__) {
    count += pair.second.size();
  }
  return count;
}

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
  
  // Log which device was opened
  const ALCchar* deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
  std::cout << "[VORPAL Engine] OpenAL device: " << (deviceName ? deviceName : "unknown") << std::endl;
  
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
  Status dsp_start = DSPServer().start(instances_, patch_paths);
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
  tick_counter__ = 0u;
#ifdef ODA_LOG
  out.open("out");
#else
  out.open("/dev/null");
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
  DSPServer().finish(instances_);
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

void Engine::tick(double dt) {
  static int engine_tick_count = 0;
  if (engine_tick_count++ < 5) {
    std::cout << "[VORPAL Engine] tick() called dt=" << dt << " lag=" << lag__ << std::endl;
    std::cout << "[VORPAL Engine] started()=" << started() 
              << " events=" << totalEventCount() << std::endl;
  }
  
  DSPServer dsp;
  const double TICK = 1.0*TICK_BUFFER_SIZE/dsp.sample_rate();
  lag__ += dt;
  // How many dsp ticks are needed for N seconds
  audioserver->update();
  dsp.cleanUp(instances_);
  dsp.handleCommands(instances_);
  out << "[VORPAL] update by " << dt << " seconds" << std::endl;
  
  if (engine_tick_count <= 5) {
    std::cout << "[VORPAL Engine] Before tick loop: lag=" << lag__ << " TICK=" << TICK 
              << " availableBuffers=" << audioserver->availableBuffers() 
              << " totalEventCount=" << totalEventCount() << std::endl;
  }
  
  while (lag__ >= TICK && audioserver->availableBuffers() >= totalEventCount()) {
    out << "[VORPAL] tick " << tick_counter__ << "("
        << audioserver->availableBuffers() << " available buffers)"
        << std::endl;
    dsp.processTick(instances_);
    
    // Process events grouped by instance
    for (auto& pair : events_by_inst__) {
      int instance_id = pair.first;
      auto& events = pair.second;
      size_t idx = 0;
      for (weak_ptr<SoundtrackEvent> weak : events) {
        shared_ptr<SoundtrackEvent> event;
        if ((event = weak.lock())) {
          out << "[VORPAL] processing event " << idx << " (instance " << instance_id << ")" << std::endl;
          event->processAudio();
        } else {
          out << "[VORPAL] dead event " << idx << " (instance " << instance_id << ")" << std::endl;
        }
        ++idx;
      }
    }
    lag__ -= TICK;
    ++tick_counter__;
    
    // Start OpenAL playback after first tick
    if (!playing_started && tick_counter__ > 0) {
      audioserver->playSource(0);
      playing_started = true;
    }
  }
}

Status Engine::eventInstance(const string &path_to_dspunit,
                             shared_ptr<SoundtrackEvent> *event_out,
                             int instance_id) {
  shared_ptr<DSPUnit> dspunit = DSPServer().loadUnit(path_to_dspunit, instances_, instance_id);
  if (!dspunit->status().ok())
    return Status::FAILURE("Could not load DSP Unit: "
                           + dspunit->status().description());
  shared_ptr<AudioUnit> audiounit = audioserver->loadUnit();
  if (!audiounit->status().ok())
    return Status::FAILURE("Could not load Audio Unit: "
                           + audiounit->status().description());
  *event_out = make_shared<SoundtrackEvent>(dspunit, audiounit);
  events_by_inst__[instance_id].emplace_back(*event_out);
  return Status::OK("Soundtrack event successfully created");
}

int Engine::createInstance(const std::vector<std::string>& paths) {
  return instances_.createInstance(paths);
}

void Engine::destroyInstance(int instance_id) {
  // Remove all events associated with this instance to prevent use-after-free
  events_by_inst__.erase(instance_id);
  
  // Now safe to destroy the PDInstance
  instances_.destroyInstance(instance_id);
}

} // namespace vorpal
