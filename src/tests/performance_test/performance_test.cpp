// Performance test - measure tick timing and CPU scaling
#include <vorpal/engine.h>
#include <vorpal/soundtrackevent.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <algorithm>

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

// Helper to calculate statistics
struct Stats {
    double min;
    double max;
    double mean;
    double median;
    
    static Stats calculate(std::vector<double> values) {
        if (values.empty()) return {0, 0, 0, 0};
        
        std::sort(values.begin(), values.end());
        Stats s;
        s.min = values.front();
        s.max = values.back();
        s.mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        s.median = values[values.size() / 2];
        return s;
    }
};

bool testTickPerformance(Engine& engine, int num_instances, const std::vector<std::string>& paths) {
    std::cout << "\n=== Testing " << num_instances << " instance(s) ===" << std::endl;
    
    // Create instances
    std::vector<int> instance_ids;
    for (int i = 0; i < num_instances; i++) {
        int id = engine.createInstance(paths);
        if (id < 0) {
            std::cerr << "ERROR: Failed to create instance " << i << std::endl;
            return false;
        }
        instance_ids.push_back(id);
        
        // Create an event on this instance to make it active
        std::shared_ptr<SoundtrackEvent> event;
        Status status = engine.eventInstance("performance_test", &event, id);
        if (status.ok()) {
            std::cout << "Created test event on instance " << id << std::endl;
        }
    }
    
    // Warmup
    for (int i = 0; i < 10; i++) {
        engine.tick(1.0/60.0);
    }
    
    // Measure tick performance
    const int NUM_TICKS = 100;
    std::vector<double> tick_times;
    tick_times.reserve(NUM_TICKS);
    
    PerformanceTimer timer;
    for (int i = 0; i < NUM_TICKS; i++) {
        timer.start();
        engine.tick(1.0/60.0);
        double elapsed = timer.elapsedMs();
        tick_times.push_back(elapsed);
    }
    
    // Calculate statistics
    Stats stats = Stats::calculate(tick_times);
    
    std::cout << "Tick timing statistics:" << std::endl;
    std::cout << "  Min:    " << stats.min << " ms" << std::endl;
    std::cout << "  Max:    " << stats.max << " ms" << std::endl;
    std::cout << "  Mean:   " << stats.mean << " ms" << std::endl;
    std::cout << "  Median: " << stats.median << " ms" << std::endl;
    
    // Check against performance target (5ms @ 60fps for 2 instances)
    bool passed = true;
    if (num_instances <= 2 && stats.mean > 5.0) {
        std::cerr << "PERFORMANCE VIOLATION: Mean tick time " << stats.mean 
                  << "ms exceeds 5ms target for " << num_instances << " instance(s)" << std::endl;
        passed = false;
    } else {
        std::cout << "PERFORMANCE OK: Mean tick time within acceptable range" << std::endl;
    }
    
    // Check for frame hitching (max time should not be excessive)
    double max_acceptable = stats.mean * 3.0;  // Allow 3x mean as max
    if (stats.max > max_acceptable) {
        std::cerr << "WARNING: Frame hitching detected - max time " << stats.max 
                  << "ms is " << (stats.max / stats.mean) << "x mean" << std::endl;
    }
    
    // Cleanup instances
    for (int id : instance_ids) {
        engine.destroyInstance(id);
    }
    
    return passed;
}

bool testCPUScaling(Engine& engine, const std::vector<std::string>& paths) {
    std::cout << "\n=== CPU Scaling Test ===" << std::endl;
    std::cout << "Measuring tick performance with increasing instance counts" << std::endl;
    
    std::vector<int> instance_counts = {1, 2, 4, 8};
    std::vector<double> mean_times;
    
    for (int count : instance_counts) {
        // Create instances and events
        std::vector<int> instance_ids;
        for (int i = 0; i < count; i++) {
            int id = engine.createInstance(paths);
            if (id < 0) continue;
            instance_ids.push_back(id);
            
            std::shared_ptr<SoundtrackEvent> event;
            engine.eventInstance("performance_test", &event, id);
        }
        
        // Warmup
        for (int i = 0; i < 10; i++) {
            engine.tick(1.0/60.0);
        }
        
        // Measure
        const int NUM_TICKS = 50;
        std::vector<double> tick_times;
        tick_times.reserve(NUM_TICKS);
        
        PerformanceTimer timer;
        for (int i = 0; i < NUM_TICKS; i++) {
            timer.start();
            engine.tick(1.0/60.0);
            tick_times.push_back(timer.elapsedMs());
        }
        
        Stats stats = Stats::calculate(tick_times);
        mean_times.push_back(stats.mean);
        
        std::cout << count << " instance(s): " << stats.mean << " ms average" << std::endl;
        
        // Cleanup
        for (int id : instance_ids) {
            engine.destroyInstance(id);
        }
    }
    
    // Analyze scaling
    std::cout << "\nScaling analysis:" << std::endl;
    for (size_t i = 1; i < mean_times.size(); i++) {
        double ratio = mean_times[i] / mean_times[0];
        double expected_linear = static_cast<double>(instance_counts[i]) / instance_counts[0];
        double efficiency = expected_linear / ratio;
        
        std::cout << instance_counts[i] << " vs 1 instance: " 
                  << ratio << "x slower (efficiency: " 
                  << (efficiency * 100.0) << "%)" << std::endl;
    }
    
    // Check if scaling is roughly linear (efficiency > 70%)
    bool scaling_acceptable = true;
    for (size_t i = 1; i < mean_times.size(); i++) {
        double ratio = mean_times[i] / mean_times[0];
        double expected_linear = static_cast<double>(instance_counts[i]) / instance_counts[0];
        double efficiency = expected_linear / ratio;
        
        if (efficiency < 0.7) {
            std::cerr << "WARNING: Scaling efficiency " << (efficiency * 100.0) 
                      << "% is below 70% threshold at " << instance_counts[i] 
                      << " instances" << std::endl;
            scaling_acceptable = false;
        }
    }
    
    if (scaling_acceptable) {
        std::cout << "SCALING OK: CPU scales acceptably with instance count" << std::endl;
    }
    
    return scaling_acceptable;
}

