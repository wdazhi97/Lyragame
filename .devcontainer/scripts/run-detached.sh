#!/usr/bin/env bash
if [ -z "${BASH_VERSION:-}" ]; then
  exec bash "$0" "$@"
fi

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
LOG_DIR="${PROJECT_ROOT}/Saved/BuildLogs"
PID_FILE="${LOG_DIR}/detached-build.pid"
CURRENT_LOG="${LOG_DIR}/detached-build.log"

mkdir -p "${LOG_DIR}"

if [ "${1:-}" = "status" ]; then
  if [ -f "${PID_FILE}" ] && kill -0 "$(cat "${PID_FILE}")" 2>/dev/null; then
    ps -p "$(cat "${PID_FILE}")" -o pid,etime,time,pcpu,pmem,rss,stat,cmd
    echo "Log: ${CURRENT_LOG}"
  else
    echo "No detached build is running."
  fi
  exit 0
fi

command="${1:-}"
if [ -z "${command}" ]; then
  echo "Usage: $0 <prepare|build|package|editor> [extra Unreal args]" >&2
  echo "       $0 status" >&2
  exit 2
fi
shift

case "${command}" in
  prepare|build|package|editor) ;;
  *)
    echo "error: unsupported detached command: ${command}" >&2
    exit 2
    ;;
esac

if [ -f "${PID_FILE}" ] && kill -0 "$(cat "${PID_FILE}")" 2>/dev/null; then
  echo "error: detached build is already running with PID $(cat "${PID_FILE}")" >&2
  echo "Log: ${CURRENT_LOG}" >&2
  exit 1
fi

timestamp="$(date -u +%Y%m%d-%H%M%S)"
archived_log="${LOG_DIR}/${command}-${timestamp}.log"
ln -sfn "$(basename "${archived_log}")" "${CURRENT_LOG}"

nohup setsid bash "${SCRIPT_DIR}/build-linux.sh" "${command}" "$@" \
  >"${archived_log}" 2>&1 </dev/null &
pid=$!
echo "${pid}" >"${PID_FILE}"

echo "Started '${command}' as PID ${pid}."
echo "Log: ${CURRENT_LOG}"
echo "Status: bash .devcontainer/scripts/run-detached.sh status"
echo "Follow: tail -f \"${CURRENT_LOG}\""
