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
#include <string>

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
			NewContext->RegisterUETorchLibrary();
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

luaL_Reg UETorchLib[] =
{
	{"GetActorPhysicsHandleComponent", FTorchLibrary::GetActorPhysicsHandleComponent},
	{"GetActorMeshComponentAsPrimitive", FTorchLibrary::GetActorMeshComponentAsPrimitive},
	{"LineTraceMeshComponent", FTorchLibrary::LineTraceMeshComponent},
	{ NULL, NULL }
};

void FTorchContext::RegisterUETorchLibrary(){
	lua_getglobal(LuaState, "UETorch");
	if (lua_isnil(LuaState, -1))
	{
		lua_pop(LuaState, 1);
		lua_newtable(LuaState);
	}
	luaL_setfuncs(LuaState, UETorchLib, 0);
	lua_setglobal(LuaState, "UETorch");
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

int FTorchLibrary::GetActorPhysicsHandleComponent(lua_State* LuaState)
{
	AActor* object = (AActor*)lua_touserdata(LuaState, 1);
	if(object == NULL) {
		printf("Object is null\n");
		return 0;
	}
	UPhysicsHandleComponent* component = object->FindComponentByClass<UPhysicsHandleComponent>();
	if(component == NULL) {
		printf("Object doesn't have a UPhysicsHandleComponent\n");
		return 0;
	}
	lua_pushlightuserdata(LuaState, component);
	return 1;
}

int FTorchLibrary::GetActorMeshComponentAsPrimitive(lua_State* LuaState)
{
	AActor* object = (AActor*)lua_touserdata(LuaState, 1);
	if(object == NULL) {
		printf("Object is null\n");
		return 0;
	}
	UStaticMeshComponent* meshComponent = object->FindComponentByClass<UStaticMeshComponent>();
	if(meshComponent == NULL) {
		printf("Object doesn't have an UStaticMeshComponent\n");
		return 0;
	}
	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(meshComponent);
	lua_pushlightuserdata(LuaState, PrimitiveComponent);
	return 1;
}

int FTorchLibrary::LineTraceMeshComponent(lua_State* LuaState)
{
	AActor* object = (AActor*)lua_touserdata(LuaState, 1);
	if(object == NULL) {
		printf("Object is null\n");
		return 0;
	}
	UStaticMeshComponent* meshComponent = object->FindComponentByClass<UStaticMeshComponent>();
	if(meshComponent == NULL) {
		printf("Object doesn't have an UStaticMeshComponent\n");
		return 0;
	}
	FCollisionQueryParams TraceParams(FName(TEXT("UETorch LineTraceMeshComponent")), true, nullptr);
	float startx = lua_tonumber(LuaState, 2);
	float starty = lua_tonumber(LuaState, 3);
	float startz = lua_tonumber(LuaState, 4);
	FVector Start(startx, starty, startz);
	float endx = lua_tonumber(LuaState, 5);
	float endy = lua_tonumber(LuaState, 6);
	float endz = lua_tonumber(LuaState, 7);
	FVector End(endx, endy, endz);
	FHitResult OutHit(ForceInit);
	bool isHit = meshComponent->LineTraceComponent(OutHit, Start, End, TraceParams);
	if(!isHit) return 0;
	lua_newtable(LuaState);
	int index = lua_gettop(LuaState);
	lua_pushlstring(LuaState, "x", 1);
	lua_pushnumber(LuaState, OutHit.Location.X);
	lua_settable(LuaState, index);
	lua_pushlstring(LuaState, "y", 1);
	lua_pushnumber(LuaState, OutHit.Location.Y);
	lua_settable(LuaState, index);
	lua_pushlstring(LuaState, "z", 1);
	lua_pushnumber(LuaState, OutHit.Location.Z);
	lua_settable(LuaState, index);
	lua_pushlstring(LuaState, "BoneName", 8);
	lua_pushlightuserdata(LuaState, &(OutHit.BoneName));
	lua_settable(LuaState, index);
	return 1;
}