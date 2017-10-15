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

UTorchPluginComponent::UTorchPluginComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
	MainModule = TEXT("");
	Context = NULL;
}

FString UTorchPluginComponent::MakeLuaInitString() {
	FString InitStr;
	if (MainModule != "") {
		InitStr =
			"require 'uetorch';"
			"local _main = require '" + MainModule + "';"
			"if type(_main)=='table' and _main.initialize then _main.initialize() end";
	} else {
		InitStr = "require 'uetorch';";
	}

	return InitStr;
}

void UTorchPluginComponent::OnRegister()
{
	Super::OnRegister();

	if (GetWorld() && GetWorld()->WorldType != EWorldType::Editor)
	{
		FString InitStr = MakeLuaInitString();
		Context = FTorchContext::Create(InitStr, this);
		if (!Context || !Context->CanTick())
		{
			bAutoActivate = false;
			PrimaryComponentTick.bCanEverTick = false;
		}
	}
}

void UTorchPluginComponent::InitializeComponent()
{
	Super::InitializeComponent();
	// FIXME: check if source file has changed?
	if (Context)
	{
		Context->BeginPlay();
	}
}

void UTorchPluginComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Context)
	{
		Context->Tick(DeltaTime);
	}
};

void UTorchPluginComponent::OnUnregister()
{
	if (Context)
	{
		Context->Destroy();
		delete Context;
		Context = NULL;
	}

	Super::OnUnregister();
}

bool UTorchPluginComponent::CallTorchFunction(FString FunctionName)
{
	bool bSuccess = false;
	if (Context)
	{
		bSuccess = Context->CallFunction(FunctionName);
	}
	return bSuccess;
}

bool UTorchPluginComponent::CallTorchFunctionString(FString FunctionName, FString In, FString &Out)
{
	bool bSuccess = false;
	if (Context)
	{
		bSuccess = Context->CallFunctionString(FunctionName, In, Out);
	}
	return bSuccess;
}

bool UTorchPluginComponent::CallTorchFunctionArray(FString FunctionName, TArray<FString> In, FString &Out)
{
  bool bSuccess = false;
  if (Context)
  {
    bSuccess = Context->CallFunctionArray(FunctionName, In, Out);
  }
  return bSuccess;
}

