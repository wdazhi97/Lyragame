# Lyra Linux dedicated server devcontainer

This devcontainer builds the project as a native Linux Unreal dedicated server.
It does not copy Unreal Engine into the image; it bind-mounts the local engine
checkout:

```text
<parent of this project>/UnrealEngine -> /opt/unreal-engine
```

The container is pinned to `linux/amd64` because Unreal Engine's bundled Linux
DotNet and build tools are x86_64 binaries.

On Windows, the default layout is:

```text
D:\WorkSpace\Lyragame
D:\WorkSpace\UnrealEngine
```

The mount uses `${localWorkspaceFolder}/../UnrealEngine`, so it is independent
of the Windows drive letter and user name. If your engine checkout is somewhere
else, edit `.devcontainer/devcontainer.json` and change the first entry in
`mounts`. Make sure that Docker Desktop is using Linux containers and can access
the drive containing both directories.

## Commands

Run these inside the container:

```bash
# Verify the mounted engine and required batch files.
bash .devcontainer/scripts/check-unreal.sh

# Compile the Linux dedicated server target only.
bash .devcontainer/scripts/build-linux.sh build

# Package a Linux dedicated server. Output goes to Saved/Packages/LinuxServer-Development.
bash .devcontainer/scripts/build-linux.sh package
```

Useful environment overrides:

```bash
UE_CONFIG=Shipping bash .devcontainer/scripts/build-linux.sh package
UE_MAX_PARALLEL_ACTIONS=8 bash .devcontainer/scripts/build-linux.sh build
UE_SERVER_TARGET=LyraServerSteam bash .devcontainer/scripts/build-linux.sh build
UE_ARCHIVE_DIR=/workspaces/out/LyraLinuxServer bash .devcontainer/scripts/build-linux.sh package
```

By default, UnrealBuildTool chooses compile concurrency from the CPU and memory
available to the container. Set `UE_MAX_PARALLEL_ACTIONS` when you need to cap
peak memory use. Packaging skips staging debug files and uses partial garbage
collection while cooking.

Before the first package, build the Linux cook tools and the project editor:

```bash
bash .devcontainer/scripts/run-detached.sh prepare
tail -f Saved/BuildLogs/detached-build.log
```

That downloads the Linux AutoSDK/toolchain required by
`Engine/Config/Linux/Linux_SDK.json`, then builds `ShaderCompileWorker`,
`UnrealPak`, and `LyraEditor`. The first run can take a long time.

Use the detached runner for long builds so a VS Code Remote disconnect does
not terminate them:

```bash
bash .devcontainer/scripts/run-detached.sh package
bash .devcontainer/scripts/run-detached.sh status
tail -f Saved/BuildLogs/detached-build.log
```
