# Windows release bundling, exe + runtime DLLs + resources/.
# I only partly understand what anything in this file does,
# so I hope nothing breaks because I won't be able to fix it.

function(cc_target_produces_runtime_dll target out_var)
    set(${out_var} FALSE PARENT_SCOPE)
    if(NOT TARGET "${target}")
        return()
    endif()
    get_target_property(_type "${target}" TYPE)
    if(_type STREQUAL "SHARED_LIBRARY")
        set(${out_var} TRUE PARENT_SCOPE)
    endif()
endfunction()

function(cc_collect_runtime_dll_targets out_var)
    set(_targets
        SDL3-shared
        SDL3_image-shared
        SDL3_mixer-shared
        SDL3_ttf-shared
    )

    # SDL3_image / SDL3_mixer load these at runtime when *_DEPS_SHARED is ON.
    # Way too many DLLs, why is there so many DLLs?
    foreach(_candidate
        ogg
        opus
        opusfile
        vorbis
        vorbisfile
        FLAC
        gme_shared
        xmp_shared
        libmpg123
        wavpack
        png_shared
        tiff
        webp
        webpdemux
        libwebpmux
    )
        cc_target_produces_runtime_dll("${_candidate}" _is_dll)
        if(_is_dll)
            list(APPEND _targets "${_candidate}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES _targets)
    set(${out_var} "${_targets}" PARENT_SCOPE)
endfunction()

function(cc_mingw_apply_runtime_fixes)
    if(NOT MINGW)
        return()
    endif()

    set(_winpthread_libs
        "-Wl,--whole-archive"
        "${CMAKE_FIND_ROOT_PATH}/lib/libwinpthread.a"
        "-Wl,--no-whole-archive")

    foreach(_target IN LISTS ARGN)
        if(TARGET "${_target}")
            target_link_libraries("${_target}" PRIVATE ${_winpthread_libs})
        endif()
    endforeach()

    foreach(_t SDL3_ttf-shared gme_shared)
        if(TARGET "${_t}")
            target_link_libraries("${_t}" PRIVATE ${_winpthread_libs})
        endif()
    endforeach()
endfunction()

function(cc_setup_windows_bundle main_target)
    if(NOT WIN32)
        return()
    endif()

    set(_extra_exe_targets ${ARGN})

    cc_collect_runtime_dll_targets(CC_RUNTIME_DLL_TARGETS)

    set(CC_BUNDLE_DIR "${CMAKE_BINARY_DIR}/bundle/cc-game" CACHE PATH
        "Output directory for the Windows release bundle")

    install(TARGETS "${main_target}" ${_extra_exe_targets} RUNTIME DESTINATION . COMPONENT Runtime)

    foreach(_dll_target IN LISTS CC_RUNTIME_DLL_TARGETS)
        install(TARGETS "${_dll_target}" RUNTIME DESTINATION . COMPONENT Runtime)
    endforeach()

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/"
        DESTINATION resources
        COMPONENT Runtime
    )

    add_custom_target(cc_bundle
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
            --prefix "${CC_BUNDLE_DIR}"
            --component Runtime
            --config "$<CONFIG>"
        DEPENDS ${main_target} ${_extra_exe_targets} ${CC_RUNTIME_DLL_TARGETS}
        COMMENT "Bundling Windows release into ${CC_BUNDLE_DIR}"
        VERBATIM
    )

    # I don't actually have CPack configured (I think), but one day I might
    include(CPack)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_PACKAGE_NAME "cc-game")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "cc-game-${CMAKE_SYSTEM_PROCESSOR}-windows")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    set(CPACK_COMPONENTS_ALL Runtime)
endfunction()
