/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "IUETorch.h"
#include "CoreUObject.h"
#include "ModuleManager.h"
#include "Engine.h"
#include "TorchPluginComponent.h"
// #include "ScriptPluginComponent.h"

SCRIPTPLUGIN_API DECLARE_LOG_CATEGORY_EXTERN(LogScriptPlugin, Log, All);

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.
