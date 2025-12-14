#include "vorpal_module.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>


namespace godot {

    bool VORPALModule::start(const String &path) {
      return engine_.start(vector<string>(1, path.ascii().get_data())).ok();
    }

    void VORPALModule::finish() {
      engine_.finish();
    }

    bool VORPALModule::ok() const { return engine_.started(); }

    int VORPALModule::createInstance() {
        return engine_.createInstance();
    }

    void VORPALModule::destroyInstance(int instance_id) {
        engine_.destroyInstance(instance_id);
    }

    int VORPALModule::eventInstance(const String &name, int instance_id) {
        shared_ptr<vorpal::SoundtrackEvent> event;
        vorpal::Status status = engine_.eventInstance(name.ascii().get_data(), &event, instance_id);
        if (status.ok()) {
            events_.push_back(event);
            return events_.size()-1;
        }
        std::cout << "[VORPAL-wrap] Failed to instantiate event '"
                  << name.ascii().get_data() << "'" << std::endl;
        std::cout << "[VORPAL-wrap] " << status.description() << std::endl;
        return -1;
    }

    void VORPALModule::freeEvent(int id) {
        std::cout << "[VORPAL-wrap] === freeEvent() START ===" << std::endl;
        std::cout << "[VORPAL-wrap] Requested to free event ID: " << id << std::endl;
        std::cout << "[VORPAL-wrap] events_.size() = " << events_.size() << std::endl;
        
        if (id < 0 || id >= static_cast<int>(events_.size())) {
            std::cerr << "[VORPAL-wrap] Cannot free invalid event ID: " << id << std::endl;
            std::cout << "[VORPAL-wrap] === freeEvent() END (invalid ID) ===" << std::endl;
            return;
        }
        
        if (!events_[id]) {
            std::cerr << "[VORPAL-wrap] Event " << id << " already freed" << std::endl;
            std::cout << "[VORPAL-wrap] === freeEvent() END (already freed) ===" << std::endl;
            return;
        }
        
        std::cout << "[VORPAL-wrap] Event " << id << " is valid, use_count = " << events_[id].use_count() << std::endl;
        std::cout << "[VORPAL-wrap] Calling events_[" << id << "].reset()..." << std::endl;
        events_[id].reset();
        std::cout << "[VORPAL-wrap] reset() completed" << std::endl;
        std::cout << "[VORPAL-wrap] === freeEvent() END ===" << std::endl;
    }

    void VORPALModule::clear() {
      events_.clear();
    }

    void VORPALModule::pushCommand (int id, const String &cmd) {
      if (id < 0 || id >= static_cast<int>(events_.size())) {
        std::cerr << "[VORPAL-wrap] Invalid event ID: " << id << std::endl;
        return;
      }
      if (!events_[id]) {
        std::cerr << "[VORPAL-wrap] Attempt to use freed event: " << id << std::endl;
        return;
      }
      events_[id]->pushCommand(cmd.ascii().get_data());
    }

    void VORPALModule::pushCommand1f (int id, const String &cmd, float arg) {
      if (id < 0 || id >= static_cast<int>(events_.size())) {
        std::cerr << "[VORPAL-wrap] Invalid event ID: " << id << std::endl;
        return;
      }
      if (!events_[id]) {
        std::cerr << "[VORPAL-wrap] Attempt to use freed event: " << id << std::endl;
        return;
      }
      events_[id]->pushCommand(cmd.ascii().get_data(), arg);
    }

    void VORPALModule::setEventPosition (int id, float x, float y, float z) {
      if (id < 0 || id >= static_cast<int>(events_.size())) {
        std::cerr << "[VORPAL-wrap] Invalid event ID: " << id << std::endl;
        return;
      }
      if (!events_[id]) {
        std::cerr << "[VORPAL-wrap] Attempt to use freed event: " << id << std::endl;
        return;
      }
      events_[id]->setAudioSource(x, y, z);
    }

    void VORPALModule::tick (double dt) {
      static int tick_count = 0;
      if (tick_count++ < 5) {
        UtilityFunctions::print("[VORPAL-wrap] tick() called dt=", dt);
      }
      engine_.tick(dt);
    }


    void VORPALModule::_bind_methods() {
      ClassDB::bind_method(D_METHOD("ok"), &VORPALModule::ok);
      ClassDB::bind_method(D_METHOD("start"), &VORPALModule::start);
      ClassDB::bind_method(D_METHOD("finish"), &VORPALModule::finish);
      
      // Multi-instance management
      ClassDB::bind_method(D_METHOD("create_instance"), &VORPALModule::createInstance);
      ClassDB::bind_method(D_METHOD("destroy_instance", "instance_id"), &VORPALModule::destroyInstance);
      
      // Event creation with instance_id parameter (default 0 for backward compatibility)
      ClassDB::bind_method(D_METHOD("event_instance", "name", "instance_id"), 
                           &VORPALModule::eventInstance, 
                           DEFVAL(0));
      
      ClassDB::bind_method(D_METHOD("free_event"), &VORPALModule::freeEvent);
      ClassDB::bind_method(D_METHOD("clear"), &VORPALModule::clear);
      ClassDB::bind_method(D_METHOD("set_event_position"),
                                    &VORPALModule::setEventPosition);
      ClassDB::bind_method(D_METHOD("push_command"), &VORPALModule::pushCommand);
      ClassDB::bind_method(D_METHOD("push_command_1f"), &VORPALModule::pushCommand1f);
      ClassDB::bind_method(D_METHOD("tick"), &VORPALModule::tick);
    }
}
