---
applyTo: '**'
---
Provide project context and coding guidelines that AI should follow when generating code, answering questions, or reviewing changes.

# VORPAL GDExtension Multi-Instance Support — WARP Planning Document

This document is the authoritative plan for implementing Pure Data (libpd) multi-instance support in the VORPAL C++ audio engine and its Godot GDExtension wrapper. It targets both human developers and LLMs for code generation.

- Status: GDExtension works; multi-instance not started
- Goal: Add isolated, concurrent Pure Data instances (libpd PDINSTANCE) with per-instance patch sets, routing, and audio streaming, maintaining backward compatibility

---

## Coding Guidelines

### Comment Style
- Use objective, formal language in all code comments
- Avoid exclamation marks, informal punctuation, and colloquial expressions
- Avoid subjective adverbs such as "gracefully", "easily", "simply", etc.
- State facts directly without emotional emphasis
- Example (incorrect): `// Create a new instance using Engine API (Phase 2!)`
- Example (correct): `// Create a new instance using Engine API (Phase 2)`

---

## Table of Contents
- 1. Purpose and Scope
- 2. System Requirements
- 3. Current Architecture (Pre-Multi-Instance)
- 4. Target Architecture (Multi-Instance)
- 5. Minimal Implementation Plan (MVP)
- 6. Full-Featured Architecture Extensions
- 7. C++ API Changes and Code Fragments
- 8. Godot Integration Details (GDExtension + GDScript)
- 9. libpd and Pure Data Integration Details
- 10. OpenAL Integration Details
- 11. Risks and Mitigations
- 12. Acceptance Criteria
- 13. Future Work and References

---

## 1. Purpose and Scope
- Purpose: Plan and specify multi-instance audio processing with libpd in VORPAL, including engine changes, Godot bindings, and integration.
- Scope: C++ design and implementation notes; GDScript usage and API surface; libpd and OpenAL specifics.
- Out of scope: UI/editor tooling, content creation workflows beyond what’s needed for testing.

---

## 2. System Requirements
- Language/Build:
  - C++17 or later; CMake build system
  - Godot 4.3+ GDExtension API
- Dependencies:
  - libpd built with multi-instance support
    - IMPORTANT: Build libpd with `make MULTI=true` (sets -DPDINSTANCE -DPDTHREADS)
    - Alternative: manually set CFLAGS `-DPDINSTANCE -DPDTHREADS`
    - C++ programs using PdBase.hpp need CPPFLAGS `-DPDINSTANCE`
  - OpenAL (OpenAL Soft recommended)
  - Vorbis/Ogg (optional externals)
- Runtime audio configuration:
  - Sample rate: 44100 Hz (engine/DSPServer currently assumes this)
  - Pd blocksize: 64 samples; ticks use integral multiples of 64
- OpenAL budget:
  - NUM_SOURCES: practical limit 32–64 (device-dependent)
  - NUM_BUFFERS per source: 2–3 for steady streaming

Build flags and defines:
- Ensure libpd compilation defines both PDINSTANCE and PDTHREADS for multi-instance support. Use `make MULTI=true` or manually set both flags.

```cmake path=null start=null
# Ensure libpd multi-instance is enabled (for the target that builds libpd)
target_compile_definitions(pdcpp PUBLIC PDINSTANCE PDTHREADS)

# For C++ programs using PdBase.hpp, also define PDINSTANCE
target_compile_definitions(your_cpp_target PRIVATE PDINSTANCE)
```

---

## 3. Current Architecture (Pre-Multi-Instance)
High-level flow:
- Engine: Manages OpenAL device/context; owns AudioServer; drives DSP tick; creates SoundtrackEvent pairs (DSPUnit + AudioUnit).
- DSPServer: Single global `pd::PdBase` instance (singleton-like); per-tick `processFloat`; reads per-patch bus arrays `vorpal-bus-$0`; delivers per-event buffers.
- AudioServer/AudioUnit: Queues PCM to OpenAL buffers and sources; basic 3D positioning.
- SoundtrackEvent: Bridges DSPUnit to AudioUnit; pushes commands via `$0-command` channels; calls `transferSignal` to stream audio.

Current limitations:
- Only one libpd instance (shared state across patches);
- Global/static registries in DSPServer (not instance-safe);
- No per-instance tick isolation.

