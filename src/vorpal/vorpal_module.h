#ifndef VORPALMODULE_H
#define VORPALMODULE_H

#include <godot_cpp/classes/object.hpp>
#include <vorpal/vorpal.h>

#include <iostream>
#include <memory>
#include <vector>

namespace godot {

using std::shared_ptr;
using std::string;
using std::vector;

    class VORPALModule: public Object {
            GDCLASS(VORPALModule, Object)

        private:
            vorpal::Engine                              engine_;
            vector<shared_ptr<vorpal::SoundtrackEvent>> events_;

        protected:

            static void _bind_methods();

        public:

            bool start(const String &path);
            void finish();
            bool ok() const;
            
            // Multi-instance management
            int createInstance();
            void destroyInstance(int instance_id);
            
            // Event creation bound to specific instance (default 0 for backward compatibility)
            int eventInstance(const String &name, int instance_id = 0);
            
            void freeEvent(int id);
            void clear();
            void pushCommand(int id, const String &cmd);
            void pushCommand1f(int id, const String &cmd, float arg);
            void setEventPosition(int id, float x, float y, float z);
            void tick(double dt);
    };

}



#endif // VORPALMODULE_H
