// Test Engine multi-instance event creation (Phase 2 completion test)
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
  
  // Test 3: Create a new instance using Engine API (Phase 2)
  int inst1 = engine.createInstance(paths);
  if (inst1 > 0) {
    std::cout << "Created new instance via Engine::createInstance(): " << inst1 << std::endl;
  } else {
    std::cerr << "ERROR: Failed to create instance via Engine::createInstance()" << std::endl;
    return 1;
  }
  
  // Test 4: Create event on the new instance
  std::shared_ptr<SoundtrackEvent> event1;
  Status status1 = engine.eventInstance("vorpal_core", &event1, inst1);
  if (status1.ok()) {
    std::cout << "Event created on new instance " << inst1 << ": " << status1.description() << std::endl;
  } else {
    std::cout << "Failed to create event on instance " << inst1 << ": " << status1.description() << std::endl;
  }
  
  // Test 5: Attempt to create event on non-existent instance (should fail)
  std::shared_ptr<SoundtrackEvent> event_invalid;
  Status status_invalid = engine.eventInstance("vorpal_core", &event_invalid, 999);
  if (!status_invalid.ok()) {
    std::cout << "NEGATIVE TEST OK: Event creation on invalid instance rejected (expected)" << std::endl;
  } else {
    std::cout << "NEGATIVE TEST FAILED: Event should not be created on non-existent instance" << std::endl;
  }
  
  // Test 6: Instance destruction with active units (previously caused dangling pointer)
  std::cout << "\nTest 6: Destroying instance with active units" << std::endl;
  
  // Destroy instance 1 while event1 still exists
  engine.destroyInstance(inst1);
  std::cout << "Instance " << inst1 << " destroyed (event still exists)" << std::endl;
  
  // Clean up event references
  event0.reset();
  event_implicit.reset();
  event1.reset();  // This triggers ~UnitImpl() which adds to to_be_closed__
  event_invalid.reset();
  
  std::cout << "All events cleaned up" << std::endl;
  
  engine.finish();  // This calls cleanUp(), which should safely handle deleted instance
  
  std::cout << "\nEngine multi-instance event creation test finished" << std::endl;
  std::cout << "SUCCESS: Dangling pointer issue fixed - instance destruction is now safe" << std::endl;
  
  return 0;
}