---

## 4. Target Architecture (Multi-Instance)
Objectives:
- Create N independent libpd instances, each with its own patches, command bus, and output buffers.
- Backward compatible default (instance_id=0) behaves like single-instance.
- Minimal cross-talk: `$0` addressing stays patch-local; routing is instance-scoped.

Key components:
- PDInstance: Encapsulates libpd PdBase and all per-instance DSP state; owns patch set; per-instance command queue.
- InstanceManager: Creates/destroys PDInstance; registry of active instances; global tick iteration.
- Engine: Owns InstanceManager; maintains events grouped by instance; iterates all instances per tick; streams to OpenAL.

Data flow (per frame):
1) Engine.tick(dt) → for each PDInstance: handleCommands() → processTick()
2) For each event bound to that instance: capture per-unit buffer → queue to OpenAL source
3) OpenAL unqueue processed buffers; keep streaming

---

## 5. Minimal Implementation Plan (MVP)
- Step 1: Enable libpd multi-instances
  - Build libpd with `make MULTI=true` to define PDINSTANCE and PDTHREADS
  - Ensure C++ program defines PDINSTANCE when using PdBase.hpp
  - Verify with `pd::PdBase::numInstances() > 1` when creating multiple PdBase objects
- Step 2: Refactor DSPServer into per-instance PDInstance (no globals)
  - Replace global `pd::PdBase dsp` with a PdBase per PDInstance
  - Move Receiver and command queues into PDInstance
  - Keep `$0-command` and `vorpal-bus-$0` conventions
- Step 3: Introduce InstanceManager
  - Map<int, unique_ptr<PDInstance>>; generate unique instance IDs
- Step 4: Engine changes
  - New: createInstance(paths), destroyInstance(id)
  - Modify: eventInstance(path, &out, instance_id=0)
  - Tick: iterate all instances; then per-instance events
- Step 5: VORPALModule Godot API
  - New: create_instance(), destroy_instance(id)
  - Modify: event_instance(name, instance_id=0)
- Step 6: Testing
  - Two instances with same patch; send different commands; verify independence; monitor audio and print hooks.

---

## 6. Full-Featured Architecture Extensions
- Threading model:
  - Single audio-thread MVP; later: per-instance worker threads with lock-free queues, avoiding locks in audio path.
- Instance pooling:
  - Pooled instances for SFX/UI/music categories; priority-based allocation; recycling/hibernation.
- QoS and scaling:
  - Monitor CPU; adjust tick ratio or instance activity; voice limiting and stealing policy.
- Debugging/Instrumentation:
  - Instance inspector; per-instance meters; bus probes; message router trace.
- Persistence/Hot-reload:
  - Per-instance patch reload; state serialization where possible.

---

## 7. C++ API Changes and Code Fragments

### 7.1 PDInstance interface
```cpp path=null start=null
#include <libpd/PdBase.hpp>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace vorpal {

struct PdCommand {
  std::string receiver;   // e.g., "$0-command"
  std::string selector;   // e.g., "start" / custom symbol
  std::vector<float> fargs; // simple MVP: numeric args only; extend to atoms/symbols
};

class PDInstance {
public:
  explicit PDInstance(int id) : id_(id) {}
  ~PDInstance() { finish(); }

  bool start(const std::vector<std::string>& search_paths,
             int sample_rate = 44100,
             bool queued = true) {
    // libpd instance lives inside pd_ (PdBase)
    if (!pd_.init(/*in=*/0, /*out=*/2, sample_rate, queued)) return false;
    for (auto& p : search_paths) addPath(p);
    pd_.computeAudio(true);
    return true;
  }

  void addPath(const std::string& path) {
    pd_.addToSearchPath(path);
    search_paths_.push_back(path);
  }

  // Load a patch; return a handle/token string (e.g., name or "$0")
  std::string loadPatch(const std::string& name) {
    auto patch = pd_.openPatch(name + ".pd", "."); // search paths apply inside libpd
    if (!patch.isValid()) return {};
    auto dz = patch.dollarZeroStr();
    patches_.emplace(dz, std::move(patch));
    return dz;
  }

  void closePatch(const std::string& dz) {
    auto it = patches_.find(dz);
    if (it == patches_.end()) return;
    pd_.closePatch(it->second);
    patches_.erase(it);
  }

  void enqueue(const PdCommand& cmd) { commands_.push(cmd); }

  void handleCommands() {
    while (!commands_.empty()) {
      auto cmd = commands_.front();
      commands_.pop();
      pd_.startMessage();
      for (float f : cmd.fargs) pd_.addFloat(f);
      pd_.finishMessage(cmd.receiver, cmd.selector);
    }
  }

  // Process one pd tick (64 samples * TICK_RATIO)
  void processTick(int tick_ratio = 1) {
    pd_.processFloat(tick_ratio, nullptr, nullptr);
  }

  // Read patch bus array into buffer; returns number of samples read
  int readBus(const std::string& dz, std::vector<float>& out, int tick_size) {
    out.resize(tick_size);
    const auto bus = std::string("vorpal-bus-") + dz;
    if (!pd_.readArray(bus, out, tick_size)) return 0;
    return tick_size;
  }

  void finish() {
    // Close patches
    for (auto& kv : patches_) {
      pd_.closePatch(kv.second);
    }
    patches_.clear();
    pd_.computeAudio(false);
    pd_.clear();
  }

  int id() const { return id_; }
  pd::PdBase& pd() { return pd_; }

private:
  int id_ = 0;
  pd::PdBase pd_;
  std::vector<std::string> search_paths_;
  std::unordered_map<std::string, pd::Patch> patches_; // key = $0 string
  std::queue<PdCommand> commands_;
};

} // namespace vorpal
```