bool testInstanceManagementHitching(Engine& engine, const std::vector<std::string>& paths) {
    std::cout << "\n=== Instance Management Hitching Test ===" << std::endl;
    std::cout << "Testing for frame hitching during create/destroy operations" << std::endl;
    
    PerformanceTimer timer;
    std::vector<double> create_times;
    std::vector<double> destroy_times;
    
    const int NUM_OPERATIONS = 20;
    
    // Test instance creation timing
    std::vector<int> instance_ids;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        timer.start();
        int id = engine.createInstance(paths);
        double elapsed = timer.elapsedMs();
        create_times.push_back(elapsed);
        
        if (id >= 0) {
            instance_ids.push_back(id);
        }
    }
    
    // Test instance destruction timing
    for (int id : instance_ids) {
        timer.start();
        engine.destroyInstance(id);
        double elapsed = timer.elapsedMs();
        destroy_times.push_back(elapsed);
    }
    
    // Analyze results
    Stats create_stats = Stats::calculate(create_times);
    Stats destroy_stats = Stats::calculate(destroy_times);
    
    std::cout << "Instance creation timing:" << std::endl;
    std::cout << "  Mean: " << create_stats.mean << " ms, Max: " << create_stats.max << " ms" << std::endl;
    
    std::cout << "Instance destruction timing:" << std::endl;
    std::cout << "  Mean: " << destroy_stats.mean << " ms, Max: " << destroy_stats.max << " ms" << std::endl;
    
    // Check for hitching (max should not be excessive compared to mean)
    bool no_hitching = true;
    const double HITCH_THRESHOLD = 5.0;  // 5x mean is considered hitching
    
    if (create_stats.max > create_stats.mean * HITCH_THRESHOLD) {
        std::cerr << "WARNING: Creation hitching detected - max " << create_stats.max 
                  << "ms is " << (create_stats.max / create_stats.mean) << "x mean" << std::endl;
        no_hitching = false;
    }
    
    if (destroy_stats.max > destroy_stats.mean * HITCH_THRESHOLD) {
        std::cerr << "WARNING: Destruction hitching detected - max " << destroy_stats.max 
                  << "ms is " << (destroy_stats.max / destroy_stats.mean) << "x mean" << std::endl;
        no_hitching = false;
    }
    
    if (no_hitching) {
        std::cout << "HITCHING OK: No significant frame hitching detected" << std::endl;
    }
    
    return no_hitching;
}

int main() {
    std::cout << "=== VORPAL Performance Test ===" << std::endl;
    std::cout << "Testing tick timing, CPU scaling, and frame hitching" << std::endl;
    
    std::vector<std::string> paths = {"patches/performance_test"};
    
    // Initialize engine once (DSPServer is singleton)
    Engine engine;
    Status status = engine.start(paths);
    if (!status.ok()) {
        std::cerr << "ERROR: Failed to start engine: " << status.description() << std::endl;
        return 1;
    }
    
    bool all_passed = true;
    
    // Test 1: Single instance baseline
    all_passed = all_passed && testTickPerformance(engine, 1, paths);
    
    // Test 2: Two instances (WARP requirement)
    all_passed = all_passed && testTickPerformance(engine, 2, paths);
    
    // Test 3: CPU scaling
    all_passed = all_passed && testCPUScaling(engine, paths);
    
    // Test 4: Instance management hitching
    all_passed = all_passed && testInstanceManagementHitching(engine, paths);
    
    // Cleanup
    engine.finish();
    
    // Final result
    std::cout << "\n=== Performance Test Summary ===" << std::endl;
    if (all_passed) {
        std::cout << "ALL PERFORMANCE TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cerr << "SOME PERFORMANCE TESTS FAILED OR SHOWED WARNINGS" << std::endl;
        return 1;
    }
}
