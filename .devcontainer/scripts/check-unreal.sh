#!/usr/bin/env bash
set -euo pipefail

UE_ENGINE_ROOT="${UE_ENGINE_ROOT:-/opt/unreal-engine}"
PROJECT_FILE="${UE_PROJECT_FILE:-$(pwd)/LyraStarterGame.uproject}"

RUN_UAT="${UE_ENGINE_ROOT}/Engine/Build/BatchFiles/RunUAT.sh"
BUILD_SH="${UE_ENGINE_ROOT}/Engine/Build/BatchFiles/Linux/Build.sh"
LINUX_SDK_JSON="${UE_ENGINE_ROOT}/Engine/Config/Linux/Linux_SDK.json"

fail() {
  echo "error: $*" >&2
  exit 1
}

[ -d "${UE_ENGINE_ROOT}/Engine" ] || fail "Unreal Engine was not found at ${UE_ENGINE_ROOT}"
[ -f "${PROJECT_FILE}" ] || fail "project file was not found at ${PROJECT_FILE}"
[ -f "${RUN_UAT}" ] || fail "RunUAT.sh was not found at ${RUN_UAT}"
[ -f "${BUILD_SH}" ] || fail "Linux Build.sh was not found at ${BUILD_SH}"
[ -f "${LINUX_SDK_JSON}" ] || fail "Linux SDK descriptor was not found at ${LINUX_SDK_JSON}"

echo "Unreal Engine: ${UE_ENGINE_ROOT}"
echo "Project: ${PROJECT_FILE}"

if [ ! -d "${UE_ENGINE_ROOT}/Engine/Binaries/ThirdParty/DotNet" ]; then
  echo "warning: Engine bundled DotNet was not found. Run '.devcontainer/scripts/build-linux.sh prepare' if this checkout has not been set up."
fi

if [ ! -d "${UE_ENGINE_ROOT}/Engine/Source/ThirdParty" ]; then
  echo "warning: Engine third-party source dependencies were not found. Run Setup.sh through the prepare command before building."
fi

LINUX_SDK_VERSION="$(sed -n 's/.*"MainVersion"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${LINUX_SDK_JSON}" | head -n 1)"
LINUX_SDK_ROOT="${UE_ENGINE_ROOT}/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/${LINUX_SDK_VERSION}"
if [ -n "${LINUX_SDK_VERSION}" ] && [ ! -d "${LINUX_SDK_ROOT}" ]; then
  echo "warning: Linux SDK ${LINUX_SDK_VERSION} was not found at ${LINUX_SDK_ROOT}"
  echo "warning: run '.devcontainer/scripts/build-linux.sh prepare' inside the container before building."
fi

echo "Ready to run .devcontainer/scripts/build-linux.sh"