### 7.2 InstanceManager
```cpp path=null start=null
#include <map>
#include <memory>
#include <vector>

namespace vorpal {

class InstanceManager {
public:
  int createInstance(const std::vector<std::string>& paths) {
    const int id = next_id_++;
    auto inst = std::make_unique<PDInstance>(id);
    if (!inst->start(paths)) return -1;
    instances_.emplace(id, std::move(inst));
    return id;
  }

  void destroyInstance(int id) { instances_.erase(id); }

  PDInstance* get(int id) const {
    auto it = instances_.find(id);
    return it == instances_.end() ? nullptr : it->second.get();
  }

  std::vector<int> ids() const {
    std::vector<int> v; v.reserve(instances_.size());
    for (auto& kv : instances_) v.push_back(kv.first);
    return v;
  }

private:
  int next_id_ = 1;
  std::map<int, std::unique_ptr<PDInstance>> instances_;
};

} // namespace vorpal
```

### 7.3 Engine adjustments
```cpp path=null start=null
namespace vorpal {

class Engine {
public:
  Status start(const std::vector<std::string>& pd_paths);
  void finish();
  void tick(double dt);

  int createInstance(const std::vector<std::string>& paths) {
    return instances_.createInstance(paths);
  }
  void destroyInstance(int id) { instances_.destroyInstance(id); }

  Status eventInstance(const std::string& path,
                       std::shared_ptr<SoundtrackEvent>* out,
                       int instance_id = 0);

private:
  InstanceManager instances_;
  // Group events by instance for efficient per-instance processing
  std::map<int, std::vector<std::shared_ptr<SoundtrackEvent>>> events_by_inst_;
  // OpenAL: device, context, AudioServer, etc.
};

} // namespace vorpal
```

### 7.4 VORPALModule (Godot wrapper)
```cpp path=null start=null
using namespace godot;

class VORPALModule: public Object {
  GDCLASS(VORPALModule, Object)

public:
  bool start(const String& path);
  void finish();
  void tick(double dt);

  // Multi-instance management
  int create_instance();
  void destroy_instance(int instance_id);

  // Event creation bound to a specific instance (default 0 for backward compatibility)
  int event_instance(const String& name, int instance_id = 0);

  void push_command(int event_id, const String& cmd);
  void push_command_1f(int event_id, const String& cmd, float arg);
  void set_event_position(int event_id, float x, float y, float z);

protected:
  static void _bind_methods();

private:
  vorpal::Engine engine_;
  std::vector<std::shared_ptr<vorpal::SoundtrackEvent>> events_;
  std::unordered_map<int, int> event_to_instance_; // event_id -> instance_id
};
```

---

## 8. Godot Integration Details (GDExtension + GDScript)

