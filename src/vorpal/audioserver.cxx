
#include <vorpal/audioserver.h>

#include <vorpal/audiounit.h>
#include <vorpal/engine.h>

#include <algorithm>
#include <iostream>
#include <cmath>

namespace vorpal {

namespace {

using std::make_shared;
using std::shared_ptr;
using std::transform;
using std::vector;

bool isSourcePlaying(int source) {
  int state;
  alGetSourcei(source, AL_SOURCE_STATE, &state);
  return state == AL_PLAYING;
}

} // unnamed namespace

class AudioServer::UnitImpl final : public AudioUnit {
 public:
  ~UnitImpl() { server_->freeUnit(this); }
  Status status() const override { return Status::OK("Valid audio unit"); }
  void setPosition(float x, float y, float z) override;
  void stream(const vector<float> &signal) override;
 private:
  friend class AudioServer;
  UnitImpl(AudioServer* server, size_t unit_id)
    : server_(server), unit_id_(unit_id) {}
  AudioServer *server_;
  size_t      unit_id_;
};

void AudioServer::UnitImpl::setPosition(float x, float y, float z) {
  server_->setSourcePosition(unit_id_, x, y, z);
}

void AudioServer::UnitImpl::stream(const vector<float> &signal) {
   vector<int16_t> samples(signal.size());
   transform(signal.begin(), signal.end(), samples.begin(),
            [] (float sample) -> int16_t {
              return static_cast<int16_t>(sample*32767.f);
            });
   server_->streamData(unit_id_, samples);
}

// Constructor
// Default options
AudioServer::AudioServer()
  : sources_(NUM_SOURCES), bytes_per_sample_(sizeof(int16_t)),
    sample_rate_(44100), format_(AL_FORMAT_MONO16) {
  // Setting up buffers and Sources
  // FIXME: check for errors
  alGenBuffers(NUM_BUFFERS, buffers_);
  for (unsigned i = 0; i < NUM_BUFFERS; ++i)
    free_buffers_.push(buffers_[i]);
  alGenSources(NUM_SOURCES, sources_.data());
  
  // Configure all sources with default parameters
  for (size_t i = 0; i < sources_.size(); ++i) {
    alSourcef(sources_[i], AL_GAIN, 1.0f);
    alSourcef(sources_[i], AL_PITCH, 1.0f);
    alSource3f(sources_[i], AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(sources_[i], AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcei(sources_[i], AL_LOOPING, AL_FALSE);
    alSourcei(sources_[i], AL_SOURCE_RELATIVE, AL_FALSE);
    free_sources_.push(i);
  }
  
  // Set up listener at origin facing forward
  ALfloat listenerPos[] = {0.0f, 0.0f, 0.0f};
  ALfloat listenerVel[] = {0.0f, 0.0f, 0.0f};
  ALfloat listenerOri[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
  alListenerfv(AL_POSITION, listenerPos);
  alListenerfv(AL_VELOCITY, listenerVel);
  alListenerfv(AL_ORIENTATION, listenerOri);
  std::cout << "[VORPAL AudioServer] OpenAL listener initialized" << std::endl;
  ALenum error = alGetError();
  if (error != AL_NO_ERROR) {
    std::cerr << "[VORPAL AudioServer] OpenAL initialization error: " << error << std::endl;
  }
}

// Destructor
AudioServer::~AudioServer() {
  alDeleteBuffers(NUM_BUFFERS, buffers_);
  alDeleteSources(NUM_SOURCES, sources_.data());
  sources_.clear();
}

shared_ptr<AudioUnit> AudioServer::loadUnit() {
  if (free_sources_.empty())
    return make_shared<AudioUnit::Null>();
  else {
    shared_ptr<AudioUnit> unit(new UnitImpl(this, free_sources_.front()));
    free_sources_.pop();
    return unit;
  }
}

void AudioServer::freeUnit(const UnitImpl *unit) {
  free_sources_.push(unit->unit_id_);
}

// Set Source parameters
void AudioServer::setSourcePosition(size_t source, float x, float y, float z) {
  alSource3f(sources_[source], AL_POSITION, x, y, z);
}

// Fill buffers_
void AudioServer::fillBuffer(ALuint buffer, const ALvoid *data_samples,
                        ALsizei buffer_size) {
  alBufferData(buffer, AL_FORMAT_MONO16, data_samples, buffer_size,
               sample_rate_);
}

void AudioServer::update() {
  for (ALuint source : sources_) {
    int processed;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    if (processed > 0) while (processed--) {
      ALuint buffer;
      alSourceUnqueueBuffers(source, 1, &buffer);
      free_buffers_.push(buffer);
    }
  }
}

size_t AudioServer::availableBuffers() const {
  return free_buffers_.size();
}

void AudioServer::streamData(size_t source_id, const vector<int16_t> &samples) {
  if (free_buffers_.size() > 0) {
    // Check if samples contain actual audio data
    static int log_counter = 0;
    if (log_counter++ < 5) { // Log first 5 times
      float rms = 0.0f;
      for (int16_t sample : samples) {
        float s = sample / 32767.0f;
        rms += s * s;
      }
      rms = sqrt(rms / samples.size());
      std::cout << "[VORPAL AudioServer] streamData source=" << source_id 
                << " samples=" << samples.size() << " RMS=" << rms << std::endl;
    }
    
    ALuint buffer = free_buffers_.front();
    ALuint source = sources_[source_id];
    free_buffers_.pop();
    fillBuffer(buffer, samples.data(), samples.size()*sizeof(int16_t));
    alSourceQueueBuffers(source, 1, &buffer);
    if (!isSourcePlaying(source)) {
      std::cout << "[VORPAL AudioServer] Starting playback for source " << source_id << std::endl;
      alSourcePlay(source);
      ALenum error = alGetError();
      if (error != AL_NO_ERROR) {
        std::cerr << "[VORPAL AudioServer] OpenAL error: " << error << std::endl;
      }
    }
  }
}

// Play Source
void AudioServer::playSource(int source_number) {
  alSourcePlay(sources_[source_number]);
}

void AudioServer::stopSource(int source_number) {
  alSourceStop(sources_[source_number]);
}

void AudioServer::playAllSources() {
  alSourcePlayv(NUM_SOURCES, sources_.data());
}

}
