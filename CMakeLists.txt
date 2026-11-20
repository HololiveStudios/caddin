cmake_minimum_required(VERSION 3.20)
project(caddin C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
    set(PLATFORM_SRCS
        src/platform/window_win32.c
        src/platform/x11_cursor.c
    )
    set(PLATFORM_LIBS
        user32
        gdi32
        msimg32
        winmm
        m
    )
else()
    find_package(X11 REQUIRED)
    # Detecta ALSA no Linux
    find_library(ALSA_LIB asound)
    set(PLATFORM_SRCS
        src/platform/window_x11.c
        src/platform/x11_cursor.c
    )
    set(PLATFORM_LIBS
        ${X11_LIBRARIES}
        ${X11_Xext_LIB}
        m
        pthread
    )
    if(ALSA_LIB)
        list(APPEND PLATFORM_LIBS ${ALSA_LIB})
        add_compile_definitions(HAVE_ALSA)
        message(STATUS "[AUDIO] ALSA encontrada: ${ALSA_LIB}")
    else()
        message(WARNING "[AUDIO] ALSA não encontrada — áudio silencioso.")
    endif()
endif()

set(CORE_SRCS
    src/core/arena.c
    src/core/rng.c
    src/core/snn.c
    src/core/neuromod.c
    src/core/cpg.c
    src/core/predict.c
    src/core/persist.c
    src/sense/sense_mouse.c
    src/sense/sense_scroll.c
    src/sense/sense_windows.c
    src/sense/sense_sysload.c
    src/sense/sense_luma.c
    src/sense/sense_heatmap.c
    src/sense/sense_mic.c
    src/sense/sense_music.c
    src/sense/sense_text.c
    src/body/palette.c
    src/body/voice_synth.c
    src/body/audio_out.c
    src/mind/workspace.c
    src/lang/lexicon.c
    src/behavior/behavior_dmn.c
    src/behavior/behavior_bond.c
    src/behavior/behavior_explore.c
    src/behavior/behavior_hide_hunt.c
    src/behavior/behavior_perch.c
    src/behavior/behavior_startle_relief.c
    src/behavior/behavior_dance.c
)

set(SRCS
    src/main.c
    ${PLATFORM_SRCS}
    ${CORE_SRCS}
    src/renderer/pixel_draw.c
    src/renderer/face.c
    src/animation/animation.c
    src/animation/softbody.c
    src/animation/emotions.c
    src/animation/autonomy.c
    neuro.c
    behavior.c
    homeostasis.c
    conditioning.c
    memory.c
    sensors.c
)

add_executable(caddin ${SRCS})

target_include_directories(caddin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sense
    ${CMAKE_CURRENT_SOURCE_DIR}/src/body
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mind
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lang
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/behavior
)

if(NOT WIN32)
    target_include_directories(caddin PRIVATE ${X11_INCLUDE_DIR})
endif()

target_link_libraries(caddin PRIVATE ${PLATFORM_LIBS})

# ── Testes Headless Automatizados ──
enable_testing()

add_executable(test_core src/core/arena.c src/core/rng.c src/core/persist.c tests/test_core.c)
target_include_directories(test_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(test_core PRIVATE m)
add_test(NAME CoreTest COMMAND test_core)

add_executable(test_snn src/core/arena.c src/core/rng.c src/core/snn.c tests/test_snn.c)
target_include_directories(test_snn PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(test_snn PRIVATE m)
add_test(NAME SnnTest COMMAND test_snn)

add_executable(test_biology src/core/neuromod.c src/core/cpg.c src/core/predict.c tests/test_biology.c)
target_include_directories(test_biology PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(test_biology PRIVATE m)
add_test(NAME BiologyTest COMMAND test_biology)

add_executable(test_sensors src/sense/sense_mouse.c src/sense/sense_scroll.c src/sense/sense_windows.c tests/test_sensors.c)
target_include_directories(test_sensors PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/sense)
target_link_libraries(test_sensors PRIVATE m)
add_test(NAME SensorsTest COMMAND test_sensors)

add_executable(test_body src/core/neuromod.c src/body/palette.c src/body/voice_synth.c tests/test_body.c)
target_include_directories(test_body PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/body)
target_link_libraries(test_body PRIVATE m)
add_test(NAME BodyTest COMMAND test_body)

add_executable(test_mind_lang src/mind/workspace.c src/lang/lexicon.c tests/test_mind_lang.c)
target_include_directories(test_mind_lang PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/mind ${CMAKE_CURRENT_SOURCE_DIR}/src/lang)
target_link_libraries(test_mind_lang PRIVATE m)
add_test(NAME MindLangTest COMMAND test_mind_lang)

add_executable(test_new_modules
    src/sense/sense_sysload.c
    src/sense/sense_heatmap.c
    src/sense/sense_mic.c
    src/sense/sense_music.c
    src/sense/sense_text.c
    src/behavior/behavior_dmn.c
    src/behavior/behavior_bond.c
    src/behavior/behavior_explore.c
    src/behavior/behavior_hide_hunt.c
    src/behavior/behavior_perch.c
    src/behavior/behavior_startle_relief.c
    src/behavior/behavior_dance.c
    src/animation/softbody.c
    tests/test_new_modules.c
)
target_include_directories(test_new_modules PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sense
    ${CMAKE_CURRENT_SOURCE_DIR}/src/behavior
)
target_link_libraries(test_new_modules PRIVATE m ${PLATFORM_LIBS})
add_test(NAME NewModulesTest COMMAND test_new_modules)