### 8.1 GDExtension bindings
```cpp path=null start=null
void VORPALModule::_bind_methods() {
  ClassDB::bind_method(D_METHOD("start", "path"), &VORPALModule::start);
  ClassDB::bind_method(D_METHOD("finish"), &VORPALModule::finish);
  ClassDB::bind_method(D_METHOD("tick", "delta"), &VORPALModule::tick);

  ClassDB::bind_method(D_METHOD("create_instance"), &VORPALModule::create_instance);
  ClassDB::bind_method(D_METHOD("destroy_instance", "instance_id"), &VORPALModule::destroy_instance);

  ClassDB::bind_method(D_METHOD("event_instance", "name", "instance_id"),
                       &VORPALModule::event_instance, DEFVAL(0));

  ClassDB::bind_method(D_METHOD("push_command", "event_id", "cmd"), &VORPALModule::push_command);
  ClassDB::bind_method(D_METHOD("push_command_1f", "event_id", "cmd", "arg"), &VORPALModule::push_command_1f);
  ClassDB::bind_method(D_METHOD("set_event_position", "event_id", "x", "y", "z"), &VORPALModule::set_event_position);
}
```

### 8.2 GDScript usage — basic multi-instance
```gdscript path=null start=null
extends Node

var vorpal: VORPALModule
var music_inst: int
var sfx_inst: int
var music_event: int
var step_event: int

func _ready():
  vorpal = VORPALModule.new()
  vorpal.start("res://audio/patches")

  music_inst = vorpal.create_instance()
  sfx_inst = vorpal.create_instance()

  music_event = vorpal.event_instance("battle_music", music_inst)
  step_event = vorpal.event_instance("step_sfx", sfx_inst)

func _process(delta):
  vorpal.tick(delta)

func on_player_step():
  vorpal.push_command(step_event, "start")

func on_intensity_changed(level: float):
  vorpal.push_command_1f(music_event, "intensity", level)

func _exit_tree():
  vorpal.destroy_instance(music_inst)
  vorpal.destroy_instance(sfx_inst)
  vorpal.finish()
```

### 8.3 GDScript usage — dynamic instance pool
```gdscript path=null start=null
extends Node

var vorpal: VORPALModule
var pool := {} # instance_id -> usage_count
const MAX_EVENTS_PER_INSTANCE := 8

func _ready():
  vorpal = VORPALModule.new()
  vorpal.start("res://audio/patches")

func create_spatial_event(patch_name: String, pos: Vector3) -> int:
  var inst_id = _find_or_create_instance()
  var eid = vorpal.event_instance(patch_name, inst_id)
  vorpal.set_event_position(eid, pos.x, pos.y, pos.z)
  return eid

func _find_or_create_instance() -> int:
  for id in pool.keys():
    if pool[id] < MAX_EVENTS_PER_INSTANCE:
      pool[id] += 1
      return id
  var new_id = vorpal.create_instance()
  pool[new_id] = 1
  return new_id
```

---

## 9. libpd and Pure Data Integration Details
- Multi-instance prerequisite:
  - Build libpd with `make MULTI=true` or manually set CFLAGS `-DPDINSTANCE -DPDTHREADS`
  - C++ programs using PdBase.hpp must define PDINSTANCE via CPPFLAGS `-DPDINSTANCE`
  - The C++ wrapper `PdBase` will wrap separate libpd instances when PDINSTANCE is defined
- PdBase lifecycle per instance:
  - `PdBase::init(in, out, sampleRate, queued)`; then `computeAudio(true)`
  - Per tick: `processFloat(TICK_RATIO, inbuf, outbuf)`; MVP uses null buffers (internal pull)
- `$0` addressing:
  - Each opened patch has a unique `$0`; commands sent to `$0-command`; audio read from `vorpal-bus-$0` arrays (64-sample aligned)
- Search paths:
  - Call `addToSearchPath()` per instance for content roots
- Command format:
  - MVP: numeric list args; extend to (float/symbol) atom sequences as needed

```cpp path=null start=null
// Send a simple message to a patch’s command inlet
pd.startMessage();
pd.addFloat(1.0f);
pd.finishMessage(dz + "-command", "start");
```

---

## 10. OpenAL Integration Details
- Device/context (Engine-level shared):
  - `alcOpenDevice(nullptr)`, `alcCreateContext`, `alcMakeContextCurrent`
