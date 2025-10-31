#include <iostream>

#include <vorpal/instancemanager.h>
#include <vorpal/pdinstance.h>

int main() {
  using namespace vorpal;

  std::cout << "InstanceManager auto-ID test start" << std::endl;

  InstanceManager mgr;

  // Test auto-ID generation
  int id1 = mgr.createInstance({"patches/instancemanager_autoid_test"}, 44100);
  if (id1 < 0) {
    std::cerr << "Failed to create instance with auto-ID" << std::endl;
    return 1;
  }
  std::cout << "Created instance with auto-ID: " << id1 << std::endl;

  // Verify instance exists and works
  PDInstance* inst1 = mgr.get(id1);
  if (!inst1) {
    std::cerr << "mgr.get(" << id1 << ") returned nullptr" << std::endl;
    return 2;
  }

  // Load a patch to verify instance is functional
  std::string dz1 = inst1->loadPatch("pdinstance_a.pd");
  if (dz1.empty()) {
    std::cerr << "failed to open patch in instance " << id1 << std::endl;
    mgr.destroyInstance(id1);
    return 3;
  }
  std::cout << "Instance " << id1 << " opened patch dz=" << dz1 << std::endl;

  // Create another instance - should get next ID
  int id2 = mgr.createInstance({"patches/instancemanager_autoid_test"}, 44100);
  if (id2 < 0) {
    std::cerr << "Failed to create second instance with auto-ID" << std::endl;
    mgr.destroyInstance(id1);
    return 4;
  }
  std::cout << "Created second instance with auto-ID: " << id2 << std::endl;

  // Verify IDs are different and sequential
  if (id2 <= id1) {
    std::cerr << "Auto-ID not incrementing correctly: id1=" << id1 << ", id2=" << id2 << std::endl;
    mgr.destroyInstance(id1);
    mgr.destroyInstance(id2);
    return 5;
  }
  std::cout << "CHECK OK: Auto-IDs are sequential (" << id1 << " < " << id2 << ")" << std::endl;

  // Verify second instance works independently
  PDInstance* inst2 = mgr.get(id2);
  if (!inst2) {
    std::cerr << "mgr.get(" << id2 << ") returned nullptr" << std::endl;
    mgr.destroyInstance(id1);
    mgr.destroyInstance(id2);
    return 6;
  }

  std::string dz2 = inst2->loadPatch("pdinstance_b.pd");
  if (dz2.empty()) {
    std::cerr << "failed to open patch in instance " << id2 << std::endl;
    mgr.destroyInstance(id1);
    mgr.destroyInstance(id2);
    return 7;
  }
  std::cout << "Instance " << id2 << " opened patch dz=" << dz2 << std::endl;

  // Test that manual ID creation still works alongside auto-ID
  bool manual_ok = mgr.createInstance(100, {"patches/instancemanager_autoid_test"}, 44100);
  if (!manual_ok) {
    std::cerr << "Manual ID creation (100) failed" << std::endl;
    mgr.destroyInstance(id1);
    mgr.destroyInstance(id2);
    return 8;
  }
  std::cout << "CHECK OK: Manual ID creation (100) works alongside auto-ID" << std::endl;

  // Verify manual instance exists
  PDInstance* inst100 = mgr.get(100);
  if (!inst100) {
    std::cerr << "mgr.get(100) returned nullptr" << std::endl;
    mgr.destroyInstance(id1);
    mgr.destroyInstance(id2);
    mgr.destroyInstance(100);
    return 9;
  }

  // Test ids() returns all active instances
  auto active_ids = mgr.ids();
  if (active_ids.size() != 3) {
    std::cerr << "Expected 3 active instances, got " << active_ids.size() << std::endl;
    mgr.destroyInstance(id1);
    mgr.destroyInstance(id2);
    mgr.destroyInstance(100);
    return 10;
  }
  std::cout << "CHECK OK: ids() returns all 3 active instances" << std::endl;

  // Cleanup
  mgr.destroyInstance(id1);
  mgr.destroyInstance(id2);
  mgr.destroyInstance(100);

  // Verify cleanup
  if (mgr.get(id1) != nullptr || mgr.get(id2) != nullptr || mgr.get(100) != nullptr) {
    std::cerr << "Instances still exist after destroy" << std::endl;
    return 11;
  }
  std::cout << "CHECK OK: All instances properly destroyed" << std::endl;

  std::cout << "InstanceManager auto-ID test finished successfully" << std::endl;
  return 0;
}
