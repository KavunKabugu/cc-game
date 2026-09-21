# CC Game
Based on the rhythm game Intralism. Rebuild with lots of love by Karp (for some reason in C++).

# Building on Linux
Prepare a C++ environment, for example CLion IDE with CMake ready to use.

git clone https://github.com/KavunKabugu/cc-game.git

cd cc-game

git submodule update --init --recursive

Follow the [official discord documentation](https://docs.discord.com/developers/discord-social-sdk/getting-started/using-c++) to get access to the game sdk download. Download the C++ zip and unpack it inside the lib folder.

Reload the CMake project:

/opt/clion/bin/cmake/linux/x64/bin/cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=/opt/clion/bin/ninja/linux/x64/ninja -G Ninja -S /home/ludeo/Coding/C++/cc-game -B /home/ludeo/Coding/C++/cc-game/cmake-build-debug

then run a build with the cc_game target:

/opt/clion/bin/cmake/linux/x64/bin/cmake --build /home/ludeo/Coding/C++/cc-game/cmake-build-debug --target cc_game -j 30

The game will open but it doesn't show anything. That's because the resource folder is not yet copied on linux builds. Copy the resource folder from the project root to the cmake-build-debug folder.

After restarting the game the resources should now show.

Keep in mind that the game will feel laggy when you are on wayland. Instead you can build for windows and launch the game through proton on Steam. This will require to install the vcrun2022 package with protontricks. If you want the discord presence to work, add this to your launch commands in steam: PROTON_DISCORD_BRIDGE=1 %command%

# Building on Windows
Install the C++ Build Tools from the Microsoft website.

```
git clone https://github.com/KavunKabugu/cc-game.git

cd cc-game

git submodule update --init --recursive
```


Follow the [official discord documentation](https://docs.discord.com/developers/discord-social-sdk/getting-started/using-c++) to get access to the game sdk download. Download the C++ zip and unpack it inside the lib folder.

Currently the fluidsynth dependency from SDL mixer doesn't build on Windows. To fix that edit the file ./lib/SDL3_mixer/external/fluidsynth/CMakeLists.txt.

Change this:
```
find_library ( HAS_LIBM NAMES "m" )
if ( HAS_LIBM )
  set ( MATH_LIBRARY "m" )
endif ( HAS_LIBM )

set ( LIBFLUID_LIBS ${MATH_LIBRARY} )
```

To this:
```
find_library ( HAS_LIBM NAMES "m" )
if ( HAS_LIBM )
  set ( MATH_LIBRARY "m" )
else ()
  set ( MATH_LIBRARY "" )
endif ()

set ( LIBFLUID_LIBS ${MATH_LIBRARY} )
```

Launch the x64 Native Tools Command Prompt for VS application and navigate to the cloned repository. Then enter the following commands:

```
cmake -S . -B cc-game-build

cmake --build cc-game-build --config Release --target cc_bundle --parallel
```

The finished release will be inside of ./cc-game-build/bundle/cc-game. The discord dll doesn't get automatically copied yet so make sure to copy the discord_partner_sdk.dll from ./lib/discord_social_sdk/bin/release inside of the finished build.
