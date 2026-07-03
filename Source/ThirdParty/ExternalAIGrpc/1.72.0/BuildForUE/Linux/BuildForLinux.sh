#!/bin/bash
set -euo pipefail
# Copyright Epic Games, Inc. All Rights Reserved.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PROJECT_ROOT="$(cd "${LIB_ROOT}/../../../.." && pwd)"
ENGINE_ROOT="${UE_ENGINE_ROOT:-${1:-}}"
ARCH="x86_64-unknown-linux-gnu"

if [[ -z "${ENGINE_ROOT}" ]]; then
	echo "Usage: UE_ENGINE_ROOT=/path/to/Engine $0"
	echo "   or: $0 /path/to/Engine"
	exit 1
fi

GRPC_SOURCE_DIR="${EXTERNAL_AI_GRPC_SOURCE_DIR:-${PROJECT_ROOT}/Intermediate/ExternalAIGrpc/grpc-1.72.0}"
if [[ ! -f "${GRPC_SOURCE_DIR}/CMakeLists.txt" ]]; then
	mkdir -p "$(dirname "${GRPC_SOURCE_DIR}")"
	git clone --depth 1 --filter=blob:none --branch v1.72.0 https://github.com/grpc/grpc.git "${GRPC_SOURCE_DIR}"
fi

THIRDPARTY_ROOT="${ENGINE_ROOT}/Source/ThirdParty"
PACKAGE_ROOT="${LIB_ROOT}/BuildForUE/CMakePackages"
BUILD_DIR="${PROJECT_ROOT}/Intermediate/ExternalAIGrpc/Linux-x86_64"
LIB_OUTPUT_DIR="${LIB_ROOT}/lib/Unix/${ARCH}/Release"
BIN_OUTPUT_DIR="${LIB_ROOT}/bin/Unix/${ARCH}/Release"
CMAKE="${CMAKE:-$(command -v cmake)}"
LINUX_MULTIARCH_ROOT="${LINUX_MULTIARCH_ROOT:-$(find "${ENGINE_ROOT}/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64" -mindepth 1 -maxdepth 1 -type d | head -n 1)}"
export LINUX_MULTIARCH_ROOT

"${CMAKE}" -S "${LIB_ROOT}/BuildForUE" -B "${BUILD_DIR}" -G "Unix Makefiles" \
	-DCMAKE_TOOLCHAIN_FILE="${THIRDPARTY_ROOT}/CMake/PlatformScripts/Unix/Unix.cmake" \
	-DCMAKE_C_COMPILER_TARGET="${ARCH}" \
	-DCMAKE_CXX_COMPILER_TARGET="${ARCH}" \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_CXX_STANDARD=20 \
	-DBUILD_WITH_LIBCXX=ON \
	-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="${LIB_OUTPUT_DIR}" \
	-DCMAKE_LIBRARY_OUTPUT_DIRECTORY="${BIN_OUTPUT_DIR}" \
	-DCMAKE_RUNTIME_OUTPUT_DIRECTORY="${BIN_OUTPUT_DIR}" \
	-DGRPC_SOURCE_DIR="${GRPC_SOURCE_DIR}" \
	-Dabsl_DIR="${THIRDPARTY_ROOT}/abseil/20240722.0/lib/Unix/${ARCH}/Release/cmake/absl" \
	-DProtobuf_DIR="${THIRDPARTY_ROOT}/Protobuf/30.0/lib/Unix/${ARCH}/Release/cmake/protobuf" \
	-Dc-ares_DIR="${PACKAGE_ROOT}/c-ares" \
	-DUE_CARES_LIBRARY="${THIRDPARTY_ROOT}/cares/1.19.1/lib/Unix/${ARCH}/Release/libcares.a" \
	-DUE_CARES_INCLUDE_DIR="${THIRDPARTY_ROOT}/cares/1.19.1/include" \
	-Dre2_DIR="${PACKAGE_ROOT}/re2" \
	-DUE_RE2_LIBRARY="${THIRDPARTY_ROOT}/Re2/2022-06-01/lib/Unix/${ARCH}/Release/libre2.a" \
	-DUE_RE2_INCLUDE_DIR="${THIRDPARTY_ROOT}/Re2/2022-06-01/include" \
	-DZLIB_LIBRARY="${THIRDPARTY_ROOT}/zlib/1.3/lib/Unix/${ARCH}/Release/libz.a" \
	-DZLIB_INCLUDE_DIR="${THIRDPARTY_ROOT}/zlib/1.3/include" \
	-DOPENSSL_USE_STATIC_LIBS=TRUE \
	-DOPENSSL_CRYPTO_LIBRARY="${THIRDPARTY_ROOT}/OpenSSL/1.1.1t/lib/Unix/${ARCH}/libcrypto.a" \
	-DOPENSSL_SSL_LIBRARY="${THIRDPARTY_ROOT}/OpenSSL/1.1.1t/lib/Unix/${ARCH}/libssl.a" \
	-DOPENSSL_INCLUDE_DIR="${THIRDPARTY_ROOT}/OpenSSL/1.1.1t/include/Unix"

"${CMAKE}" --build "${BUILD_DIR}" --config Release --target ExternalAIGrpc --parallel "${EXTERNAL_AI_GRPC_JOBS:-$(nproc)}"

mkdir -p "${LIB_ROOT}/include" "${LIB_ROOT}/bin/Unix/${ARCH}"
rsync -a --delete "${GRPC_SOURCE_DIR}/include/" "${LIB_ROOT}/include/"
cp "${BIN_OUTPUT_DIR}/grpc_cpp_plugin" "${LIB_ROOT}/bin/Unix/${ARCH}/grpc_cpp_plugin"

echo "Built ExternalAIGrpc 1.72.0 for Linux ${ARCH}."
