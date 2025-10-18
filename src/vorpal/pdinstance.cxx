#include "pdinstance.h"
#include <cstdio>

namespace vorpal {

PDInstance::PDInstance(int id) : id_(id) {}

PDInstance::~PDInstance() { finish(); }

bool PDInstance::start(const std::vector<std::string>& search_paths, int sample_rate, bool queued) {
  search_paths_ = search_paths;
  // Initialize PdBase: in/out channels: in=0, out=2 by default
  in_channels_ = 0;
  out_channels_ = 2;
  if (!pd_.init(in_channels_, out_channels_, sample_rate, queued)) {
    std::fprintf(stderr, "PDInstance::start() pd.init failed for instance %d\n", id_);
    return false;
  }
  pd_.computeAudio(true);
  for (const auto& p : search_paths_) pd_.addToSearchPath(p);
  return true;
}

void PDInstance::finish() {
  // Close patches
  for (auto& kv : patches_) {
    pd_.closePatch(kv.second);
  }
  patches_.clear();
  pd_.computeAudio(false);
  pd_.clear();
}

std::string PDInstance::loadPatch(const std::string& patch_name) {
  // Attempt to open patch by name with search paths then fallback to current dir
  pd::Patch patch;
  for (const auto& p : search_paths_) {
    patch = pd_.openPatch(patch_name, p);
    if (patch.isValid()) break;
  }
  if (!patch.isValid()) {
    patch = pd_.openPatch(patch_name, ".");
  }
  if (!patch.isValid()) {
    std::fprintf(stderr, "PDInstance::loadPatch() failed to open %s (instance %d)\n", patch_name.c_str(), id_);
    return std::string();
  }
  std::string dz = patch.dollarZeroStr();
  patches_.emplace(dz, std::move(patch));
  return dz;
}

void PDInstance::closePatch(const std::string& dollar_zero) {
  auto it = patches_.find(dollar_zero);
  if (it == patches_.end()) return;
  pd_.closePatch(it->second);
  patches_.erase(it);
}

void PDInstance::enqueue(const PdCommand& cmd) { commands_.push(cmd); }

void PDInstance::handleCommands() {
  while (!commands_.empty()) {
    const auto cmd = commands_.front();
    commands_.pop();
    pd_.startMessage();
    for (float f : cmd.fargs) pd_.addFloat(f);
    pd_.finishMessage(cmd.receiver, cmd.selector);
  }
}

void PDInstance::processTick(int tick_ratio) {
  // libpd expects valid buffers. allocate minimal buffers based on channels.
  const int ticks = tick_ratio;
  const int in_size = ticks * pd::PdBase::blockSize() * in_channels_;
  const int out_size = ticks * pd::PdBase::blockSize() * out_channels_;
  std::vector<float> inbuf(in_size > 0 ? in_size : 1);
  std::vector<float> outbuf(out_size > 0 ? out_size : 1);
  const float* inptr = in_channels_ > 0 ? inbuf.data() : nullptr;
  float* outptr = outbuf.data();
  pd_.processFloat(ticks, inptr, outptr);
}

int PDInstance::readBus(const std::string& dollar_zero, std::vector<float>& out, int tick_size) {
  out.resize(tick_size);
  const auto bus = std::string("vorpal-bus-") + dollar_zero;
  if (!pd_.readArray(bus, out, tick_size)) return 0;
  return tick_size;
}

} // namespace vorpal
