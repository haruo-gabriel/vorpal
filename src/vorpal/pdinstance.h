#pragma once

#include <libpd/PdBase.hpp>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace vorpal {

struct PdCommand {
  std::string receiver; // e.g., "$0-command"
  std::string selector; // e.g., "start" / custom symbol
  std::vector<float> fargs; // numeric args only for MVP
};

class PDInstance {
public:
  explicit PDInstance(int id = 0);
  ~PDInstance();

  // Start the instance; sample_rate defaults to 44100.
  bool start(const std::vector<std::string>& search_paths = {},
             int sample_rate = 44100,
             bool queued = true,
             int in_channels = 0,
             int out_channels = 2);
  void finish();

  // Patch management
  std::string loadPatch(const std::string& patch_name);
  void closePatch(const std::string& dollar_zero);

  // Command queue
  void enqueue(const PdCommand& cmd);
  void handleCommands();

  // Audio processing
  void processTick(int tick_ratio = 1);

  // Read bus array for a patch (vorpal-bus-<dollar_zero>)
  int readBus(const std::string& dollar_zero, std::vector<float>& out, int tick_size);

  // Convenience: write to a pd array
  bool writeArray(const std::string& arrayName, const std::vector<float>& source, int writeLen = -1, int offset = 0);

  int id() const { return id_; }
  pd::PdBase& pd() { return pd_; }

private:
  int id_ = 0;
  pd::PdBase pd_;
  int in_channels_ = 0;
  int out_channels_ = 2;
  std::vector<std::string> search_paths_;
  std::unordered_map<std::string, pd::Patch> patches_; // key = $0 string
  std::queue<PdCommand> commands_;
};

} // namespace vorpal