- Sources/buffers:
  - Maintain a free-buffer queue; triple-buffer per source to avoid underruns
  - Stream per-event audio each tick; queue new buffers; unqueue processed ones
- 3D spatialization:
  - Use `alSource3f(AL_POSITION, x,y,z)`; set listener pose from camera

```cpp path=null start=null
// Queue a float32 stereo buffer for streaming
alBufferData(buffer, AL_FORMAT_STEREO_FLOAT32, pcm, num_frames * 2 * sizeof(float), 44100);
alSourceQueueBuffers(source, 1, &buffer);
ALint state; alGetSourcei(source, AL_SOURCE_STATE, &state);
if (state != AL_PLAYING) alSourcePlay(source);
```

Performance notes:
- Samples per tick = `PdBase::blockSize() * TICK_RATIO` (typically 64)
- Keep buffer durations short (<= ~10 ms) to minimize latency; use multiple queued buffers

---

## 11. Risks and Mitigations
- Macro mismatch: `PD_MULTI_INSTANCE` vs `PDINSTANCE`
  - Mitigate: define PDINSTANCE for libpd; verify with `PdBase::numInstances()`
- OpenAL source exhaustion with many instances/events
  - Mitigate: voice budget, priority-based stealing, per-instance quotas
- Threading hazards
  - Mitigate: single-thread MVP; later lock-free queues and actor-style processing
- Memory growth per instance (patches, buffers)
  - Mitigate: instance caps; pooling; hibernation of idle instances
- Backward compatibility
  - Mitigate: default `instance_id=0` path preserves old behavior and APIs

---

## 12. Acceptance Criteria
Functional:
- Create ≥3 PDInstance objects; each loads and runs independent patches
- Commands route to intended instance; no cross-talk
- Audio renders concurrently without dropouts (with 2 instances at 44100 Hz)
- `$0` uniqueness verified per patch and instance

Performance:
- CPU scales roughly linearly with active instances; no frame hitching
- Tick processing < 5 ms @ 60 fps for 2 instances on typical dev hardware

API/Integration:
- VORPALModule exposes `create_instance`, `destroy_instance`, `event_instance(name, instance_id)`
- Default `instance_id=0` (single instance) works with existing demos
- libpd PDINSTANCE compiled-in; Godot extension initializes and runs

Testing:
- Unit tests for InstanceManager (create/destroy/get)
- Integration test: two instances, same patch, different commands
- Stress test: burst events; verify voice-stealing policy

---

## 13. Future Work and References
Enhancements:
- Parallel PD processing per instance
- Advanced voice priority (distance, category, loudness)
- Hot-reload patches; state save/restore
- Editor tooling: instance inspector; meters; OSC/MIDI routing

References:
- libpd: https://github.com/libpd/libpd
- Godot GDExtension: https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/
- OpenAL Soft: https://openal-soft.org/
- VORPAL Thesis (2017): system requirements, architecture, and real-time soundtrack design

---

## 14. libpd Documentation and Code Reference

When implementing multi-instance support, developers should reference the libpd library documentation and example code:

### Primary Documentation Sources
- **libpd Wiki**: https://github.com/libpd/libpd/wiki (official documentation)
- **Main README**: `/home/haruo/ic-vorpal/Vorpal-GDExtension/vorpal/externals/libpd/README.md`
  - Comprehensive build instructions, multi-instance setup, C++ wrapper details
  - Critical information on PDINSTANCE vs PD_MULTI_INSTANCE compilation flags

### Code Examples and Samples
The libpd samples directory contains practical C++ examples:
- **Location**: `/home/haruo/ic-vorpal/Vorpal-GDExtension/vorpal/externals/libpd/samples/cpp/`
- **Key Examples**:
  - `pdtest/`: Basic PdBase usage, PdReceiver callbacks
  - `pdtest_multi/`: Multi-instance demonstration (critical for this project)
  - `pdtest_jack/`: JACK audio integration patterns
  - `pdtest_rtaudio/`: RtAudio integration (similar to OpenAL patterns)
  - `pdtest_freeverb/`: Audio processing example

### Critical Code References
- **PdObject.h**: Custom receiver classes, callback patterns for print/bang/float/symbol/list/message
- **Multi-instance compilation**: Ensure libpd built with `make MULTI=true` and `-DPDINSTANCE -DPDTHREADS`
- **C++ wrapper usage**: Header-only library requiring `PdBase.hpp` and `util` directory inclusion

