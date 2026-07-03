#!/bin/zsh -e
# Copyright Epic Games, Inc. All Rights Reserved.

SCRIPT_DIR=${0:a:h}
LIB_ROOT=${SCRIPT_DIR:h:h}
PROJECT_ROOT=${LIB_ROOT:h:h:h:h}
ENGINE_ROOT=${UE_ENGINE_ROOT:-${1}}

if [[ -z "${ENGINE_ROOT}" || ! -x "${ENGINE_ROOT}/Extras/ThirdPartyNotUE/CMake/bin/cmake" ]]; then
	echo "Usage: UE_ENGINE_ROOT=/path/to/Engine $0"
	echo "   or: $0 /path/to/Engine"
	exit 1
fi

GRPC_SOURCE_DIR="${EXTERNAL_AI_GRPC_SOURCE_DIR:-${PROJECT_ROOT}/Intermediate/ExternalAIGrpc/grpc-1.72.0}"
if [[ ! -f "${GRPC_SOURCE_DIR}/CMakeLists.txt" ]]; then
	mkdir -p "${GRPC_SOURCE_DIR:h}"
	git clone --depth 1 --filter=blob:none --branch v1.72.0 https://github.com/grpc/grpc.git "${GRPC_SOURCE_DIR}"
fi

THIRDPARTY_ROOT="${ENGINE_ROOT}/Source/ThirdParty"
PACKAGE_ROOT="${LIB_ROOT}/BuildForUE/CMakePackages"
BUILD_DIR="${PROJECT_ROOT}/Intermediate/ExternalAIGrpc/Mac-arm64"
LIB_OUTPUT_DIR="${LIB_ROOT}/lib/Mac/arm64/Release"
BIN_OUTPUT_DIR="${LIB_ROOT}/bin/Mac/arm64/Release"
CMAKE="${ENGINE_ROOT}/Extras/ThirdPartyNotUE/CMake/bin/cmake"

"${CMAKE}" -S "${LIB_ROOT}/BuildForUE" -B "${BUILD_DIR}" -G "Unix Makefiles" \
	-DCMAKE_TOOLCHAIN_FILE="${THIRDPARTY_ROOT}/CMake/PlatformScripts/Mac/Mac.cmake" \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_CXX_STANDARD=20 \
	-DCMAKE_OSX_ARCHITECTURES=arm64 \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
	-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="${LIB_OUTPUT_DIR}" \
	-DCMAKE_LIBRARY_OUTPUT_DIRECTORY="${BIN_OUTPUT_DIR}" \
	-DCMAKE_RUNTIME_OUTPUT_DIRECTORY="${BIN_OUTPUT_DIR}" \
	-DGRPC_SOURCE_DIR="${GRPC_SOURCE_DIR}" \
	-Dabsl_DIR="${THIRDPARTY_ROOT}/abseil/20240722.0/lib/Mac/Release/cmake/absl" \
	-DProtobuf_DIR="${THIRDPARTY_ROOT}/Protobuf/30.0/lib/Mac/Release/cmake/protobuf" \
	-Dc-ares_DIR="${PACKAGE_ROOT}/c-ares" \
	-DUE_CARES_LIBRARY="${THIRDPARTY_ROOT}/cares/1.19.1/lib/Mac/Release/libcares.a" \
	-DUE_CARES_INCLUDE_DIR="${THIRDPARTY_ROOT}/cares/1.19.1/include" \
	-Dre2_DIR="${PACKAGE_ROOT}/re2" \
	-DUE_RE2_LIBRARY="${THIRDPARTY_ROOT}/Re2/2022-06-01/lib/Mac/Release/libre2.a" \
	-DUE_RE2_INCLUDE_DIR="${THIRDPARTY_ROOT}/Re2/2022-06-01/include" \
	-DZLIB_LIBRARY="${THIRDPARTY_ROOT}/zlib/1.3/lib/Mac/Release/libz.a" \
	-DZLIB_INCLUDE_DIR="${THIRDPARTY_ROOT}/zlib/1.3/include" \
	-DOPENSSL_USE_STATIC_LIBS=TRUE \
	-DOPENSSL_CRYPTO_LIBRARY="${THIRDPARTY_ROOT}/OpenSSL/1.1.1t/lib/Mac/libcrypto.a" \
	-DOPENSSL_SSL_LIBRARY="${THIRDPARTY_ROOT}/OpenSSL/1.1.1t/lib/Mac/libssl.a" \
	-DOPENSSL_INCLUDE_DIR="${THIRDPARTY_ROOT}/OpenSSL/1.1.1t/include/Mac"

"${CMAKE}" --build "${BUILD_DIR}" --config Release --target ExternalAIGrpc --parallel "${EXTERNAL_AI_GRPC_JOBS:-10}"

mkdir -p "${LIB_ROOT}/include" "${LIB_ROOT}/bin/Mac"
rsync -a --delete "${GRPC_SOURCE_DIR}/include/" "${LIB_ROOT}/include/"
cp "${BIN_OUTPUT_DIR}/grpc_cpp_plugin" "${LIB_ROOT}/bin/Mac/grpc_cpp_plugin"

echo "Built ExternalAIGrpc 1.72.0 for Mac arm64."
