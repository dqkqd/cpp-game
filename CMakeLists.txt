cmake_minimum_required(VERSION 3.5)
project(mygame)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
option(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG dad1a84be4e9236267dd7c5393b56c60b3ed7875
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    )
FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG 77441588fa084269d4fe99ba21e2690853245764
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    )
FetchContent_Declare(
    SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG f9b636fc85b9355cea7482e9dd02c8be61060dc6
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    )
FetchContent_Declare(
    SDL3_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG 8ab0d03dc4eb1de59fa9a84c1d92e4fbd8fd6757
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    )

FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(SDL3_image)
FetchContent_MakeAvailable(SDL3_ttf)
FetchContent_MakeAvailable(SDL3_mixer)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

# Create your game executable target as usual
add_executable(mygame src/main.cpp)


# Link to the actual SDL3 library.
target_link_libraries(mygame PRIVATE SDL3::SDL3)
target_link_libraries(mygame PRIVATE SDL3_image::SDL3_image)
target_link_libraries(mygame PRIVATE SDL3_ttf::SDL3_ttf)
target_link_libraries(mygame PRIVATE SDL3_mixer::SDL3_mixer)