### Multi-Instance Specific Notes
- Each `PdBase` instance wraps a separate libpd instance when `PDINSTANCE` is defined
- libpd library must be built with both `-DPDINSTANCE -DPDTHREADS` flags
- External objects need recompilation with `-DPDINSTANCE -DPDTHREADS` for compatibility
- C++ programs using multi-instance PdBase need `-DPDINSTANCE` in CPPFLAGS
- Sample rate and buffer size management per instance

### Search Commands for Documentation
```bash path=null start=null
# Find all README and documentation files
find /home/haruo/ic-vorpal/Vorpal-GDExtension/vorpal/externals/libpd -name "*.md" -o -name "README*" -o -name "*.txt"

# Search for multi-instance related code
grep -r "PDINSTANCE\|multi.*instance" /home/haruo/ic-vorpal/Vorpal-GDExtension/vorpal/externals/libpd/samples/cpp/

# Find C++ header files with API definitions
find /home/haruo/ic-vorpal/Vorpal-GDExtension/vorpal/externals/libpd/cpp -name "*.h" -o -name "*.hpp"
```

---

## Implementation Checklist (MVP)
- [ ] Tracking: VORPAL Multi-Instance Support (WARP MVP) — https://github.com/haruo-gabriel/vorpal/issues/7
- [x] Step 1: Enable libpd multi-instances (PDINSTANCE/PDTHREADS) — https://github.com/haruo-gabriel/vorpal/issues/1 ✅ COMPLETE
- [x] Step 2: Refactor DSPServer into per-instance PDInstance (no globals) — https://github.com/haruo-gabriel/vorpal/issues/2 ✅ COMPLETE
  - ✅ Replaced global `pd::PdBase dsp` with per-instance `PDInstance::pd_`
  - ✅ Moved command queues into PDInstance scope
  - ✅ Maintained `$0-command` and `vorpal-bus-$0` conventions
  - ✅ Removed global `units__` registry (now per-instance)
  - ✅ Each PDInstance manages its own patch lifecycle
  - ✅ Implemented `handleCommands()` and `processTick()` methods
  - ✅ Per-instance unit registry for true isolation
  - ✅ Tests pass: instancemanager_test, pdinstance_test, pd_multi_test
- [x] Step 3: Introduce InstanceManager — https://github.com/haruo-gabriel/vorpal/issues/3 ✅ COMPLETE
  - ✅ InstanceManager class implemented with `Map<int, unique_ptr<PDInstance>>`
  - ✅ Integrated into DSPServer as static member
  - ✅ Default instance (id=0) for backward compatibility
  - ✅ Auto-ID generation API: `createInstance(paths)` returns int (WARP spec compliant)
  - ✅ Manual ID API: `createInstance(id, paths)` for internal use
  - ✅ Methods: `destroyInstance(id)`, `get(id)`, `ids()`, `defaultInstance()`, `findInstanceByPatchDollar()`
  - ✅ Graceful failure handling (returns -1 or false on creation failure)
  - ✅ Tests pass: instancemanager_test validates create/destroy/get/duplicate detection
