
#include <vorpal/dspserver.h>

#include <vorpal/audiounit.h>
#include <vorpal/dspunit.h>
#include <vorpal/engine.h>
#include <vorpal/parameter.h>

#include <libpd/PdBase.hpp>
#include <libpd/PdReceiver.hpp>
#include <libpd/PdTypes.hpp>
#include <vorpal/instancemanager.h>

#include <algorithm>
#include <deque>
#include <functional>
#include <fstream>
#include <iostream>
#include <memory>
#include <tuple>

namespace vorpal {

namespace {

using pd::Patch;
using pd::PdBase;
using pd::PdReceiver;
using std::bind;
using std::deque;
using std::fstream;
using std::make_shared;
using std::mem_fn;
using std::plus;
using std::shared_ptr;
using std::string;
using std::transform;
using std::unique_ptr;
using std::unordered_set;
using std::vector;

class Receiver : public PdReceiver {
 public:
  void print(const string &message) override;
};

using Command = std::tuple<Patch*, string, vector<Parameter>>;

const int             TICK_RATIO = 1;

bool                  started = false;
unique_ptr<Receiver>  receiver;
// keep search_paths locally for patch lookup; PDInstance also stores its own paths
vector<string>        search_paths;
InstanceManager       instance_manager;

// Patch management
deque<Command>                        commands__;
deque<std::pair<PDInstance*, Patch*>> to_be_closed__;

void Receiver::print(const string &message) {
  std::printf("%s\n", message.c_str());
}

void addNumber(float number) {
  auto inst = instance_manager.defaultInstance();
  if (inst) inst->pd().addFloat(number);
}

void addSymbol(const string &symbol) {
  auto inst = instance_manager.defaultInstance();
  if (inst) inst->pd().addSymbol(symbol);
}

bool checkPath (const string &path) {
  fstream check;
  check.open(path, fstream::in);
  if (check.fail())
    return false;
  check.close();
  return true;
}

} // unnamed namespace

// nested class DSPServer::UnitImpl

class DSPServer::UnitImpl final : public DSPUnit {
 public:
  UnitImpl(Patch *patch, PDInstance* owner);
  ~UnitImpl();
  Status status() const override { return Status::OK("Valid dsp unit"); }
  void transferSignal(shared_ptr<AudioUnit> audio_unit) override;
  void pushCommand(const string &identifier,
                   const vector<Parameter> &parameters) override;
 private:
  friend class DSPServer;
  static bool popCommand(pd::Patch **patch, std::string *identifier,
                         std::vector<Parameter> *parameters);
  static std::pair<PDInstance*, pd::Patch*> to_be_closed();
  Patch                           *patch_;
  PDInstance                      *owner_;
  vector<float>                   buffer_;
  static unordered_set<UnitImpl*> units__;
};

unordered_set<DSPServer::UnitImpl*> DSPServer::UnitImpl::units__;

DSPServer::UnitImpl::UnitImpl(Patch *patch, PDInstance* owner)
  : patch_(patch), owner_(owner), buffer_(Engine::TICK_BUFFER_SIZE, 0.0f) {
  units__.insert(this);
}

DSPServer::UnitImpl::~UnitImpl() {
  to_be_closed__.emplace_back(owner_, patch_);
  units__.erase(this);
}

void DSPServer::UnitImpl::transferSignal(shared_ptr<AudioUnit> audio_unit) {
  audio_unit->stream(buffer_);
}

void DSPServer::UnitImpl::pushCommand(const string &identifier,
                                       const vector<Parameter> &parameters) {
  // If this UnitImpl has an owning instance, route the command there.
  if (owner_) {
    PdCommand cmd;
    cmd.receiver = patch_->dollarZeroStr() + std::string("-command");
    cmd.selector = identifier;
    cmd.params = parameters;
    owner_->enqueue(cmd);
    return;
  }
  // Fallback to global queue for backward compatibility
  commands__.emplace_back(patch_, identifier, parameters);
}

bool DSPServer::UnitImpl::popCommand(pd::Patch **patch, string *identifier,
                                     vector<Parameter> *parameters) {
  if (commands__.empty())
    return false;
  Command command = commands__.front();
  *patch = std::get<0>(command);
  *identifier = std::get<1>(command);
  *parameters = std::get<2>(command);
  commands__.pop_front();
  return true;
}

std::pair<PDInstance*, pd::Patch*> DSPServer::UnitImpl::to_be_closed() {
  if (to_be_closed__.empty())
    return {nullptr, nullptr};
  auto pr = to_be_closed__.front();
  to_be_closed__.pop_front();
  return pr;
}

// Enclosing class DSPServer

Status DSPServer::start(const vector<string>& patch_paths) {
  if (started)
    return Status::FAILURE("DSP Server already started");
  // create a default instance (id 0) to preserve single-instance behavior
  if (instance_manager.createInstance(0, patch_paths, sample_rate())) {
    started = true;
    search_paths.clear();
    for (const string& path : patch_paths)
      addPath(path);
    receiver.reset(new Receiver);
    auto inst = instance_manager.get(0);
    if (inst) inst->pd().setReceiver(receiver.get());
    return Status::OK("DSP Server started succesfully");
  }
  return Status::FAILURE("DSP Server could not start");
}

shared_ptr<DSPUnit> DSPServer::loadUnit(const string &path) {
  string filename = path + ".pd";
  for (string search_path : search_paths) {
    if (checkPath(search_path+"/"+filename)) {
      // open with the default instance
      PDInstance* inst = instance_manager.get(0);
      if (!inst) continue;
      Patch check = inst->pd().openPatch(filename, search_path);
      if (check.isValid()) {
        Patch *patch = new Patch(check);
          return make_shared<UnitImpl>(patch, inst);
      }
    }
  }
  return make_shared<DSPUnit::Null>();
}

size_t DSPServer::sample_rate() const {
  return 44100;
}

int DSPServer::tick_size() const {
  return PdBase::blockSize()*TICK_RATIO;
}

double DSPServer::time_per_tick() const {
  return 1.0*tick_size()/sample_rate();
}

void DSPServer::addPath(const string &path) {
  PDInstance* inst = instance_manager.get(0);
  if (inst) inst->pd().addToSearchPath(path);
  search_paths.push_back(path);
}

void DSPServer::handleCommands() {
  using namespace std::placeholders;
  Patch             *patch;
  string            identifier;
  vector<Parameter> parameters;
  ParameterSwitch   switcher(&addNumber, &addSymbol);
  while (UnitImpl::popCommand(&patch, &identifier, &parameters)) {
    PDInstance* inst = instance_manager.get(0);
    if (!inst) continue;
    inst->pd().startMessage();
    for (Parameter param : parameters)
      switcher.handle(param);
    inst->pd().finishMessage(patch->dollarZeroStr() + "-command", identifier);
  }
}

void DSPServer::process(int ticks, vector<float> *signal) {
  // Process signal
  vector<float> temp;
  signal->resize(ticks*tick_size(), 0.0f);
  for (int i = 0; i < ticks; ++i) {
    // Process global signal
    PDInstance* inst = instance_manager.get(0);
    if (inst) inst->processTick(TICK_RATIO);
    // Collect processed audio per-unit using its owning instance
    for (UnitImpl *unit : UnitImpl::units__) {
      Patch *patch = unit->patch_;
      PDInstance* owner = unit->owner_ ? unit->owner_ : inst;
      if (owner && owner->readBus(patch->dollarZeroStr(), temp, tick_size()))
        for (int k = 0; k < tick_size(); ++k)
          (*signal)[k + i*tick_size()] += temp[k];
    }
  }
}

void DSPServer::processTick() {
  PDInstance* inst = instance_manager.get(0);
  if (!inst) return;
  inst->processTick(TICK_RATIO);
  for (UnitImpl *unit : UnitImpl::units__) {
    PDInstance* owner = unit->owner_ ? unit->owner_ : inst;
    if (!owner->readBus(unit->patch_->dollarZeroStr(), unit->buffer_, tick_size()))
      ; // FIXME houston...
  }
}

void DSPServer::cleanUp() {
  while (true) {
    auto pr = UnitImpl::to_be_closed();
    if (pr.second == nullptr) break;
    PDInstance* owner = pr.first;
    Patch *patch = pr.second;
    if (patch->isValid()) {
      if (owner) owner->pd().closePatch(*patch);
    }
    delete patch;
  }
}

void DSPServer::finish() {
  cleanUp();
}

} // namespace vorpal

