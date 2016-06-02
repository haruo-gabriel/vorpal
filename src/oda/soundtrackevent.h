
#ifndef LIBODA_SOUNDTRACKEVENT_H_
#define LIBODA_SOUNDTRACKEVENT_H_

#include <oda/audiounit.h>
#include <oda/dspunit.h>
#include <oda/parameter.h>
#include <oda/status.h>

#include <memory>

namespace oda {

class SoundtrackEvent {
 public:
  SoundtrackEvent();
  SoundtrackEvent(const DSPUnit &dspunit,
                  const std::shared_ptr<AudioUnit> &audiounit);
  void pushCommand(const std::string &identifier,
                   const std::vector<Parameter> &parameters);
 private:
  DSPUnit   dspunit_;
  std::shared_ptr<AudioUnit> audiounit_;
};

} // namespace oda

#endif // LIBODA_SOUNDTRACKEVENT_H_

