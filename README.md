
# UETorch
UETorch is an [Unreal Engine 4](http://www.unrealengine.com) plugin that adds support for embedded [Lua/Torch](http://torch.ch/) scripts in the game engine loop, and a set of Lua APIs for providing user input, taking screenshots and segmentation masks, controlling game state, running faster than real time, etc. Torch is an AI Research platform that is focused on deep learning.
UETorch strongly leverages the sparsely documented [ScriptPlugin](https://forums.unrealengine.com/showthread.php?3958-Scripting-Language-extensions-via-plugins) plugin provided with Unreal Engine 4.

Some recent research done using the UETorch platform is detailed in this paper ["Learning Physical Intuition of Block Towers by Example"](http://arxiv.org/abs/1603.01312) where we explore the ability of deep feed-forward models to learn intuitive physics.

## Requirements
See the [Unreal Engine 4 Requirements](https://docs.unrealengine.com/latest/INT/GettingStarted/RecommendedSpecifications/).
UETorch was developed for Linux; see [Building Unreal Engine on Linux](https://wiki.unrealengine.com/Building_On_Linux#Prerequisites). Running on Mac and Windows is not currently supported, but should be relatively straightforward if you're willing to figure out the build process.


## Installing UETorch
1. Download and install torch from https://github.com/torch/torch-distro. When it's time to run install.sh or .bat, set the Lua version to 5.2:

 **Linux**
 ```
 TORCH_LUA_VERSION=LUA52 ./install.sh
 ```
 **Windows**
 ```
 set TORCH_LUA_VERSION=LUA52
 ./install.bat
 ```

2. Set up an Epic Games account at https://github.com/EpicGames/Signup/, needed to clone the Unreal Engine repository from github. UETorch currently only works with the source distribution of UE4, not the binary download.  
3. Install all the Linux prerequisites mentioned at https://wiki.unrealengine.com/Building_On_Linux. In the case of Windows, skip this step.
 ```
 sudo apt-get install build-essential mono-gmcs mono-xbuild mono-dmcs libmono-corlib4.0-cil \
 libmono-system-data-datasetextensions4.0-cil libmono-system-web-extensions4.0-cil \
 libmono-system-management4.0-cil libmono-system-xml-linq4.0-cil cmake dos2unix clang xdg-user-dirs
 ```
4. Install UnrealEngine / UETorch

 **Linux**
 ```bash
 git clone https://github.com/EpicGames/UnrealEngine.git
 cd UnrealEngine
 
 # clone UETorch into the plugins directory
 git clone https://github.com/facebook/UETorch.git Engine/Plugins/UETorch
 # run the UETorch setup script
 # this will update you to a specific revision on UnrealEngine-4.8, add some patches, and set up the Lua paths
 Engine/Plugins/UETorch/Setup.sh
 
 # grab some coffee, this will take a long time
 ./Setup.sh && ./GenerateProjectFiles.sh && make
 ```
 **Windows**
 ```bat
 git clone https://github.com/EpicGames/UnrealEngine.git
 cd UnrealEngine
 
 rem clone UETorch into the plugins directory
 git clone https://github.com/facebook/UETorch.git Engine/Plugins/UETorch
 rem run the UETorch setup script
 rem this will update you to a specific revision on UnrealEngine-4.13, add some patches, and set up the Lua paths
 Engine/Plugins/UETorch/Setup.bat
 
 rem grab some coffee, this will take a long time
 ./Setup.bat && ./GenerateProjectFiles.bat
 ```
 Since UE4.sln has been generated, build it with Visual Studio.

5. Profit!

## Getting Started with UETorch

1. Source the `uetorch_activate.sh` script. You might want to add this to your `.bashrc`.

 **Linux**
 ```bash
 source Engine/Plugins/UETorch/uetorch_activate.sh
 ```
 **Windows**
 ```bat
 call Engine/Plugins/UETorch/uetorch_activate.bat
 ```

2. Launch Unreal Editor 

 **Linux**
 ```bash
 cd Engine/Binaries/Linux
 ./UE4Editor
 ```
 **Windows**
 ```bat
 cd Engine/Binaries/Win64
 ./UE4Editor
 ```

3. Create a new 'First Person' project. 
 ![Create a 'First Person' Project](Resources/Screenshots/ut_setup.png)

4. Lets add a TorchPlugin component to the player. In the 'World Outliner' panel, select 'FirstPersonCharacter', and then in the 'Details' panel, click 'Add Component' and select 'Torch Plugin' (you can use the typeahead).
 ![Add a Torch Plugin Component to FirstPersonCharacter](Resources/Screenshots/fpc_add.png)

5. Now we just need to tell the UETorch component which script to run. We'll use the example script in [UETorch/Scripts/uetorch\_example.lua](Scripts/uetorch_example.lua). Just set the 'Main Module' field on the TorchPlugin to 'uetorch\_example'. (If you sourced uetorch\_activate.sh, then UETorch/Scripts should already be on your LUA_PATH).
 ![Set the 'Main Module' field to uetorch\_example](Resources/Screenshots/torchplugin_module.png)

6. Press the 'Play' button. The player should move towards the cubes, based on the simple tick function inside [uetorch\_example.lua](Scripts/uetorch_example.lua). Go take a look at that script now. You can exit the game by pressing the 'Esc' key. This script demos several different UETorch features: 
  * locates the blocks via a segmentation mask, and moves the character towards them by simulating keyboard input.
  * takes a screenshot after 100 frames and saves it to `./uetorch_screenshot.jpg`.
  * moves one of the blocks into the air via the `uetorch.SetActorLocation` function.
7. You can call a Lua function from inside Unreal Engine's [Blueprints scripting language](https://docs.unrealengine.com/latest/INT/Engine/Blueprints/index.html). We'll add a routine to the 'level blueprint' that calls into Lua and starts the REPL when you press the 'H' key, which will allow you to run the Lua interpreter interactively inside a game. Open the level blueprint (from the Blueprints menu). Right click in the main window to add a new widget, uncheck 'Context Sensitive', and search for 'Call TorchFunction'. This widget just calls a Lua function with no input or output (only void -> void and string -> string widgets are provided; you can write your own as well). You can then drag your FirstPersonCharacter into the blueprint and hook it up as the target to the widget. Read the [Blueprints documentation](https://docs.unrealengine.com/latest/INT/Engine/Blueprints/index.html) for more details. Here's what the final blueprint should look like
 ![The final blueprint](Resources/Screenshots/torch_bp.png)
8. The interactive Torch REPL won't work inside this editor process because it is a child process with no attached TTY. In the main Editor window, go to File->Open Project, check 'Always load last project on startup', and then close the window. Then restart UE4Editor, and it should directly load your Project.
9. Now press 'Play' again, and press the 'H' key. The game should freeze and you will enter the Torch REPL inside of your terminal.

## Full documentation
To learn how to develop Unreal Engine projects, see the Unreal Engine documentation at https://docs.unrealengine.com/latest/INT/.

In-line documentation for the APIs provided by UETorch can be found in [uetorch.lua](Scripts/uetorch.lua).

More coming soon.

## Join the UETorch community
See the CONTRIBUTING file for how to help out.

## License
UETorch is BSD-licensed. We also provide an additional patent grant.
