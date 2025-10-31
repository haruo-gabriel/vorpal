#include <iostream>

#include <vorpal/instancemanager.h>
#include <vorpal/pdinstance.h>

int main() {
  using namespace vorpal;

  std::cout << "InstanceManager test start" << std::endl;

  InstanceManager mgr;

  // Create instance 0 and ensure it starts
  bool ok = mgr.createInstance(0, {"patches/instancemanager_test"}, 44100);
  if (!ok) {
    std::cerr << "Failed to create default instance 0" << std::endl;
    return 2;
  }

  PDInstance* inst0 = mgr.get(0);
  if (!inst0) {
    std::cerr << "mgr.get(0) returned nullptr" << std::endl;
    return 3;
  }

  // Load a patch, then close it
  std::string dz = inst0->loadPatch("pdinstance_a.pd");
  if (dz.empty()) {
    std::cerr << "failed to open patch in instance 0" << std::endl;
    mgr.destroyInstance(0);
    return 4;
  }
  std::cout << "opened patch dz=" << dz << std::endl;

  inst0->closePatch(dz);

  // Negative test: creating the same instance id again should fail
  bool duplicate = mgr.createInstance(0, {"patches/instancemanager_test"}, 44100);
  if (duplicate) {
    std::cerr << "NEGATIVE TEST FAILED: duplicate instance creation returned true" << std::endl;
    mgr.destroyInstance(0);
    return 6;
  } else {
    std::cout << "NEGATIVE TEST OK: duplicate instance creation rejected" << std::endl;
  }

  // Negative test: load a missing patch on a new instance
  bool ok2 = mgr.createInstance(5, {"patches/instancemanager_test"}, 44100);
  if (!ok2) {
    std::cerr << "Warning: could not create instance 5 for missing-patch test" << std::endl;
  } else {
    PDInstance* inst5 = mgr.get(5);
    std::string dz_missing = inst5->loadPatch("this_patch_does_not_exist.pd");
    if (!dz_missing.empty()) {
      std::cerr << "NEGATIVE TEST FAILED: unexpected patch opened in instance 5: " << dz_missing << std::endl;
      mgr.destroyInstance(5);
      mgr.destroyInstance(0);
      return 7;
    } else {
      std::cout << "NEGATIVE TEST OK: missing patch failed to open in instance 5" << std::endl;
    }
    mgr.destroyInstance(5);
  }

  // Destroy and ensure get returns null
  mgr.destroyInstance(0);
  if (mgr.get(0) != nullptr) {
    std::cerr << "instance 0 still present after destroy" << std::endl;
    return 5;
  }

  std::cout << "InstanceManager test finished" << std::endl;
  return 0;
}
