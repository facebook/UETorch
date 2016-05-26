/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once
#include "TorchPluginComponent.generated.h"

class FTorchContext;

/**
 * UETorch script component.
 * If this component is added to an Actor, the specified script will be
 * loaded on game start, and can register functions to be executed on each
 * iteration ('tick') of the game loop.
 */
UCLASS(Blueprintable, ClassGroup = Script, hidecategories = (Activation, Collision), meta = (BlueprintSpawnableComponent))
class UETORCH_API UTorchPluginComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	/**
	* Entry-point Torch module
	* This module will be 'required' by the Lua interpreter,
	* and the initialize() method will be called on this module if it exists
	*/
	UPROPERTY(EditAnywhere, Category = "Script")
	FString MainModule;

	/**
	* Calls a script-defined function (no arguments)
	* @param FunctionName Name of the function to call
	*/
	UFUNCTION(BlueprintCallable, Category = "Script|Functions")
	virtual bool CallTorchFunction(FString FunctionName);

	/**
	* Calls a script defined function (string -> string)
	* @param FunctionName Name of the function to call
	* @param In String argument to the function
	* @param Out String output from the function
	*/
	UFUNCTION(BlueprintCallable, Category = "Script|Functions")
	virtual bool CallTorchFunctionString(FString FunctionName, FString In, FString &Out);

  /**
  * Calls a script defined function (Array<string> -> string)
  * @param FunctionName Name of the function to call
  * @param In Array of String arguments to the function
  * @param Out String output from the function
  */
  UFUNCTION(BlueprintCallable, Category = "Script|Functions")
  virtual bool CallTorchFunctionArray(FString FunctionName, TArray<FString> In, FString &Out);


	// Begin UActorComponent interface.
	virtual void OnRegister() override;
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void OnUnregister() override;
	// Begin UActorComponent interface.

protected:

	FString MakeLuaInitString();

	/** Script context (code) */
	FTorchContext* Context;
};


