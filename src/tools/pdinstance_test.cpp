#include <iostream>
#include <thread>
#include <chrono>

#include "pdinstance.h"

#include <cmath>

static bool checkFloat(const char *label, float actual, float expected, float eps = 1e-6f) {
  if (std::fabs(actual - expected) > eps) {
    std::cerr << "CHECK FAILED: " << label << " actual=" << actual << " expected=" << expected << "\n";
    return false;
  }
  std::cout << "CHECK OK: " << label << " = " << actual << "\n";
  return true;
}

int main() {
  using namespace vorpal;
  std::cout << "PDInstance stress test start" << std::endl;

  const int NUM_INSTANCES = 6; // create multiple instances to stress libpd
  std::vector<std::unique_ptr<PDInstance>> instances;
  instances.reserve(NUM_INSTANCES);

  for (int i = 0; i < NUM_INSTANCES; ++i) {
    instances.emplace_back(new PDInstance(100 + i));
    bool started = instances.back()->start({"../patches"}, 44100, true, 1, 2);
    if (!started) {
      std::cerr << "Failed to start instance " << (100 + i) << std::endl;
      return 3;
    }
  }

  // Load the same patch into each instance
  std::vector<std::string> dzs;
  for (int i = 0; i < NUM_INSTANCES; ++i) {
    dzs.push_back(instances[i]->loadPatch("pdinstance_a.pd"));
    std::cout << "inst " << i << " dz=" << dzs.back() << std::endl;
  }

  // Directly write different float arrays per instance and then read back to verify independence
  bool ok = true;
  std::vector<float> out;
  for (int i = 0; i < NUM_INSTANCES; ++i) {
    if (dzs[i].empty()) continue;
    float expected = 0.1f * (i + 1);
    std::vector<float> src(64, expected);
    // write into the per-instance array named 'vorpal-bus-A'
    instances[i]->writeArray(std::string("vorpal-bus-A"), src);
    int n = instances[i]->readBus("A", out, 64);
    float got = n ? out[0] : 0.0f;
    std::string label = "inst-" + std::to_string(i) + " first";
    ok = ok && checkFloat(label.c_str(), got, expected, 1e-6f);
  }

  // Optional: run a time-bound stress loop to simulate longer activity
  const int STRESS_SECONDS = 3;
  const auto stress_end = std::chrono::steady_clock::now() + std::chrono::seconds(STRESS_SECONDS);
  while (std::chrono::steady_clock::now() < stress_end) {
    for (int i = 0; i < NUM_INSTANCES; ++i) {
      instances[i]->handleCommands();
      instances[i]->processTick();
    }
    // small sleep to avoid pegging CPU in this synthetic test
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  // Negative test: attempt to open a missing patch in a fresh instance
  {
    std::unique_ptr<PDInstance> missing(new PDInstance(9999));
    bool started = missing->start({"../patches"}, 44100, true, 1, 2);
    if (!started) {
      std::cerr << "Warning: could not start instance for missing-patch test" << std::endl;
    } else {
      std::string dz_missing = missing->loadPatch("this_patch_does_not_exist.pd");
      if (!dz_missing.empty()) {
        std::cerr << "NEGATIVE TEST FAILED: unexpected patch opened: " << dz_missing << std::endl;
        ok = false;
      } else {
        std::cout << "NEGATIVE TEST OK: missing patch did not open (as expected)" << std::endl;
      }
      missing->finish();
    }
  }

  // shutdown
  for (auto &inst : instances) inst->finish();

  std::cout << "PDInstance stress test finished" << std::endl;
  return ok ? 0 : 4;
}
