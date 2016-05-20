/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "UETorchPrivatePCH.h"
#include "ScriptBlueprintGeneratedClass.h"
#include "TorchContext.h"

const ANSICHAR *UTPackage = "uetorch";


FTorchContext* FTorchContext::Create(const FString& SourceCode, UObject* Owner)
{
	FTorchContext* NewContext = NULL;
#if WITH_LUA
	NewContext = new FTorchContext();
#endif
	if (NewContext)
	{
		if (NewContext->Initialize(SourceCode, Owner))
		{
			// nothing
		}
		else
		{
			delete NewContext;
			NewContext = NULL;
		}
	}
	return NewContext;
}

void FTorchContext::Tick(float DeltaTime)
{
  check(LuaState && bHasTick);
  if (bHasTick) {
    const ANSICHAR* FunctionName = "Tick";
    lua_getglobal(LuaState, FunctionName);
    lua_pushnumber(LuaState, DeltaTime);
    const int NumArgs = 1;
    const int NumResults = 0;
    if (lua_pcall(LuaState, NumArgs, NumResults, 0) != 0)
    {
      UE_LOG(LogScriptPlugin, Warning, TEXT("Cannot call Lua function %s: %s"), ANSI_TO_TCHAR(FunctionName), ANSI_TO_TCHAR(lua_tostring(LuaState, -1)));
    }
  }
}

bool FTorchContext::CallFunctionString(const FString& FunctionName, FString In, FString& Out)
{
	check(LuaState);

	bool bSuccess = FLuaUtils::DoesFunctionExist(LuaState, TCHAR_TO_ANSI(*FunctionName));
	if (bSuccess)
	{
		bSuccess = FTorchUtils::CallFunctionString(LuaState, TCHAR_TO_ANSI(*FunctionName), TCHAR_TO_ANSI(*In), Out);
	}
	else
	{
		UE_LOG(LogScriptPlugin, Warning, TEXT("Failed to call function '%s' "), *FunctionName);
	}

	return bSuccess;
}

bool FTorchUtils::CallFunctionString(lua_State* LuaState, const ANSICHAR* FunctionName, const ANSICHAR* In, FString& Out)
{
	// int topBegin = lua_gettop(LuaState);
	if (FunctionName != NULL)
	{
		lua_getglobal(LuaState, FunctionName);
	}
	lua_pushstring(LuaState, In);
	bool bResult = true;
	const int NumArgs = 1;
	const int NumResults = 1;
	if (lua_pcall(LuaState, NumArgs, NumResults, 0) != 0)
	{
		UE_LOG(LogScriptPlugin, Warning, TEXT("Cannot call Lua function %s: %s"), ANSI_TO_TCHAR(FunctionName), ANSI_TO_TCHAR(lua_tostring(LuaState, -1)));
		bResult = false;
	}
	if (!lua_isstring(LuaState, -1) && !lua_isnil(LuaState, -1)) {
		UE_LOG(LogScriptPlugin, Warning, TEXT("Lua function %s did not return a string or nil"), ANSI_TO_TCHAR(FunctionName));
	}
	if (lua_isnil(LuaState, -1)) {
	  Out = FString(TEXT("<nil>"));
	} else {
	  Out = lua_tostring(LuaState, -1);
	}
	lua_pop(LuaState, 1);
	return bResult;
}

bool FTorchContext::CallFunctionArray(const FString& FunctionName, const TArray<FString>& In, FString& Out)
{
	check(LuaState);

	bool bSuccess = FLuaUtils::DoesFunctionExist(LuaState, TCHAR_TO_ANSI(*FunctionName));
	if (bSuccess)
	{
		bSuccess = FTorchUtils::CallFunctionArray(LuaState, TCHAR_TO_ANSI(*FunctionName), In, Out);
	}
	else
	{
		UE_LOG(LogScriptPlugin, Warning, TEXT("Failed to call function '%s' "), *FunctionName);
	}

	return bSuccess;
}

bool FTorchUtils::CallFunctionArray(lua_State* LuaState, const ANSICHAR* FunctionName, const TArray<FString>& In, FString& Out)
{
	if (FunctionName != NULL)
	{
		lua_getglobal(LuaState, FunctionName);
	}
	for (auto &Str : In) {
	  lua_pushstring(LuaState, TCHAR_TO_ANSI(*Str));
	}
	bool bResult = true;
	const int NumArgs = In.Num();
	const int NumResults = 1;
	if (lua_pcall(LuaState, NumArgs, NumResults, 0) != 0)
	{
		UE_LOG(LogScriptPlugin, Warning, TEXT("Cannot call Lua function %s: %s"), ANSI_TO_TCHAR(FunctionName), ANSI_TO_TCHAR(lua_tostring(LuaState, -1)));
		bResult = false;
	}
	if (!lua_isstring(LuaState, -1) && !lua_isnil(LuaState, -1)) {
		UE_LOG(LogScriptPlugin, Warning, TEXT("Lua function %s did not return a string or nil"), ANSI_TO_TCHAR(FunctionName));
	}
	if (lua_isnil(LuaState, -1)) {
	  Out = FString(TEXT("<nil>"));
	} else {
  	  Out = lua_tostring(LuaState, -1);
	}
	lua_pop(LuaState, 1);
	return bResult;
}
