
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

namespace vorpal { class PDInstance; }

namespace vorpal {

// Forwatd declaration
class DSPUnit;

class DSPServer {
 public:
  Status start(const std::vector<std::string>& patch_paths);
  std::shared_ptr<DSPUnit> loadUnit(const std::string &path);
  size_t sample_rate() const;
  int tick_size() const;
  double time_per_tick() const;
  void addPath(const std::string &path);
  void handleCommands();
  void process(int ticks, std::vector<float> *signal);
  void processTick();
  void cleanUp();
  void finish();
 private:
  class UnitImpl;
  static std::unordered_set<UnitImpl*> units__;

  // Previously file-global state moved here as static members
  static bool started;
  static std::unique_ptr<pd::PdReceiver> receiver;
  static std::vector<std::string> search_paths;
  static class InstanceManager instance_manager;
  static std::deque<std::pair<PDInstance*, pd::Patch*>> to_be_closed__;
};

} // namespace vorpal

#endif // ODA_DSPSERVER_H_

