
#ifndef LIBODA_AUDIOSERVER_H_
#define LIBODA_AUDIOSERVER_H_

#include <vorpal/portable.h>
#include <vorpal/status.h>

/* #include ODA_OPENAL_DIR(al.h)
#include ODA_OPENAL_DIR(alc.h) */
#include <AL/al.h>
#include <AL/alc.h>

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#define NUM_BUFFERS 1024
#define NUM_SOURCES 64

namespace vorpal {

class AudioUnit;

class AudioServer final {
 public:
  AudioServer();
  ~AudioServer();
  std::shared_ptr<AudioUnit> loadUnit();

  void playSource(int source_number);
  void stopSource(int source_number);
  void playAllSources();
  void update();
  size_t availableBuffers() const;
  void streamData (size_t source_id, const std::vector<int16_t> &samples);
  void setSourcePosition(size_t source, float x, float y, float z);

 protected:
  class UnitImpl;
  friend class UnitImpl;
  void freeUnit(const UnitImpl *unit);

 private:
  void fillBuffer(ALuint buffer, const ALvoid *dataSamples, ALsizei bufferSize);
  ALuint  buffers_[NUM_BUFFERS];
  std::queue<ALuint>  free_buffers_;
  std::vector<ALuint> sources_;
  std::queue<size_t>  free_sources_;

  const size_t bytes_per_sample_;
  const int sample_rate_;
  const ALenum format_;
};

} // namespace vorpal

#endif // LIBODA_AUDIOSERVER_H_
