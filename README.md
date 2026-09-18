# CC Game
Based on the rhythm game Intralism. Rebuild with lots of love by Karp (for some reason in C++).

# Building on Linux
Prepare a C++ environment, for example CLion IDE with CMake ready to use.

git clone https://github.com/KavunKabugu/cc-game.git

cd cc-game

git submodule update --init --recursive

Follow the [official discord documentation](https://docs.discord.com/developers/discord-social-sdk/getting-started/using-c++) to get access to the game sdk download. Download the C++ zip and unpack it inside the lib folder.

Reload the CMake project, then run a build with the cc_game target.

The game will open but it doesn't show anything. That's because the resource folder is not yet copied on linux builds. Copy the resource folder from the project root to the cmake-build-debug folder.

After restarting the game the resources should now show.