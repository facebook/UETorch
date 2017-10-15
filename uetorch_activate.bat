@echo off

set UE_SCRIPTS_DIR=%~dp0Scripts
set LUA_PATH=%UE_SCRIPTS_DIR%/?.lua;%UE_SCRIPTS_DIR%/?/init.lua;%LUA_PATH%
