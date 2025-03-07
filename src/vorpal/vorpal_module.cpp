#include "vorpal_module.h"

#include <godot_cpp/core/class_db.hpp>


namespace godot {

    bool VORPALModule::start(const String &path) {
      return engine_.start(vector<string>(1, path.ascii().get_data())).ok();
    }

    void VORPALModule::finish() {
      engine_.finish();
    }

    bool VORPALModule::ok() const { return engine_.started(); }

    int VORPALModule::eventInstance(const String &name) {
        shared_ptr<vorpal::SoundtrackEvent> event;
        vorpal::Status status = engine_.eventInstance(name.ascii().get_data(), &event);
        if (status.ok()) {
            events_.push_back(event);
            return events_.size()-1;
        }
        std::cout << "[VORPAL-wrap] Failed to instanciate event '"
                  << name.ascii().get_data() << "'" << std::endl;
        std::cout << "[VORPAL-wrap] " << status.description() << std::endl;
        return -1;
    }

    void VORPALModule::freeEvent(int id) {
        events_[id].reset();
    }

    void VORPALModule::clear() {
      events_.clear();
    }

    void VORPALModule::pushCommand (int id, const String &cmd) {
      events_[id]->pushCommand(cmd.ascii().get_data());
    }

    void VORPALModule::pushCommand1f (int id, const String &cmd, float arg) {
      events_[id]->pushCommand(cmd.ascii().get_data(), arg);
    }

    void VORPALModule::setEventPosition (int id, float x, float y, float z) {
      events_[id]->setAudioSource(x, y, z);
    }

    void VORPALModule::tick (double dt) {
      engine_.tick(dt);
    }


    void VORPALModule::_bind_methods() {
      ClassDB::bind_method(D_METHOD("ok"), &VORPALModule::ok);
      ClassDB::bind_method(D_METHOD("start"), &VORPALModule::start);
      ClassDB::bind_method(D_METHOD("finish"), &VORPALModule::finish);
      ClassDB::bind_method(D_METHOD("event_instance"), &VORPALModule::eventInstance);
      ClassDB::bind_method(D_METHOD("free_event"), &VORPALModule::freeEvent);
      ClassDB::bind_method(D_METHOD("clear"), &VORPALModule::clear);
      ClassDB::bind_method(D_METHOD("set_event_position"),
                                    &VORPALModule::setEventPosition);
      ClassDB::bind_method(D_METHOD("push_command"), &VORPALModule::pushCommand);
      ClassDB::bind_method(D_METHOD("push_command_1f"), &VORPALModule::pushCommand1f);
      ClassDB::bind_method(D_METHOD("tick"), &VORPALModule::tick);
    }
}
