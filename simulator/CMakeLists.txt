cmake_minimum_required(VERSION 3.20)
project(ui_sim C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
  lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v8.3.11
)

FetchContent_Declare(
  lv_drivers
  GIT_REPOSITORY https://github.com/lvgl/lv_drivers.git
  GIT_TAG v8.3.0
)

FetchContent_MakeAvailable(lvgl lv_drivers)

find_package(SDL2 REQUIRED)

add_executable(ui_sim
  src/main.cpp
  ${CMAKE_CURRENT_LIST_DIR}/../shared/ui/src/ui.cpp
  ${lv_drivers_SOURCE_DIR}/sdl/sdl.c
  ${lv_drivers_SOURCE_DIR}/sdl/sdl_common.c
)

target_include_directories(ui_sim PRIVATE
  ${SDL2_INCLUDE_DIRS}
  ${lvgl_SOURCE_DIR}
  ${lv_drivers_SOURCE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../shared/ui/include
)

target_compile_definitions(ui_sim PRIVATE
  LV_CONF_INCLUDE_SIMPLE
  LV_DRV_CONF_INCLUDE_SIMPLE
  USE_SDL
  MONITOR_SDL_INCLUDE_PATH=\"SDL2/SDL.h\"
)

target_link_libraries(ui_sim PRIVATE
  lvgl
  ${SDL2_LIBRARIES}
)

# Ensure lvgl can find lv_conf.h
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)

# Ensure lv_drivers sees SDL headers and config macros
if(TARGET lv_drivers)
  target_include_directories(lv_drivers PUBLIC ${SDL2_INCLUDE_DIRS})
  target_compile_definitions(lv_drivers PUBLIC
    LV_DRV_CONF_INCLUDE_SIMPLE
    USE_SDL
    MONITOR_SDL_INCLUDE_PATH=\"SDL2/SDL.h\"
  )
endif()
