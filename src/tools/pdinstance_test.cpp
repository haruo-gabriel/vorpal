#include <iostream>
#include <thread>
#include <chrono>

#include "pdinstance.h"

int main() {
  using namespace vorpal;

  std::cout << "PDInstance test start" << std::endl;

  PDInstance a(1);
  PDInstance b(2);

  // Start both instances
  a.start({"../patches"});
  b.start({"../patches"});

  // For a safer smoke-test we won't open patches (some patches touch globals/GUI)
  // Instead just confirm instances initialize independently and can enqueue/handle
  std::cout << "PdBase::numInstances() = " << a.pd().numInstances() << std::endl;

  PdCommand cmd1;
  cmd1.receiver = "nop"; // no-op receiver; safe placeholder
  cmd1.selector = "bang";
  cmd1.fargs = { 0.0f };
  a.enqueue(cmd1);

  PdCommand cmd2;
  cmd2.receiver = "nop";
  cmd2.selector = "bang";
  cmd2.fargs = { 0.0f };
  b.enqueue(cmd2);

  for (int i = 0; i < 4; ++i) {
    a.handleCommands();
    b.handleCommands();
    a.processTick();
    b.processTick();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  a.finish();
  b.finish();

  std::cout << "PDInstance test finished" << std::endl;
  return 0;
}
