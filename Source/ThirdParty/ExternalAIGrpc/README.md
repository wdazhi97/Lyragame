# ExternalAIGrpc

This module pins gRPC 1.72.0 and links it to the versions of Protobuf, Abseil,
c-ares, RE2, zlib, and OpenSSL shipped by this Unreal Engine source tree.

The generated static libraries are intentionally not committed. Build them once
on each native build host before invoking UnrealBuildTool.

## macOS arm64

```sh
UE_ENGINE_ROOT=/path/to/UnrealEngine/Engine \
Source/ThirdParty/ExternalAIGrpc/1.72.0/BuildForUE/Mac/BuildForMac.command
```

## Linux x86-64

```sh
UE_ENGINE_ROOT=/path/to/UnrealEngine/Engine \
Source/ThirdParty/ExternalAIGrpc/1.72.0/BuildForUE/Linux/BuildForLinux.sh
```

Both scripts use Unreal's CMake platform toolchain. In particular, Linux is
compiled by Unreal's Clang against the libc++ and libc++abi in the installed
Linux multi-architecture SDK. A library built by the host's default compiler
and libstdc++ is not ABI-compatible with this module.

Set `EXTERNAL_AI_GRPC_SOURCE_DIR` to reuse an existing gRPC v1.72.0 checkout,
`EXTERNAL_AI_GRPC_CACHE_DIR` to override the Linux source/object cache, or
`EXTERNAL_AI_GRPC_JOBS` to control parallel compilation. The Linux script
defaults its cache to `~/.external-ai-grpc-cache`, which avoids slow
Windows bind-mount I/O in a devcontainer.
