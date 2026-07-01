cmake_minimum_required(VERSION 3.25)
project(cc_game VERSION 0.1.1)

set(CMAKE_CXX_STANDARD 23)
include(CTest)

# Remove X11 from SDL, it's not fun to build for,
# it adds nothing to the project,
# and I don't even use it so I don't want it
set(SDL_X11 OFF CACHE BOOL "" FORCE)
# Use the vendored SDL libraries
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLMIXER_VENDORED ON CACHE BOOL "" FORCE)
# Don't use AVIF, I couldn't get it to build properly
# so I guess we won't have AVIF support
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)

# SDL library directories
add_subdirectory(lib/SDL3 EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL3_ttf EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL3_image EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL3_mixer EXCLUDE_FROM_ALL)

# cc-game source files
set(GAME_SOURCES
    src/Game/Game.cpp
    src/Game/VideoSettings.h
        src/Game/objects/GameObject.cpp
        src/Game/objects/GameObject.h
        src/Game/objects/Drawable.cpp
        src/Game/objects/Drawable.h
        src/Game/objects/Sprite.cpp
        src/Game/objects/Sprite.h
        src/Game/objects/Container.cpp
        src/Game/objects/Container.h
        src/Game/objects/Label.cpp
        src/Game/objects/Label.h
        src/Game/objects/TextButton.cpp
        src/Game/objects/TextButton.h
        src/Game/objects/PanelRect.cpp
        src/Game/objects/PanelRect.h
        src/Game/ResourceManager.cpp
        src/Game/ResourceManager.h
        src/Game/SettingsStorage.cpp
        src/Game/SettingsStorage.h
        src/Game/EventManager.cpp
        src/Game/EventManager.h
        src/Game/Events/Interfaces.h
        src/Game/AudioManager.cpp
        src/Game/AudioManager.h
        src/Game/AudioBuffer.h
        src/Game/Sound.cpp
        src/Game/Sound.h
        src/Game/Song/SongManager.cpp
        src/Game/objects/ScrollContainer.cpp
        src/Game/objects/ScrollContainer.h
        src/Game/objects/Slider.cpp
        src/Game/objects/Slider.h
        src/Game/Layout/VBoxLayout.cpp
        src/Game/Layout/VBoxLayout.h
        src/Game/Layout/ILayoutHelper.h
        src/Game/Scene/IScene.h
        src/Game/Scene/SceneBase.h
        src/Game/Scene/SceneManager.cpp
        src/Game/Scene/SceneManager.h
        src/Game/Scene/Scenes/MainMenuScene.cpp
        src/Game/Scene/Scenes/MainMenuScene.h
        src/Game/Scene/Scenes/SongSelectScene.cpp
        src/Game/Scene/Scenes/SongSelectScene.h
        src/Game/Scene/Scenes/GameplayScene.cpp
        src/Game/Scene/Scenes/GameplayScene.h
        src/Game/Scene/Scenes/OptionsOverlayScene.cpp
        src/Game/Scene/Scenes/OptionsOverlayScene.h
        src/Game/Gameplay/ChartData.h
        src/Game/Gameplay/ChartLoader.cpp
        src/Game/Gameplay/ChartLoader.h
        src/Game/Gameplay/GameplayConstants.h
        src/Game/Gameplay/GameplayMath.h
        src/Game/Gameplay/GameplaySettings.h
        src/Game/Gameplay/HitScoring.h
        src/Game/Gameplay/Judgement.h
        src/Game/Gameplay/JudgementDisplayColors.h
        src/Game/Gameplay/LaneInputHandler.cpp
        src/Game/Gameplay/LaneInputHandler.h
        src/Game/Gameplay/NoteSimulation.cpp
        src/Game/Gameplay/NoteSimulation.h
        src/Game/Gameplay/RhythmField.cpp
        src/Game/Gameplay/RhythmField.h
        src/Game/Gameplay/ResultsGraphDisplay.cpp
        src/Game/Gameplay/ResultsGraphDisplay.h
        src/Game/Gameplay/ResultsGraphDeltaBarWidth.h
        src/Game/Gameplay/SongClock.cpp
        src/Game/Gameplay/SongClock.h
        src/Game/Gameplay/TimingRuler.cpp
        src/Game/Gameplay/TimingRuler.h
        src/Game/Gameplay/TimingRulerMath.h
        src/Game/Profile.cpp
        src/Game/Profile.h
)

add_executable(cc_game main.cpp ${GAME_SOURCES})

