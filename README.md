# CC Game
Based on the rhythm game Intralism. Rebuild with lots of love by Karp (for some reason in C++).

# Building
This describes what I personally did in my environment to build the project. I am using Arch Linux with paru as a package manager.

paru -S clion clion-jre clion-make
git clone https://github.com/Ludeo/cc-game.git
cd cc-game
git submodule update --init --recursive

Follow the ![official discord documentation](https://docs.discord.com/developers/discord-social-sdk/getting-started/using-c++) to get access to the game sdk download. Download the C++ zip and unpack it inside the lib folder.

Reload CMake Project in CLion
Run Debug of cc_game application
Close the Game and copy the resources folder into the cmake-build-debug folder