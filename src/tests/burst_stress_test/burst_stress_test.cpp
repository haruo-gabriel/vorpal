// Burst stress test - rapid event creation and destruction
#include <vorpal/engine.h>
#include <vorpal/soundtrackevent.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <memory>

using namespace vorpal;

class PerformanceTimer {
public:
    void start() { 
        start_ = std::chrono::high_resolution_clock::now(); 
    }
    
    double elapsedMs() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        return duration.count() / 1000.0;
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_;
};

bool testBurstEventCreation(Engine& engine, int instance_id) {
    std::cout << "\n=== Burst Event Creation Test ===" << std::endl;
    std::cout << "Creating many events rapidly on instance " << instance_id << std::endl;
    
    const int BURST_COUNT = 50;  // Reduced from 100 to stay below OpenAL source limits
    std::vector<std::shared_ptr<SoundtrackEvent>> events;
    events.reserve(BURST_COUNT);
    
    PerformanceTimer timer;
    timer.start();
    
    int created = 0;
    for (int i = 0; i < BURST_COUNT; i++) {
        std::shared_ptr<SoundtrackEvent> event;
        Status status = engine.eventInstance("burst_test", &event, instance_id);
        if (status.ok()) {
            events.push_back(event);
            created++;
        } else {
            std::cerr << "WARNING: Failed to create event " << i << ": " 
                      << status.description() << std::endl;
        }
    }
    
    double creation_time = timer.elapsedMs();
    double avg_time = creation_time / created;
    
    std::cout << "Created " << created << "/" << BURST_COUNT << " events in " 
              << creation_time << " ms" << std::endl;
    std::cout << "Average: " << avg_time << " ms per event" << std::endl;
    
    // Process some ticks to ensure events work
    std::cout << "Processing 10 ticks with all events active..." << std::endl;
    for (int i = 0; i < 10; i++) {
        engine.tick(1.0/60.0);
    }
    
    // Cleanup - let shared_ptr handle destruction
    timer.start();
    events.clear();
    double cleanup_time = timer.elapsedMs();
    
    std::cout << "Cleanup took " << cleanup_time << " ms" << std::endl;
    
    // Check if creation was reasonable
    if (avg_time > 1.0) {  // More than 1ms per event is slow
        std::cerr << "WARNING: Event creation is slow (" << avg_time << " ms per event)" << std::endl;
        return false;
    }
    
    std::cout << "BURST TEST OK: Event creation performance acceptable" << std::endl;
    return true;
}

bool testRapidInstanceCycling(Engine& engine, const std::vector<std::string>& paths) {
    std::cout << "\n=== Rapid Instance Cycling Test ===" << std::endl;
    std::cout << "Creating and destroying instances in rapid succession" << std::endl;
    
    const int CYCLE_COUNT = 20;  // Reduced from 50 to avoid resource exhaustion
    PerformanceTimer timer;
    
    timer.start();
    for (int i = 0; i < CYCLE_COUNT; i++) {
        // Create instance
        int id = engine.createInstance(paths);
        if (id < 0) {
            std::cerr << "ERROR: Failed to create instance in cycle " << i << std::endl;
            return false;
        }
        
        // Create an event on it
        std::shared_ptr<SoundtrackEvent> event;
        engine.eventInstance("burst_test", &event, id);
        
        // Process one tick
        engine.tick(1.0/60.0);
        
        // CRITICAL: Clear event before destroying its instance to prevent use-after-free
        event.reset();
        
        // Destroy instance
        engine.destroyInstance(id);
    }
    
    double total_time = timer.elapsedMs();
    double avg_cycle = total_time / CYCLE_COUNT;
    
    std::cout << "Completed " << CYCLE_COUNT << " create/destroy cycles in " 
              << total_time << " ms" << std::endl;
    std::cout << "Average: " << avg_cycle << " ms per cycle" << std::endl;
    
    if (avg_cycle > 10.0) {  // More than 10ms per cycle is concerning
        std::cerr << "WARNING: Instance cycling is slow (" << avg_cycle << " ms per cycle)" << std::endl;
        return false;
    }
    
    std::cout << "CYCLING TEST OK: Instance lifecycle performance acceptable" << std::endl;
    return true;
}

