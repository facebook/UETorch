/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#ifndef WITH_LUA
#error "Lua was not installed."
#endif

#include "LuaIntegration.h"

class FTorchContext : public FLuaContext
{
protected:

public:
	static FTorchContext* Create(const FString& SourceCode, UObject* Owner);

  void Tick(float DeltaTime);
	bool CallFunctionString(const FString& FunctionName, FString In, FString& Out);
	bool CallFunctionArray(const FString& FunctionName, const TArray<FString>& In, FString& Out);
};

struct FTorchUtils {
	static bool CallFunctionString(lua_State* LuaState, const ANSICHAR* FunctionName, const ANSICHAR* In, FString& Out);
	static bool CallFunctionArray(lua_State* LuaState, const ANSICHAR* FunctionName, const TArray<FString>& In, FString& Out);
};
