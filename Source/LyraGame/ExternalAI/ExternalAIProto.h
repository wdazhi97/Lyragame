// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// Protobuf/Abseil headers contain methods named `verify`; keep Unreal's macro
// from expanding while including the generated protobuf header.
#pragma push_macro("verify")
#undef verify
#include "ExternalAI/Generated/external_ai.pb.h"
#pragma pop_macro("verify")
