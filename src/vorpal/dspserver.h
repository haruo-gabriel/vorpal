
#ifndef ODA_DSPSERVER_H_
#define ODA_DSPSERVER_H_

#include <vorpal/status.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <deque>

// Forward-declare libpd types used by DSPServer static members
namespace pd { class Patch; class PdReceiver; }

namespace vorpal { class PDInstance; class InstanceManager; }

namespace vorpal {

// Forward declaration
class DSPUnit;

// Detail namespace for internal DSPServer implementation
namespace dsp_detail {
  class UnitImpl;
}

class DSPServer {
 public:
  Status start(InstanceManager& instance_manager, const std::vector<std::string>& patch_paths);
  std::shared_ptr<DSPUnit> loadUnit(const std::string &path, InstanceManager& instance_manager, int instance_id = 0);
  size_t sample_rate() const;
  int tick_size() const;
  double time_per_tick() const;
  void addPath(const std::string &path);
  void handleCommands(InstanceManager& instance_manager);
  void process(InstanceManager& instance_manager, int ticks, std::vector<float> *signal);
  void processTick(InstanceManager& instance_manager);
  void cleanUp(InstanceManager& instance_manager);
  void finish(InstanceManager& instance_manager);
 private:
  // UnitImpl is now in dsp_detail namespace and needs access to private statics
  friend class dsp_detail::UnitImpl;

  // Previously file-global state moved here as static members
  static bool started;
  static std::unique_ptr<pd::PdReceiver> receiver;
  static std::vector<std::string> search_paths;
  static std::deque<std::pair<PDInstance*, pd::Patch*>> to_be_closed__;
};

} // namespace vorpal

#endif // ODA_DSPSERVER_H_

