-------------------------------------------------------------------------------
-- Copyright (c) 2015-present, Facebook, Inc.
-- All rights reserved.
-- This source code is licensed under the BSD-style license found in the
-- LICENSE file in the root directory of this source tree. An additional grant
-- of patent rights can be found in the PATENTS file in the same directory.
-------------------------------------------------------------------------------

local uetorch = require 'uetorch'
print("loading uetorch_example")

local M = {}

-------------------------------------------------------------------------------
-- Initalization: start the Torch REPL
-------------------------------------------------------------------------------
function M.initialize()
   -- attempt to set the frame rate to 32 fps
   -- this will only work if UnrealEngine.patch has been applied
   local succ = uetorch.SetFPS(32)

   -- If you're in a standalone game (not in the editor)
   -- this will set the resolution to 240x240
   -- (this will also make it run very fast)
   uetorch.SetResolution(240, 240)

   print("\n-- UETorch example --\n")
   print("At each tick, this script captures a small segmentation mask of all")
   print("the cubes. At each tick, if there's a cube in the vertical center line,")
   print("it moves forward; otherwise, it turns left.")
   print("The game will run at exactly 32 frames per game second,")
   print("so it may run in faster than real time.")
   print("")
end

function my_repl()
   print("Type `go(5)` to run for 5 seconds and drop back into the REPL")
   print("or type break to leave the REPL. You can uncomment `start_repl()`")
   print("in this script to get rid of the REPL altogether.")
   print("NOTE: Make sure to check 'Always load last project on startup' in the")
   print("project selector and restart, or the REPL won't work.")
   print("")
   uetorch.start_repl()
end

local cubes = {}
for i = 1, 14 do
   -- this is hardcoded for the objects in FirstPersonExampleMap,the first-person shooter demo map
   -- At least in 4.13, these have names like 'EditorCube8_2' for some reason
   cubes[i] = uetorch.GetActor(string.format('EditorCube%d_%d', i + 7, i + 1))
end

-------------------------------------------------------------------------------
-- Tick Hook: automatically move towards blocks
-------------------------------------------------------------------------------
local frame = 0
local function ExampleTickHandler(dt)
   local seg = uetorch.ObjectSegmentation(cubes, 8)
   local centerStrip = seg:select(2, math.floor(seg:size(2)/2))
   if centerStrip:sum() > 0 then
      -- if there's a block in the middle of the screen move to it
      uetorch.TapKey('Up')
   else
      -- otherwise, turn left until you find a block
      uetorch.TapKey('Left')
   end

   -- move block 7 into the sky for fun
   local loc = uetorch.GetActorLocation(cubes[7])
   uetorch.SetActorLocation(cubes[7], loc.x, loc.y, loc.z + 1)

   -- Demonstrate taking a screenshot.
   -- There are other images you can capture like optical flow;
   -- check out uetorch.lua for all of them.
   if frame == 100 then
      local screen = uetorch.Screen()
      local filename = "./uetorch_screenshot.png"
      print("Saving screenshot to " .. filename)
      require 'image'
      image.save(filename, screen)
   end
   frame = frame + 1
end

uetorch.AddTickHook(ExampleTickHandler)

-------------------------------------------------------------------------------
-- Example Lua function that can be called from blueprints using the
-- CallTorchFunctionString blueprints function.
-- https://docs.unrealengine.com/latest/INT/Engine/Blueprints/index.html
-------------------------------------------------------------------------------
function ExampleScriptFunction(x)
   return "Script Function called with x= " .. x
end

return M