# If building for windows, make a desktop app (not console)
if(WIN32)
	set_target_properties(cc_game PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Optional profiler for the game, I am pretty sure it's shit so maybe replace it later?
option(CC_ENABLE_PROFILING "Enable in-game frame profiling scopes" OFF)
if(CC_ENABLE_PROFILING)
    target_compile_definitions(cc_game PRIVATE CC_ENABLE_PROFILING)
endif()

target_include_directories(cc_game PRIVATE src)

target_link_libraries(cc_game PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer atomic)

# Standalone C++ Map Converter
add_executable(cc_map_converter
    map-converter/main.cpp
    map-converter/miniz.c
)
target_include_directories(cc_map_converter PRIVATE src map-converter)
# ...or as standalone as it can get
target_link_libraries(cc_map_converter PRIVATE SDL3::SDL3)

# Bundler for windows, pretty sure this breaks stuff
include(cmake/BundleWindows.cmake)
cc_mingw_apply_runtime_fixes(cc_game cc_map_converter)
cc_setup_windows_bundle(cc_game cc_map_converter)

# Bunch of CTest testing stuff from this point on
add_executable(scene_manager_tests
    tests/SceneManagerTests.cpp
    src/Game/Scene/SceneManager.cpp
)
target_include_directories(scene_manager_tests PRIVATE src)
target_link_libraries(scene_manager_tests PRIVATE SDL3::SDL3)
add_test(NAME scene_manager_tests COMMAND scene_manager_tests)

add_executable(hit_scoring_tests tests/HitScoringTests.cpp)
target_include_directories(hit_scoring_tests PRIVATE src)
target_link_libraries(hit_scoring_tests PRIVATE SDL3::SDL3)
add_test(NAME hit_scoring_tests COMMAND hit_scoring_tests)

add_executable(hit_accuracy_tests tests/HitAccuracyTests.cpp)
target_include_directories(hit_accuracy_tests PRIVATE src)
target_link_libraries(hit_accuracy_tests PRIVATE SDL3::SDL3)
add_test(NAME hit_accuracy_tests COMMAND hit_accuracy_tests)

add_executable(timing_ruler_math_tests tests/TimingRulerMathTests.cpp)
target_include_directories(timing_ruler_math_tests PRIVATE src)
target_link_libraries(timing_ruler_math_tests PRIVATE SDL3::SDL3)
add_test(NAME timing_ruler_math_tests COMMAND timing_ruler_math_tests)

add_executable(results_graph_delta_bar_width_tests tests/ResultsGraphDeltaBarWidthTests.cpp)
target_include_directories(results_graph_delta_bar_width_tests PRIVATE src)
target_link_libraries(results_graph_delta_bar_width_tests PRIVATE SDL3::SDL3)
add_test(NAME results_graph_delta_bar_width_tests COMMAND results_graph_delta_bar_width_tests)

add_executable(gameplay_math_tests tests/GameplayMathTests.cpp)
target_include_directories(gameplay_math_tests PRIVATE src)
target_link_libraries(gameplay_math_tests PRIVATE SDL3::SDL3)
add_test(NAME gameplay_math_tests COMMAND gameplay_math_tests)

add_executable(chart_loader_tests
    tests/ChartLoaderTests.cpp
    src/Game/Gameplay/ChartLoader.cpp
)
target_include_directories(chart_loader_tests PRIVATE src)
target_link_libraries(chart_loader_tests PRIVATE SDL3::SDL3)
add_test(NAME chart_loader_tests COMMAND chart_loader_tests)

add_executable(note_simulation_tests
    tests/NoteSimulationTests.cpp
    src/Game/Gameplay/NoteSimulation.cpp
)
target_include_directories(note_simulation_tests PRIVATE src)
target_link_libraries(note_simulation_tests PRIVATE SDL3::SDL3)
add_test(NAME note_simulation_tests COMMAND note_simulation_tests)

add_executable(judgement_display_colors_tests tests/JudgementDisplayColorsTests.cpp)
target_include_directories(judgement_display_colors_tests PRIVATE src)
target_link_libraries(judgement_display_colors_tests PRIVATE SDL3::SDL3)
add_test(NAME judgement_display_colors_tests COMMAND judgement_display_colors_tests)

add_executable(lane_input_handler_tests
    tests/LaneInputHandlerTests.cpp
    src/Game/Gameplay/LaneInputHandler.cpp
    src/Game/objects/GameObject.cpp
)
target_include_directories(lane_input_handler_tests PRIVATE src)
target_link_libraries(lane_input_handler_tests PRIVATE SDL3::SDL3)
add_test(NAME lane_input_handler_tests COMMAND lane_input_handler_tests)

add_executable(vbox_layout_tests
    tests/VBoxLayoutTests.cpp
    src/Game/Layout/VBoxLayout.cpp
    src/Game/objects/Container.cpp
    src/Game/objects/GameObject.cpp
    src/Game/objects/Drawable.cpp
)
target_include_directories(vbox_layout_tests PRIVATE src)
target_link_libraries(vbox_layout_tests PRIVATE SDL3::SDL3)
add_test(NAME vbox_layout_tests COMMAND vbox_layout_tests)

add_executable(slider_tests
    tests/SliderTests.cpp
    src/Game/objects/Slider.cpp
    src/Game/objects/Drawable.cpp
    src/Game/objects/GameObject.cpp
    src/Game/objects/Container.cpp
    src/Game/EventManager.cpp
)
target_include_directories(slider_tests PRIVATE src)
target_link_libraries(slider_tests PRIVATE SDL3::SDL3)
add_test(NAME slider_tests COMMAND slider_tests)

