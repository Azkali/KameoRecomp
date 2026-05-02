# Kameo Recomp

**KameoRecomp** is a recompilation project that ports
[Kameo: Elements of Power](https://en.wikipedia.org/wiki/Kameo:_Elements_of_Power)
to Windows and Linux on x86_64 and ARM64. It uses the static recompilation
approach provided by the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk).

The starter scaffolding is forked from
[Fable2Recomp](https://github.com/Fable2Recomp/Fable2Recomp), with all
Fable-2-specific function patches removed.

## Building

### Prerequisites

- Clang 18 or newer
- CMake 3.25+
- Ninja
- A built/installed `rexglue-sdk` (see its README)

### Game assets

Extract the Kameo ISO with [extract-xiso](https://github.com/XboxDev/extract-xiso)
so that `default.xex` and the rest of the game files end up under:

```
KameoRecomp/assets/game/
```

### Configure & build

```bash
# Generate the recompiled C++ from default.xex
rexglue codegen Kameo_config.toml

# Configure the project
cmake --preset linux-amd64-relwithdebinfo \
    -DREXSDK_DIR=/path/to/rexglue-sdk

# Build
cmake --build out/build/linux-amd64-relwithdebinfo
```

## Status

Early bring-up. Codegen runs against Kameo's `default.xex` and produces a
linkable target. Most kernel imports are stubbed in `src/hooks.cpp`; expect
to add game-specific patches as functions are exercised at runtime.
