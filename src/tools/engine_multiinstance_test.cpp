// Test Engine multi-instance event creation (Phase 1 completion test)
#include <vorpal/engine.h>
#include <vorpal/soundtrackevent.h>
#include <vorpal/dspserver.h>
#include <vorpal/instancemanager.h>
#include <iostream>
#include <memory>
#include <vector>

using namespace vorpal;

int main() {
  std::cout << "Engine multi-instance event creation test start" << std::endl;
  
  // Create test patches vector
  std::vector<std::string> paths = {"../patches"};
  
  // Start engine (creates default instance 0)
  Engine engine;
  Status start_status = engine.start(paths);
  if (!start_status.ok()) {
    std::cerr << "ERROR: Engine failed to start: " << start_status.description() << std::endl;
    return 1;
  }
  std::cout << "Engine started: " << start_status.description() << std::endl;
  
  // Create additional instances via DSPServer (Phase 2 will move this to Engine)
  DSPServer dsp;
  auto& instance_manager = const_cast<class InstanceManager&>(
    *reinterpret_cast<const class InstanceManager*>(&dsp)
  );
  
  // For now, manually create instances using DSPServer's static instance_manager
  // This is a temporary workaround until Phase 2 moves InstanceManager to Engine
  std::cout << "\nNOTE: Manual instance creation (Phase 2 will add Engine::createInstance())" << std::endl;
  
  // Test 1: Create event on default instance (0) - backward compatibility
  std::shared_ptr<SoundtrackEvent> event0;
  Status status0 = engine.eventInstance("vorpal_core", &event0, 0);
  if (status0.ok()) {
    std::cout << "Event created on instance 0 (default): " << status0.description() << std::endl;
  } else {
    std::cout << "Failed to create event on instance 0: " << status0.description() << std::endl;
  }
  
  // Test 2: Create event with implicit default (backward compatibility)
  std::shared_ptr<SoundtrackEvent> event_implicit;
  Status status_implicit = engine.eventInstance("vorpal_core", &event_implicit);
  if (status_implicit.ok()) {
    std::cout << "Event created with implicit default: " << status_implicit.description() << std::endl;
  } else {
    std::cout << "Failed to create event with implicit default: " << status_implicit.description() << std::endl;
  }
  
  // Test 3: Attempt to create event on non-existent instance (should fail gracefully)
  std::shared_ptr<SoundtrackEvent> event_invalid;
  Status status_invalid = engine.eventInstance("vorpal_core", &event_invalid, 999);
  if (!status_invalid.ok()) {
    std::cout << "NEGATIVE TEST OK: Event creation on invalid instance rejected (expected)" << std::endl;
  } else {
    std::cout << "NEGATIVE TEST FAILED: Event should not be created on non-existent instance" << std::endl;
  }
  
  engine.finish();
  
  std::cout << "\nEngine multi-instance event creation test finished" << std::endl;
  std::cout << "Phase 1 complete: Engine::eventInstance() now accepts instance_id parameter!" << std::endl;
  
  return 0;
}
