#ifndef CC_GAME_PROFILE_H
#define CC_GAME_PROFILE_H

#include <SDL3/SDL.h>

namespace Game {

// in-game frame profiling. Enable with -DCC_ENABLE_PROFILING=ON at configure time.

#ifdef CC_ENABLE_PROFILING

class ProfileScope {
public:
    explicit ProfileScope(const char* name);
    ~ProfileScope();

    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    const char* name;
    Uint64 startNs;
};

#define CC_PROFILE(name) ::Game::ProfileScope _cc_profile_scope_##__LINE__{name}

void ProfileRecord(const char* name, Uint64 elapsedNs);
void ProfileEndFrame();
void ProfilePrintIfDue();

#else

#define CC_PROFILE(name) (void)0

inline void ProfileEndFrame() {}
inline void ProfilePrintIfDue() {}

#endif

} // namespace Game

#endif // CC_GAME_PROFILE_H
