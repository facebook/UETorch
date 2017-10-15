#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if git rev-parse 4.13-UETorch > /dev/null 2>&1; then
  if [ $(git rev-parse 4.13-UETorch^1) != $(git rev-parse 4.13) ]; then   
    echo "git branch 4.13-UETorch already exists and has your commits on it. If you really want to re-run setup, you'll need to clean up and delete this branch yourself."   
    exit 1    
  fi    
  git checkout 4.13
  git branch -D 4.13-UETorch
fi    
    
echo "=== Checking out the baseline UE4 commit... ==="    
git checkout 4.13

echo "=== Patching UE4 ==="
cd $DIR/../../..
git branch 4.13-UETorch
git checkout 4.13-UETorch
git apply $DIR/UnrealEngine.patch
git add -u
git commit -m "UETorch patches"

echo "=== Setting up Lua... === "
### alternative approach: download lua-5.2.4
# cd $DIR/../ScriptPlugin/Source/Lua
# curl -R -O http://www.lua.org/ftp/lua-5.2.4.tar.gz
# tar zxf lua-5.2.4.tar.gz
# cd lua-5.2.4
# make CFLAGS='-fPIC -DLUA_USE_LINUX' linux test

# find Lua
LUA=`which lua`
echo "LUA= $LUA"
if [ $? != 0 ]; then
  echo "Couldn't find Lua. Did you forget to install torch and run torch-activate?"
  exit 1
fi

# check Lua version
LUA_VERSION=$($LUA -v | cut -c -7)
if [ "$LUA_VERSION" != "Lua 5.2" ]; then
  echo "Expected Lua 5.2, but your Lua version is $LUA_VERSION, which is not supported."
  exit 1
fi

TORCH_BIN=`dirname $LUA`
if [ ! -f "$TORCH_BIN/../include/lua.h" ]; then
  echo "Couldn't find lua.h relative to lua. Did you forget to install torch and run torch-activate?"
  exit 1
fi

cd $DIR/../ScriptPlugin/Source/Lua
ln -sfT $TORCH_BIN/.. install
