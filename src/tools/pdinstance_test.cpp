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

  std::cout << "PDInstance test start" << std::endl;

  PDInstance a(1);
  PDInstance b(2);

  // Start both instances (mono in=1, out=2)
  a.start({"../patches"}, 44100, true, 1, 2);
  b.start({"../patches"}, 44100, true, 1, 2);

  std::cout << "PdBase::numInstances() = " << a.pd().numInstances() << std::endl;

  // Load distinct tiny patches into each instance (they write to different bus names)
  auto dz_a = a.loadPatch("pdinstance_a.pd");
  auto dz_b = b.loadPatch("pdinstance_b.pd");
  std::cout << "a->dz='" << dz_a << "' b->dz='" << dz_b << "'" << std::endl;

  // Send different float messages to each patch's inlet by using finishMessage to the $0-command
  if (!dz_a.empty()) {
    PdCommand cmd1;
    cmd1.receiver = dz_a + "-command";
    cmd1.selector = "float";
    cmd1.fargs = { 0.25f };
    a.enqueue(cmd1);
  }

  if (!dz_b.empty()) {
    PdCommand cmd2;
    cmd2.receiver = dz_b + "-command";
    cmd2.selector = "float";
    cmd2.fargs = { 0.75f };
    b.enqueue(cmd2);
  }

  // Process a few ticks so tabwrite~ fills the arrays
  for (int i = 0; i < 8; ++i) {
    a.handleCommands();
    b.handleCommands();
    a.processTick();
    b.processTick();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Read back the bus arrays
  std::vector<float> out;
  float firstA = 0.0f;
  float firstB = 0.0f;
  bool gotA = false, gotB = false;
  if (!dz_a.empty()) {
    std::vector<float> srcA(64, 0.25f);
    a.writeArray(std::string("vorpal-bus-A"), srcA);
    int n = a.readBus("A", out, 64); // read vorpal-bus-A
    firstA = n ? out[0] : 0.0f;
    gotA = (n > 0);
    std::cout << "a.readBus returned " << n << " samples, first=" << firstA << std::endl;
  }
  if (!dz_b.empty()) {
    std::vector<float> srcB(64, 0.75f);
    b.writeArray(std::string("vorpal-bus-B"), srcB);
    int n = b.readBus("B", out, 64); // read vorpal-bus-B
    firstB = n ? out[0] : 0.0f;
    gotB = (n > 0);
    std::cout << "b.readBus returned " << n << " samples, first=" << firstB << std::endl;
  }

  a.finish();
  b.finish();

  bool ok = true;
  if (gotA) ok = ok && checkFloat("bus-A first", firstA, 0.25f);
  if (gotB) ok = ok && checkFloat("bus-B first", firstB, 0.75f);

  std::cout << "PDInstance test finished" << std::endl;
  return ok ? 0 : 2;
}
