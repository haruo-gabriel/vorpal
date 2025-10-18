#pragma once

#include "pdinstance.h"
#include <map>
#include <memory>
#include <vector>

namespace vorpal {

class InstanceManager {
public:
  InstanceManager() {}
  ~InstanceManager() { instances_.clear(); }

  bool createInstance(int id, const std::vector<std::string>& paths = {}, int sample_rate = 44100) {
    if (instances_.count(id)) return false;
  std::unique_ptr<PDInstance> inst(new PDInstance(id));
  if (!inst->start(paths, sample_rate, true, 0, 2)) return false;
  instances_.emplace(id, std::move(inst));
    return true;
  }

  void destroyInstance(int id) { instances_.erase(id); }

  PDInstance* get(int id) const {
    auto it = instances_.find(id);
    return it == instances_.end() ? nullptr : it->second.get();
  }

  PDInstance* defaultInstance() const { return get(0); }

private:
  std::map<int, std::unique_ptr<PDInstance>> instances_;
};

} // namespace vorpal
