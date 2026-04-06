# Contributing to OSynaptic-RX

Thank you for your interest in contributing! Please read the [OpenSynaptic CONTRIBUTING guide](../OpenSynaptic/CONTRIBUTING.md) for general guidelines.

## Quick development loop

```bash
cmake -B build -DOSRX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Coding conventions

- **C89 only** — no C99/C11 features, no `//` comments, no `<stdint.h>`, no VLAs.
- **No heap** — `malloc` / `calloc` / `realloc` / `free` are forbidden.
- **No float** — use scaled integers (`osrx_i32`).
- Every new `.c` file must have a corresponding entry in `CMakeLists.txt`
  under `add_library(osrx STATIC ...)`.
- Headers live in both `src/` (for Arduino) and `include/` (for CMake) with
  identical content.

## Running Arduino Lint locally

```bash
arduino-lint --library-manager submit --compliance strict .
```
