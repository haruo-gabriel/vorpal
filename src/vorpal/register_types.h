#ifndef VORPAL_REGISTER_TYPES_H
#define VORPAL_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_vorpal_module(ModuleInitializationLevel p_level);
void uninitialize_vorpal_module(ModuleInitializationLevel p_level);

#endif // VORPAL_REGISTER_TYPES_H
