#include "pdinstance.h"
#include "parameter.h"
#include <cstdio>

namespace vorpal {

PDInstance::PDInstance(int id) : id_(id) {}

PDInstance::~PDInstance() { finish(); }

bool PDInstance::start(const std::vector<std::string>& search_paths, int sample_rate, bool queued,
                       int in_channels, int out_channels) {
  search_paths_ = search_paths;
  // Initialize PdBase: set channels from caller
  in_channels_ = in_channels;
  out_channels_ = out_channels;
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
  // Use ParameterSwitch to handle numbers and symbols.
  // Note: the handlers capture `this` (a raw pointer). These lambdas are invoked
  // synchronously inside `handleCommands()` while the PDInstance is alive, so
  // capturing `this` is safe here. Do NOT store these lambdas for later use or
  // call them after the PDInstance may have been destroyed; that would be UB.
    pd_.startMessage();
    ParameterSwitch sw(
      // number handler
      [this](float v) { this->pd().addFloat(v); },
      // symbol handler
      [this](const std::string &s) { this->pd().addSymbol(s); }
    );
    for (const auto &p : cmd.params) sw.handle(p);
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

bool PDInstance::writeArray(const std::string& arrayName, const std::vector<float>& source, int writeLen, int offset) {
  // delegate to PdBase::writeArray
  std::vector<float> src = source; // PdBase API expects non-const vector
  return pd_.writeArray(arrayName, src, writeLen, offset);
}

bool PDInstance::hasPatch(const std::string &dollar) const {
  return patches_.find(dollar) != patches_.end();
}

void PDInstance::registerUnit(dsp_detail::UnitImpl* unit) {
  if (unit) units_.insert(unit);
}

void PDInstance::unregisterUnit(dsp_detail::UnitImpl* unit) {
  if (unit) units_.erase(unit);
}

} // namespace vorpal