- [ ] Step 4: Engine changes for multi-instance tick and event grouping — https://github.com/haruo-gabriel/vorpal/issues/4
  - [x] **Phase 1**: Update `DSPServer::loadUnit(path, instance_id=0)` to accept instance parameter ✅ COMPLETE
    - ✅ Updated signature with default parameter `instance_id=0`
    - ✅ Routes unit creation to `instance_manager.get(instance_id)`
    - ✅ Updated `Engine::eventInstance()` caller to pass instance_id
    - ✅ Backward compatibility maintained (default parameter)
    - ✅ Tests pass: instancemanager_test, pdinstance_test, engine_multiinstance_test
  - [x] **Phase 2**: Move InstanceManager ownership from DSPServer to Engine ✅ COMPLETE
    - ✅ Added `InstanceManager instances_;` private member to Engine class
    - ✅ Added `#include <vorpal/instancemanager.h>` to engine.h
    - ✅ Removed `static InstanceManager instance_manager` from DSPServer
    - ✅ Updated all DSPServer methods to accept `InstanceManager&` parameter
    - ✅ Forward declaration added to dspserver.h for InstanceManager
    - ✅ Engine::start() passes `instances_` to DSPServer::start()
    - ✅ Engine::tick() passes `instances_` to DSPServer methods
    - ✅ Engine::finish() passes `instances_` to DSPServer::finish()
    - ✅ Tests pass: instancemanager_test, pdinstance_test, engine_multiinstance_test
  - [x] **Phase 3**: Implement Engine multi-instance API ✅ COMPLETE
    - ✅ `Engine::createInstance(paths)` → delegates to InstanceManager
    - ✅ `Engine::destroyInstance(id)` → delegates to InstanceManager
    - ✅ `Engine::instanceManager()` → accessor for DSPServer
    - ✅ `Engine::eventInstance(path, out, instance_id=0)` updated to pass instances_ to loadUnit
    - ✅ Tests pass: engine_multiinstance_test validates create/destroy/event-binding
    - ⚠️ Known Issue: Destroying instances with active units causes dangling pointer in to_be_closed__ queue (TODO for future)
  - [ ] **Phase 4**: Update tick loop to iterate all instances via InstanceManager
  - [ ] **Phase 4**: Group events by instance_id: `std::map<int, std::vector<std::shared_ptr<SoundtrackEvent>>> events_by_inst_`
- [ ] Step 5: Godot GDExtension API surface for instances — https://github.com/haruo-gabriel/vorpal/issues/5
  - Expose `VORPALModule::create_instance()` in GDScript
  - Expose `VORPALModule::destroy_instance(instance_id)`
  - Update `VORPALModule::event_instance(name, instance_id=0)` with default parameter
  - Update GDExtension bindings (_bind_methods)
- [ ] Step 6: Testing and Acceptance Criteria — https://github.com/haruo-gabriel/vorpal/issues/6
  - Integration test: Two instances, same patch, different commands, verify isolation
  - Stress test: Burst events across multiple instances
  - Performance test: CPU scaling with instance count
  - Audio quality test: No dropouts with 2+ concurrent instances

## Current Architecture Status (as of Issue #4 Phase 2-3 completion)

### ✅ Completed Components
- **PDInstance**: Per-instance libpd wrapper with isolated state (patches, commands, units)
- **InstanceManager**: Registry and lifecycle management for PDInstance objects with auto-ID generation
- **Engine**: Now owns InstanceManager and exposes multi-instance API (createInstance/destroyInstance)
- **DSPServer**: Refactored to accept InstanceManager reference; no longer has static instance_manager
- **Per-instance unit registry**: Units register with owning instance for O(N+M) performance

### ✅ Architectural Alignment (WARP §7.3)
**Current Reality (as of Phase 2-3 completion):**
- ✅ InstanceManager **owned by Engine** (as `Engine::instances_` private member)
- ✅ Engine **exposes multi-instance API**: `createInstance()`, `destroyInstance()`, `instanceManager()`
- ✅ `DSPServer::loadUnit(path, instance_manager, instance_id=0)` **accepts InstanceManager reference**
- ✅ `Engine::eventInstance(path, out, instance_id=0)` **accepts instance_id parameter**
- ✅ DSPServer is now a utility layer (no static InstanceManager)
- ⚠️ `Engine::tick()` processes all instances via InstanceManager (Phase 4 goal: per-instance event grouping)

**Architecture Achievement:**
- Matches WARP specification §7.3: Engine owns InstanceManager
- Multi-instance API surface complete at C++ level
- Backward compatibility maintained with default instance_id=0

