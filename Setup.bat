@echo off

set DIR=%~dp0

echo === Checking out the baseline UE4 commit... ===
git checkout 4.13

echo === Patching UE4 ===
cd %DIR%\..\..\..
git branch 4.13-UETorch
git checkout 4.13-UETorch
git apply --whitespace=nowarn %DIR%\UnrealEngine_Win.patch
git add -u
git commit -m "UETorch patches"

echo === Setting up Lua... ===
rem find Lua
for /f "delims=" %%i in ('where lua') do (
  set LUA=%%i
  goto :AFTER_LUA
)
:AFTER_LUA

if "%LUA%"=="" (
  echo Couldn't find Lua. Did you forget to install torch and run torch-activate?
  goto :FAIL
)

rem check Lua version
set LUA_VERSION=%DIR%\check_lua_version.txt
call lua -v > %LUA_VERSION%
findstr "Lua 5.2" "%LUA_VERSION%" >nul
if errorlevel 1 (
  echo Expected Lua 5.2, but your Lua version is $LUA_VERSION, which is not supported.
  goto :FAIL
)

set TORCH_DIR=%LUA:\lua.cmd=%
if not exist %TORCH_DIR%\include\lua.h (
  echo Couldn't find lua.h relative to lua. Did you forget to install torch and run torch-activate?
  goto :FAIL
)

cd %DIR%\..\ScriptPlugin\Source\Lua
mklink /j install %TORCH_DIR%

goto :END

:FAIL
echo Setup fail!

:END
if exist "%LUA_VERSION%" del /q %LUA_VERSION%
