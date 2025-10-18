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

  // Find the instance that has a patch with the given dollarZero (returns nullptr if not found)
  PDInstance* findInstanceByPatchDollar(const std::string &dollar) const {
    for (auto &kv : instances_) {
      PDInstance* inst = kv.second.get();
      if (!inst) continue;
      // PDInstance stores patches_ keyed by dollarZero; we'll rely on pdinstance.h exposing an API for this
      // If PDInstance had a public method hasPatch(dollar), call it. Otherwise fallback: attempt to find via try/catch
      // We add a small public hasPatch method to PDInstance to support this lookup.
      if (inst->hasPatch(dollar)) return inst;
    }
    return nullptr;
  }

  // Return a list of active instance ids
  std::vector<int> ids() const {
    std::vector<int> out;
    out.reserve(instances_.size());
    for (auto &kv : instances_) out.push_back(kv.first);
    return out;
  }

private:
  std::map<int, std::unique_ptr<PDInstance>> instances_;
};

} // namespace vorpal