### 🔄 In Progress / Next Steps (Issue #4)
- ✅ **Phase 1 COMPLETE**: DSPServer::loadUnit and Engine::eventInstance accept instance_id parameter
- ✅ **Phase 2 COMPLETE**: Move InstanceManager ownership from DSPServer to Engine (WARP alignment achieved!)
- ✅ **Phase 3 COMPLETE**: Engine exposes multi-instance API (createInstance/destroyInstance)
- ⏭️ **Phase 4 NEXT**: Multi-instance tick processing and event grouping in Engine ← CURRENT PRIORITY
- **Future (Issue #5)**: Godot GDExtension bindings for multi-instance API

### 📋 Remaining Work
Loading video...


#### Immediate Next Steps (Priority Order) - Issue #4

**Phase 1: DSPServer::loadUnit API Update** ✅ COMPLETE
1. ✅ Changed signature: `shared_ptr<DSPUnit> loadUnit(const string& path, int instance_id=0)`
2. ✅ Routes unit creation to `instance_manager.get(instance_id)->loadPatch()`
3. ✅ Updated `Engine::eventInstance()` signature to accept and pass instance_id
4. ✅ Backward compatibility maintained with default instance_id=0
5. ✅ New test: `engine_multiinstance_test` validates multi-instance event creation

**Phase 2: Move InstanceManager to Engine (Architectural Refactor)** ✅ COMPLETE
1. ✅ Added `InstanceManager instances_;` member to Engine class (private)
2. ✅ Removed `static InstanceManager instance_manager` from DSPServer
3. ✅ Updated all DSPServer methods to accept `InstanceManager&` reference
4. ✅ Engine passes `instances_` to all DSPServer methods (start, loadUnit, tick, handleCommands, cleanUp, finish)
5. ✅ Forward declaration added to dspserver.h for InstanceManager
6. ✅ Tests pass: instancemanager_test, pdinstance_test, engine_multiinstance_test

**Phase 3: Engine Multi-Instance API** ✅ COMPLETE
1. ✅ Implemented `int Engine::createInstance(paths)` → delegates to `instances_.createInstance(paths)`
2. ✅ Implemented `void Engine::destroyInstance(id)` → delegates to `instances_.destroyInstance(id)`
3. ✅ Added `InstanceManager& Engine::instanceManager()` accessor
4. ✅ Modified `Status Engine::eventInstance(path, out, instance_id=0)` to pass instances_ to loadUnit
5. ✅ Group events: replace `vector<weak_ptr<SoundtrackEvent>> events__` with `map<int, vector<...>> events_by_inst_` (partially done)
6. ✅ Tests updated: engine_multiinstance_test validates createInstance/destroyInstance/event-binding
7. ⚠️ Known Issue: Destroying instances with active units causes dangling pointer in to_be_closed__ queue (deferred to future work)

**Phase 4: Multi-Instance Tick Processing** ⏭️ NEXT
1. Update `Engine::tick(dt)` to iterate `instances_.ids()`
2. Per instance: call `handleCommands()` → `processTick()`
3. Process events grouped by instance_id
4. Stream per-instance audio to OpenAL

**Future (Issue #5): Godot GDExtension**
- VORPALModule wrapper methods
- GDScript bindings
- Example usage in demo project

## Next improvements:
- ✅ ~~Convert UnitImpl::units__ into a per-instance registry~~ **DONE** (completed as part of Issue #2)
- ✅ ~~Add auto-ID overload: `InstanceManager::createInstance(paths)` returns int~~ **DONE** (completed in Issue #3)
- ✅ ~~Issue #4 Phase 1: Update loadUnit to accept instance_id argument~~ **DONE**
  - ✅ Modified `DSPServer::loadUnit(path, instance_id=0)` signature
  - ✅ Routes unit creation to specified instance via `instance_manager.get(instance_id)`
  - ✅ Updated `Engine::eventInstance(path, out, instance_id=0)` signature and implementation
  - ✅ Backward compatibility maintained with default parameters
  - ✅ Tests: instancemanager_test, pdinstance_test, engine_multiinstance_test all pass
- ✅ ~~Issue #4 Phase 2: Move InstanceManager from DSPServer to Engine~~ **DONE**
  - ✅ Engine owns `InstanceManager instances_` (per WARP §7.3)
  - ✅ DSPServer methods accept InstanceManager& reference
  - ✅ Architectural alignment with WARP specification achieved
- ✅ ~~Issue #4 Phase 3: Engine multi-instance API~~ **DONE**
  - ✅ `Engine::createInstance()` / `Engine::destroyInstance()` implemented
  - ✅ `Engine::instanceManager()` accessor added
  - ✅ Tests validate multi-instance event creation and binding
- **Issue #4 Phase 4: Multi-instance tick and event grouping** ← CURRENT PRIORITY
  - Update `Engine::tick()` to process all instances
  - Group events by instance_id for efficient per-instance streaming
- **Issue #5: Expose multi-instance API to Godot GDExtension (VORPALModule)**
  - Bind createInstance/destroyInstance/eventInstance to GDScript
  - Update VORPALModule wrapper
  - Create GDScript demo showing multi-instance usage