# VORPAL Multi-Instance Tests

This directory contains all tests for VORPAL's multi-instance support implementation.

## Structure

Each test has its own isolated directory with dedicated resources:

```
src/tests/
├── pd_multi_test/              # Test 1: libpd multi-instance compilation
│   └── pd_multi_test.cpp
├── pdinstance_test/            # Test 2: PDInstance isolation (6 instances)
│   ├── pdinstance_test.cpp
│   └── patches/
│       └── pdinstance_a.pd
├── instancemanager_test/       # Test 3: InstanceManager lifecycle
│   ├── instancemanager_test.cpp
│   └── patches/
│       └── pdinstance_a.pd
├── instancemanager_autoid_test/ # Test 4: Auto-ID generation
│   ├── instancemanager_autoid_test.cpp
│   └── patches/
│       ├── pdinstance_a.pd
│       └── pdinstance_b.pd
├── engine_multiinstance_test/  # Test 5: Engine API validation
│   └── engine_multiinstance_test.cpp
└── command_routing_test/       # Test 6: Command isolation (PRIMARY)
    ├── command_routing_test.cpp
    └── patches/
        └── command_routing_test.pd
```

## Build Output

Tests are built to `build/bin/` with patches copied to `build/bin/patches/<testname>/`:

```
build/bin/
├── pd_multi_test                   # Executables
├── pdinstance_test
├── instancemanager_test
├── instancemanager_autoid_test
├── engine_multiinstance_test
├── command_routing_test
└── patches/                        # Test resources
    ├── pdinstance_test/
    ├── instancemanager_test/
    ├── instancemanager_autoid_test/
    └── command_routing_test/
```

## Running Tests

From the `build/bin` directory:

```bash
cd build/bin

# Run all tests
./pd_multi_test
./pdinstance_test
./instancemanager_test
./instancemanager_autoid_test
./engine_multiinstance_test
./command_routing_test

# Or run from build directory
cd build
make && cd bin && for test in *_test; do ./$test || break; done
```

## Test Coverage

| Test | Purpose | Status |
|------|---------|--------|
| **pd_multi_test** | Verify libpd PDINSTANCE compilation | ✅ PASS |
| **pdinstance_test** | Stress test: 6 isolated instances | ✅ PASS |
| **instancemanager_test** | Lifecycle: create/destroy/errors | ✅ PASS |
| **instancemanager_autoid_test** | Auto-ID generation | ✅ PASS |
| **engine_multiinstance_test** | Engine API & dangling pointer fix | ✅ PASS |
| **command_routing_test** | Command isolation (PRIMARY TEST) | ✅ PASS |

## Benefits of This Organization

1. **Isolation**: Each test has dedicated resources, no shared state
2. **Clarity**: Test fixtures co-located with test code
3. **Maintenance**: Easy to add/remove tests independently
4. **Clean separation**: Library patches in `vorpal/patches/`, test fixtures here
5. **Self-documenting**: Directory structure reflects test purpose

## Adding New Tests

To add a new test:

1. Create directory: `src/tests/mytest/`
2. Add test code: `src/tests/mytest/mytest.cpp`
3. (Optional) Add patches: `src/tests/mytest/patches/*.pd`
4. Update `src/tests/CMakeLists.txt`:
   ```cmake
   add_executable(mytest mytest/mytest.cpp)
   target_link_libraries(mytest PRIVATE vorpal)
   # ... (include dirs, compile features, etc.)
   ```
5. If using patches, add copy command to CMakeLists.txt

## Integration with WARP Specification

These tests validate the WARP (Multi-Instance Planning Document) acceptance criteria:

- **Layer 1** (libpd): `pd_multi_test`
- **Layer 2** (PDInstance): `pdinstance_test`
- **Layer 3** (InstanceManager): `instancemanager_test`, `instancemanager_autoid_test`
- **Layer 4** (Engine): `engine_multiinstance_test`
- **Layer 5** (Command Routing): `command_routing_test`

See: `.github/instructions/ic-vorpal.instructions.md` (WARP §12 Acceptance Criteria)
