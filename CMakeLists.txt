cmake_minimum_required(VERSION 3.20)
project(caddin C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(CADDIN_HEADLESS "Build headless (sem X11/display) para Android/CLI" OFF)

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
elseif(CADDIN_ANDROID_APP)
    message(STATUS "[PLATFORM] Compilando APK Nativo Android (ANativeWindow + JNI Overlay)")
    add_compile_definitions(ANDROID_APP CADDIN_HEADLESS WITHOUT_ALSA)
    set(PLATFORM_SRCS
        src/platform/window_android.c
        src/platform/android_main.c
        src/platform/android_overlay_bridge.c
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )
    include_directories(${ANDROID_NDK}/sources/android/native_app_glue)
    set(PLATFORM_LIBS m log android jnigraphics)
elseif(CADDIN_HEADLESS)
    # Modo headless: sem X11, sem ALSA — para Android/Termux/ADB
    message(STATUS "[PLATFORM] Modo HEADLESS — sem X11/ALSA")
    add_compile_definitions(CADDIN_HEADLESS WITHOUT_ALSA)
    set(PLATFORM_SRCS
        src/platform/window_headless.c
    )
    if(ANDROID)
        # No Android (Bionic libc), pthread e dl já fazem parte da libc básica
        set(PLATFORM_LIBS m log)
    else()
        set(PLATFORM_LIBS m pthread)
    endif()
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
    src/core/ontogeny.c
    src/core/config_rules.c
    src/core/sdm.c
    src/core/rituals.c
    src/core/event_ring.c
    src/core/cognitive_event.c
    src/core/actuator_guard.c
    src/core/carmin_snapshot.c
    src/core/async_request.c
    src/core/bandit.c
    src/core/daily_summary.c
    src/core/telemetry_fifo.c
    src/sense/sense_fatigue.c
    src/sense/sense_keystroke_biometrics.c
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

if(CADDIN_ANDROID_APP)
    add_library(caddin SHARED ${SRCS})
else()
    add_executable(caddin ${SRCS})
endif()

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

if(NOT WIN32 AND NOT CADDIN_HEADLESS)
    target_include_directories(caddin PRIVATE ${X11_INCLUDE_DIR})
endif()

target_link_libraries(caddin PRIVATE ${PLATFORM_LIBS})

# Benchmarks explicitamente invocados; ficam fora do build normal do app.
function(add_neuro_benchmark target scale dense_reference)
    add_executable(${target} EXCLUDE_FROM_ALL bench/bench_neuro_step.c neuro.c src/core/persist.c)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
    target_compile_definitions(${target} PRIVATE BRAIN_STEP_STATS_AVAILABLE=1 PESTINHA_NEURON_SCALE=${scale})
    if(dense_reference)
        target_compile_definitions(${target} PRIVATE CADDIN_DENSE_REFERENCE=1)
    endif()
    if(NOT WIN32)
        target_link_libraries(${target} PRIVATE m)
    endif()
endfunction()

add_neuro_benchmark(bench_neuro_step_2k 1 FALSE)
add_neuro_benchmark(bench_neuro_step_2k_dense 1 TRUE)
add_neuro_benchmark(bench_neuro_step_10k 5 FALSE)
add_neuro_benchmark(bench_neuro_step_10k_dense 5 TRUE)
add_neuro_benchmark(bench_neuro_step_100k 50 FALSE)
add_neuro_benchmark(bench_neuro_step_100k_dense 50 TRUE)
add_neuro_benchmark(bench_neuro_step_100k_sensory_reference 50 FALSE)
target_compile_definitions(bench_neuro_step_100k_sensory_reference PRIVATE CADDIN_SENSORY_BERNOULLI_REFERENCE=1)
add_neuro_benchmark(bench_neuro_step_1m 512 FALSE)
add_neuro_benchmark(bench_neuro_step_1m_dense 512 TRUE)

# ── Testes Headless Automatizados ──
enable_testing()

add_executable(test_core src/core/arena.c src/core/rng.c src/core/persist.c tests/test_core.c)
target_include_directories(test_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(test_core PRIVATE m)
add_test(NAME CoreTest COMMAND test_core)

add_executable(test_cognitive_event src/core/cognitive_event.c tests/test_cognitive_event.c)
target_include_directories(test_cognitive_event PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_cognitive_event PRIVATE -UNDEBUG)
endif()
target_link_libraries(test_cognitive_event PRIVATE m)
add_test(NAME CognitiveEventTest COMMAND test_cognitive_event)

add_executable(test_actuator_guard src/core/actuator_guard.c tests/test_actuator_guard.c)
target_include_directories(test_actuator_guard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_actuator_guard PRIVATE -UNDEBUG)
endif()
add_test(NAME ActuatorGuardTest COMMAND test_actuator_guard)

add_executable(test_carmin_snapshot src/core/carmin_snapshot.c tests/test_carmin_snapshot.c)
target_include_directories(test_carmin_snapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_carmin_snapshot PRIVATE -UNDEBUG)
endif()
add_test(NAME CarminSnapshotTest COMMAND test_carmin_snapshot)

add_executable(test_async_request src/core/async_request.c tests/test_async_request.c)
target_include_directories(test_async_request PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_async_request PRIVATE -UNDEBUG)
endif()
add_test(NAME AsyncRequestTest COMMAND test_async_request)

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

add_executable(test_ontogeny src/core/ontogeny.c tests/test_ontogeny.c)
target_include_directories(test_ontogeny PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(test_ontogeny PRIVATE m)
add_test(NAME OntogenyTest COMMAND test_ontogeny)

add_executable(test_config_rules src/core/config_rules.c tests/test_config_rules.c)
target_include_directories(test_config_rules PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(test_config_rules PRIVATE m)
add_test(NAME ConfigRulesTest COMMAND test_config_rules)

add_executable(test_sdm src/core/sdm.c tests/test_sdm.c)
target_include_directories(test_sdm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_sdm PRIVATE -UNDEBUG)
endif()
target_link_libraries(test_sdm PRIVATE m)
add_test(NAME SdmTest COMMAND test_sdm)

add_executable(bench_sdm_association EXCLUDE_FROM_ALL src/core/sdm.c bench/bench_sdm_association.c)
target_include_directories(bench_sdm_association PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(bench_sdm_association PRIVATE m)

add_executable(simulate_association EXCLUDE_FROM_ALL src/core/sdm.c bench/simulate_association.c)
target_include_directories(simulate_association PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(simulate_association PRIVATE m)

add_executable(test_rituals src/core/rituals.c tests/test_rituals.c)
target_include_directories(test_rituals PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(test_rituals PRIVATE m)
add_test(NAME RitualsTest COMMAND test_rituals)

add_executable(test_fase5 src/core/event_ring.c src/core/bandit.c src/sense/sense_fatigue.c tests/test_fase5.c)
target_include_directories(test_fase5 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/sense)
target_link_libraries(test_fase5 PRIVATE m)
add_test(NAME Fase5Test COMMAND test_fase5)

add_executable(test_fase6 src/sense/sense_keystroke_biometrics.c src/core/daily_summary.c src/core/telemetry_fifo.c tests/test_fase6.c)
target_include_directories(test_fase6 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/sense)
target_link_libraries(test_fase6 PRIVATE m)
add_test(NAME Fase6Test COMMAND test_fase6)

# ── Ferramenta CLI de Monitoramento de Spikes / Telemetria em C Puro ──
add_executable(caddin_monitor tools/caddin_monitor.c src/core/telemetry_fifo.c)
target_include_directories(caddin_monitor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
