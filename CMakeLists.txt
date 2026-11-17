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
        m
    )
else()
    find_package(X11 REQUIRED)
    set(PLATFORM_SRCS
        src/platform/window_x11.c
        src/platform/x11_cursor.c
    )
    set(PLATFORM_LIBS
        ${X11_LIBRARIES}
        ${X11_Xext_LIB}
        m
    )
endif()

set(SRCS
    src/main.c
    ${PLATFORM_SRCS}
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animation
)

if(NOT WIN32)
    target_include_directories(caddin PRIVATE ${X11_INCLUDE_DIR})
endif()

target_link_libraries(caddin PRIVATE ${PLATFORM_LIBS})
