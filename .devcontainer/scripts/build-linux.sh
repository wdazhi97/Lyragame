#!/usr/bin/env bash
if [ -z "${BASH_VERSION:-}" ]; then
  exec bash "$0" "$@"
fi

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

UE_ENGINE_ROOT="${UE_ENGINE_ROOT:-/opt/unreal-engine}"
PROJECT_FILE="${UE_PROJECT_FILE:-${PROJECT_ROOT}/LyraStarterGame.uproject}"
PLATFORM="${UE_PLATFORM:-Linux}"
CONFIG="${UE_CONFIG:-Development}"
SERVER_TARGET="${UE_SERVER_TARGET:-LyraServer}"
ARCHIVE_DIR="${UE_ARCHIVE_DIR:-${PROJECT_ROOT}/Saved/Packages/${PLATFORM}Server-${CONFIG}}"
MAX_PARALLEL_ACTIONS="${UE_MAX_PARALLEL_ACTIONS:-}"
DEFAULT_UBT_ARGS="-NoUBA -NoDumpSyms"
if [ -n "${MAX_PARALLEL_ACTIONS}" ]; then
  DEFAULT_UBT_ARGS="${DEFAULT_UBT_ARGS} -MaxParallelActions=${MAX_PARALLEL_ACTIONS}"
fi
UBT_ARGS="${UE_UBT_ARGS:-${DEFAULT_UBT_ARGS}}"

RUN_UAT="${UE_ENGINE_ROOT}/Engine/Build/BatchFiles/RunUAT.sh"
BUILD_SH="${UE_ENGINE_ROOT}/Engine/Build/BatchFiles/Linux/Build.sh"
SETUP_SH="${UE_ENGINE_ROOT}/Setup.sh"
LINUX_SETUP_SH="${UE_ENGINE_ROOT}/Engine/Build/BatchFiles/Linux/Setup.sh"
EXTERNAL_AI_GRPC_BUILD_SH="${PROJECT_ROOT}/Source/ThirdParty/ExternalAIGrpc/1.72.0/BuildForUE/Linux/BuildForLinux.sh"
EXTERNAL_AI_GRPC_LIBRARY="${PROJECT_ROOT}/Source/ThirdParty/ExternalAIGrpc/1.72.0/lib/Unix/x86_64-unknown-linux-gnu/Release/libgrpc++.a"

usage() {
  cat <<EOF
Usage: $0 <command> [extra Unreal args]

Commands:
  check           Verify Unreal Engine and project paths.
  prepare         Set up Linux, build Linux third-party libraries, cook tools, and LyraEditor.
  build           Compile the Linux dedicated server target. Defaults: UE_SERVER_TARGET=${SERVER_TARGET}, UE_CONFIG=${CONFIG}.
  package         Build, cook, stage, pak, and archive the Linux dedicated server.
  server          Alias for build.
  editor          Compile the Linux editor target for this project.

Environment overrides:
  UE_ENGINE_ROOT  Unreal Engine root inside the container. Default: /opt/unreal-engine
  UE_PROJECT_FILE Project file path. Default: ${PROJECT_FILE}
  UE_SERVER_TARGET Server target. Default: LyraServer
  UE_CONFIG       Build configuration. Default: Development
  UE_ARCHIVE_DIR  Package output directory. Default: ${ARCHIVE_DIR}
  UE_MAX_PARALLEL_ACTIONS Limit parallel compile actions. Default: let UnrealBuildTool choose.
  UE_UBT_ARGS     Extra UnrealBuildTool args. Default: ${DEFAULT_UBT_ARGS}
EOF
}

require_file() {
  [ -f "$1" ] || {
    echo "error: required file not found: $1" >&2
    exit 1
  }
}

require_package_tools() {
  local missing=0
  local tool

  for tool in UnrealEditor UnrealPak ShaderCompileWorker; do
    if [ ! -x "${UE_ENGINE_ROOT}/Engine/Binaries/Linux/${tool}" ]; then
      echo "error: Linux package tool is missing: Engine/Binaries/Linux/${tool}" >&2
      missing=1
    fi
  done

  if [ ! -f "${PROJECT_ROOT}/Binaries/Linux/LyraEditor.target" ]; then
    echo "error: Linux LyraEditor target is missing: Binaries/Linux/LyraEditor.target" >&2
    missing=1
  fi

  if [ ! -f "${EXTERNAL_AI_GRPC_LIBRARY}" ]; then
    echo "error: Linux ExternalAIGrpc library is missing: ${EXTERNAL_AI_GRPC_LIBRARY}" >&2
    missing=1
  fi

  if [ "${missing}" -ne 0 ]; then
    echo "Run this first: bash .devcontainer/scripts/build-linux.sh prepare" >&2
    exit 1
  fi
}

run_build() {
  local target="$1"
  shift

  require_file "${BUILD_SH}"
  bash "${BUILD_SH}" "${target}" "${PLATFORM}" "${CONFIG}" "${PROJECT_FILE}" \
    -NoHotReload \
    -Progress \
    ${UBT_ARGS} \
    "$@"
}

run_engine_build() {
  local target="$1"
  shift

  require_file "${BUILD_SH}"
  bash "${BUILD_SH}" "${target}" "${PLATFORM}" "${CONFIG}" \
    -Progress \
    ${UBT_ARGS} \
    "$@"
}

command="${1:-package}"
shift || true

case "${command}" in
  check)
    bash "${SCRIPT_DIR}/check-unreal.sh"
    ;;

  prepare)
    require_file "${LINUX_SETUP_SH}"
    require_file "${EXTERNAL_AI_GRPC_BUILD_SH}"
    GIT_CONFIG_COUNT=1 \
      GIT_CONFIG_KEY_0=safe.directory \
      GIT_CONFIG_VALUE_0="${UE_ENGINE_ROOT}" \
      bash "${LINUX_SETUP_SH}"
    bash "${EXTERNAL_AI_GRPC_BUILD_SH}"
    run_engine_build ShaderCompileWorker "$@"
    run_engine_build UnrealPak "$@"
    run_build LyraEditor "$@"
    ;;

  build|server)
    run_build "${SERVER_TARGET}" "$@"
    ;;

  package)
    require_file "${RUN_UAT}"
    require_package_tools
    mkdir -p "${ARCHIVE_DIR}"
    bash "${RUN_UAT}" BuildCookRun \
      -project="${PROJECT_FILE}" \
      -noP4 \
      -server \
      -noclient \
      -serverplatform="${PLATFORM}" \
      -serverconfig="${CONFIG}" \
      -servertarget="${SERVER_TARGET}" \
      -build \
      -cook \
      -CookPartialGC \
      -stage \
      -pak \
      -archive \
      -nocompileeditor \
      -nodebuginfo \
      -unattended \
      -archivedirectory="${ARCHIVE_DIR}" \
      -ubtargs="${UBT_ARGS}" \
      -utf8output \
      "$@"
    ;;

  editor)
    run_build LyraEditor "$@"
    ;;

  -h|--help|help)
    usage
    ;;

  *)
    usage >&2
    exit 2
    ;;
esac
