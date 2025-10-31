// Command routing test - verify commands route to correct instance with no cross-talk
#include <vorpal/engine.h>
#include <vorpal/soundtrackevent.h>
#include <vorpal/instancemanager.h>
#include <vorpal/pdinstance.h>
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

using namespace vorpal;

// Helper function to check float values with tolerance
static bool checkFloat(const char* label, float actual, float expected, float eps = 0.01f) {
  if (std::fabs(actual - expected) > eps) {
    std::cerr << "CHECK FAILED: " << label 
              << " actual=" << actual 
              << " expected=" << expected << std::endl;
    return false;
  }
  std::cout << "CHECK OK: " << label 
            << " = " << actual 
            << " (expected " << expected << ")" << std::endl;
  return true;
}

int main() {
  std::cout << "Command routing test start" << std::endl;
  std::cout << "Goal: Verify commands route to correct instance with no cross-talk" << std::endl;
  
  // Setup: Create test patches vector
  std::vector<std::string> paths = {"../patches"};
  
  // Start engine (creates default instance 0)
  Engine engine;
  Status start_status = engine.start(paths);
  if (!start_status.ok()) {
    std::cerr << "ERROR: Engine failed to start: " << start_status.description() << std::endl;
    return 1;
  }
  std::cout << "Engine started: " << start_status.description() << std::endl;
  
  // Create second instance for testing
  int inst1 = engine.createInstance(paths);
  if (inst1 < 0) {
    std::cerr << "ERROR: Failed to create second instance" << std::endl;
    return 1;
  }
  std::cout << "Created second instance: " << inst1 << std::endl;

  // ===== SECTION 2: Load patches and create events =====
  std::cout << "\n=== Section 2: Loading patches and tracking dollarZero ===" << std::endl;
  
  // Get access to instances to load patches and track dollarZero values
  InstanceManager& mgr = engine.instanceManager();
  PDInstance* inst0 = mgr.get(0);
  PDInstance* inst1_ptr = mgr.get(inst1);
  
  if (!inst0 || !inst1_ptr) {
    std::cerr << "ERROR: Could not access instances" << std::endl;
    return 2;
  }
  
  // Load patches directly via PDInstance to get dollarZero values
  std::string dz0 = inst0->loadPatch("command_routing_test.pd");
  if (dz0.empty()) {
    std::cerr << "ERROR: Failed to load patch in instance 0" << std::endl;
    return 2;
  }
  std::cout << "Loaded patch in instance 0, dollarZero=" << dz0 << std::endl;
  
  std::string dz1 = inst1_ptr->loadPatch("command_routing_test.pd");
  if (dz1.empty()) {
    std::cerr << "ERROR: Failed to load patch in instance 1" << std::endl;
    return 2;
  }
  std::cout << "Loaded patch in instance 1, dollarZero=" << dz1 << std::endl;
  
  // Note: dollarZero may be the same across instances (Pure Data behavior)
  // but the arrays are still isolated per libpd instance
  if (dz0 == dz1) {
    std::cout << "NOTE: Both patches have same dollarZero (this is normal)" << std::endl;
    std::cout << "      Arrays are still isolated per libpd instance" << std::endl;
  } else {
    std::cout << "CHECK OK: dollarZero values are unique (" << dz0 << " != " << dz1 << ")" << std::endl;
  }
  
  // Note: We're loading patches directly, not creating SoundtrackEvents
  // This gives us finer control for testing command routing

  // ===== SECTION 3: Send commands directly to each instance =====
  std::cout << "\n=== Section 3: Sending commands ===" << std::endl;
  
  // Create commands for each instance
  // Instance 0: set-value 100.0
  PdCommand cmd0;
  cmd0.receiver = dz0 + "-command";
  cmd0.selector = "set-value";
  cmd0.params.push_back(Parameter(100.0f));
  inst0->enqueue(cmd0);
  std::cout << "Sent command to instance 0: " << cmd0.receiver << " " << cmd0.selector << " 100.0" << std::endl;
  
  // Instance 1: set-value 200.0
  PdCommand cmd1;
  cmd1.receiver = dz1 + "-command";
  cmd1.selector = "set-value";
  cmd1.params.push_back(Parameter(200.0f));
  inst1_ptr->enqueue(cmd1);
  std::cout << "Sent command to instance 1: " << cmd1.receiver << " " << cmd1.selector << " 200.0" << std::endl;
  
  std::cout << "Commands enqueued - ready for processing" << std::endl;

  // ===== SECTION 4: Process commands and verify isolation =====
  std::cout << "\n=== Section 4: Processing and verification ===" << std::endl;
  
  // Process commands and run DSP
  std::cout << "Handling commands in both instances..." << std::endl;
  inst0->handleCommands();
  inst1_ptr->handleCommands();
  
  std::cout << "Processing ticks..." << std::endl;
  inst0->processTick();
  inst1_ptr->processTick();
  
  std::cout << "\nReading results from arrays..." << std::endl;
  
  // Read arrays using PdBase::readArray (our patch creates "test-result-<dollarZero>")
  std::vector<float> result0;
  std::vector<float> result1;
  
  // Array names are "test-result-<dollarZero>"
  std::string array_name0 = "test-result-" + dz0;
  std::string array_name1 = "test-result-" + dz1;
  
  // Read array from instance 0
  bool read0_ok = inst0->pd().readArray(array_name0, result0);
  if (!read0_ok || result0.empty()) {
    std::cerr << "ERROR: Failed to read array from instance 0: " << array_name0 << std::endl;
    return 4;
  }
  std::cout << "Read " << result0.size() << " samples from instance 0 array: " << array_name0 << std::endl;
  
  // Read array from instance 1
  bool read1_ok = inst1_ptr->pd().readArray(array_name1, result1);
  if (!read1_ok || result1.empty()) {
    std::cerr << "ERROR: Failed to read array from instance 1: " << array_name1 << std::endl;
    return 4;
  }
  std::cout << "Read " << result1.size() << " samples from instance 1 array: " << array_name1 << std::endl;
  
  // Verify the values
  std::cout << "\n=== Verification ===" << std::endl;
  bool test_passed = true;
  
  // Check instance 0 received 100.0
  float value0 = result0[0];
  test_passed = test_passed && checkFloat("Instance 0 value", value0, 100.0f);
  
  // Check instance 1 received 200.0
  float value1 = result1[0];
  test_passed = test_passed && checkFloat("Instance 1 value", value1, 200.0f);
  
  // Final result
  if (test_passed) {
    std::cout << "\nTEST PASSED: Command routing verified - no cross-talk detected!" << std::endl;
    std::cout << "   Instance 0 received 100.0, Instance 1 received 200.0" << std::endl;
  } else {
    std::cerr << "\nTEST FAILED: Command routing has issues" << std::endl;
    return 5;
  }
  
  // Cleanup
  inst0->closePatch(dz0);
  inst1_ptr->closePatch(dz1);
  
  engine.finish();
  std::cout << "Command routing test finished" << std::endl;
  return 0;
}
