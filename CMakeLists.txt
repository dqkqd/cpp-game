cmake_minimum_required(VERSION 3.5)
project(mygame)

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

# Create your game executable target as usual
add_executable(mygame src/main.cpp)


# Link to the actual SDL3 library.
target_link_libraries(mygame PRIVATE SDL3::SDL3)
