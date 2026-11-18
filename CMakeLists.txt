cmake_minimum_required(VERSION 3.16)
project(pemu_frontend LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # pour clangd / l'autocomplétion

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include(FetchContent)

# ---------------------------------------------------------------------------
# libretro.h  (header seul, vient de libretro-common)
# ---------------------------------------------------------------------------
FetchContent_Declare(
    libretro_common
    GIT_REPOSITORY https://github.com/libretro/libretro-common.git
    GIT_TAG        fa8a1b5          # même commit que melonDS DS
)
# Pas de CMakeLists.txt dans ce repo : MakeAvailable se contente de le télécharger
FetchContent_MakeAvailable(libretro_common)

add_library(libretro_api INTERFACE)
target_include_directories(libretro_api SYSTEM INTERFACE
    ${libretro_common_SOURCE_DIR}/include)

# ---------------------------------------------------------------------------
# SDL3 : installée sur le système (PC ou sysroot Buildroot), sinon téléchargée
# ---------------------------------------------------------------------------
find_package(SDL3 CONFIG QUIET)

if(SDL3_FOUND)
    message(STATUS "SDL3 ${SDL3_VERSION} trouvée sur le système")
else()
    message(STATUS "SDL3 introuvable -> téléchargement et compilation")
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.4.16
        GIT_SHALLOW    TRUE
    )
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON  CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL3)
endif()
# SDL3::SDL3 pointe vers la version partagée ou statique selon ce qui existe

# ---------------------------------------------------------------------------
# Exécutable
# ---------------------------------------------------------------------------
file(GLOB_RECURSE PEMU_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.c)

add_executable(pemu ${PEMU_SOURCES})

target_include_directories(pemu PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(pemu PRIVATE
    libretro_api
    SDL3::SDL3
    ${CMAKE_DL_LIBS}    # dlopen / dlsym pour charger le core
)

target_compile_options(pemu PRIVATE -Wall -Wextra)

install(TARGETS pemu RUNTIME DESTINATION bin)