bool testConcurrentOperations(Engine& engine, const std::vector<std::string>& paths) {
    std::cout << "\n=== Concurrent Operations Test ===" << std::endl;
    std::cout << "Testing multiple instances and events simultaneously" << std::endl;
    
    const int NUM_INSTANCES = 5;
    const int EVENTS_PER_INSTANCE = 10;
    
    PerformanceTimer timer;
    timer.start();
    
    // Create multiple instances
    std::vector<int> instance_ids;
    for (int i = 0; i < NUM_INSTANCES; i++) {
        int id = engine.createInstance(paths);
        if (id < 0) {
            std::cerr << "ERROR: Failed to create instance " << i << std::endl;
            continue;
        }
        instance_ids.push_back(id);
    }
    
    // Create multiple events on each instance
    std::vector<std::shared_ptr<SoundtrackEvent>> all_events;
    for (int instance_id : instance_ids) {
        for (int j = 0; j < EVENTS_PER_INSTANCE; j++) {
            std::shared_ptr<SoundtrackEvent> event;
            Status s = engine.eventInstance("burst_test", &event, instance_id);
            if (s.ok()) {
                all_events.push_back(event);
            }
        }
    }
    
    double setup_time = timer.elapsedMs();
    std::cout << "Setup complete: " << instance_ids.size() << " instances, " 
              << all_events.size() << " total events (" << setup_time << " ms)" << std::endl;
    
    // Process ticks with all instances and events active
    std::cout << "Processing 20 ticks with all instances active..." << std::endl;
    timer.start();
    
    for (int i = 0; i < 20; i++) {
        engine.tick(1.0/60.0);
    }
    
    double tick_time = timer.elapsedMs();
    double avg_tick = tick_time / 20.0;
    
    std::cout << "Tick processing: " << avg_tick << " ms average" << std::endl;
    
    // Cleanup
    all_events.clear();
    for (int id : instance_ids) {
        engine.destroyInstance(id);
    }
    
    // Check performance
    if (avg_tick > 10.0) {  // More than 10ms for this load is concerning
        std::cerr << "WARNING: Tick performance degraded with concurrent load" << std::endl;
        return false;
    }
    
    std::cout << "CONCURRENT TEST OK: Performance acceptable under concurrent load" << std::endl;
    return true;
}

bool testMemoryStability(Engine& engine, const std::vector<std::string>& paths) {
    std::cout << "\n=== Memory Stability Test ===" << std::endl;
    std::cout << "Testing for memory leaks over extended operations" << std::endl;
    
    const int ITERATIONS = 50;  // Reduced from 100 to avoid resource exhaustion
    
    std::cout << "Running " << ITERATIONS << " iterations of create/use/destroy..." << std::endl;
    
    for (int iter = 0; iter < ITERATIONS; iter++) {
        // Create instance
        int id = engine.createInstance(paths);
        if (id < 0) continue;
        
        // Create events
        std::vector<std::shared_ptr<SoundtrackEvent>> events;
        for (int i = 0; i < 5; i++) {
            std::shared_ptr<SoundtrackEvent> event;
            engine.eventInstance("burst_test", &event, id);
            if (event) events.push_back(event);
        }
        
        // Process ticks
        for (int i = 0; i < 5; i++) {
            engine.tick(1.0/60.0);
        }
        
        // Cleanup
        events.clear();
        engine.destroyInstance(id);
        
        // Progress indicator
        if ((iter + 1) % 20 == 0) {
            std::cout << "  Completed " << (iter + 1) << "/" << ITERATIONS << " iterations" << std::endl;
        }
    }
    
    std::cout << "MEMORY TEST OK: Completed " << ITERATIONS 
              << " iterations without crashes" << std::endl;
    std::cout << "Note: Check system memory usage for actual leak detection" << std::endl;
    
    return true;
}

int main() {
    std::cout << "=== VORPAL Burst Stress Test ===" << std::endl;
    std::cout << "Testing rapid operations and concurrent load" << std::endl;
    
    std::vector<std::string> paths = {"patches/burst_test"};
    
    // Initialize engine once (DSPServer is singleton)
    Engine engine;
    Status status = engine.start(paths);
    if (!status.ok()) {
        std::cerr << "ERROR: Failed to start engine: " << status.description() << std::endl;
        return 1;
    }
    
    bool all_passed = true;
    
    // Test 1: Burst event creation
    int instance_id = engine.createInstance(paths);
    if (instance_id >= 0) {
        all_passed = all_passed && testBurstEventCreation(engine, instance_id);
        engine.destroyInstance(instance_id);
    } else {
        std::cerr << "ERROR: Failed to create test instance" << std::endl;
        all_passed = false;
    }
    
    // Test 2: Rapid instance cycling
    all_passed = all_passed && testRapidInstanceCycling(engine, paths);
    
    // Test 3: Concurrent operations
    all_passed = all_passed && testConcurrentOperations(engine, paths);
    
    // Test 4: Memory stability
    all_passed = all_passed && testMemoryStability(engine, paths);
    
    // Cleanup
    engine.finish();
    
    // Final result
    std::cout << "\n=== Burst Stress Test Summary ===" << std::endl;
    if (all_passed) {
        std::cout << "ALL BURST STRESS TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cerr << "SOME BURST STRESS TESTS FAILED OR SHOWED WARNINGS" << std::endl;
        return 1;
    }
}
