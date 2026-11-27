cmake_minimum_required(VERSION 4.3)
project(Minesweeper C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find the locally bundled Raven library
find_package(raven CONFIG REQUIRED
        PATHS "${CMAKE_CURRENT_SOURCE_DIR}/lib/raven"
        NO_DEFAULT_PATH
)

file(COPY views DESTINATION ${CMAKE_BINARY_DIR})

add_executable(Minesweeper main.c
        minesweeper.c
        minesweeper.h)

# Link Raven
target_link_libraries(Minesweeper PRIVATE raven::raven)

# Automatically copy XML when modified
set(VIEW_FILE "${CMAKE_CURRENT_SOURCE_DIR}/views/main.xml")
set(BUILD_VIEW "${CMAKE_CURRENT_BINARY_DIR}/views/main.xml")

add_custom_command(
        OUTPUT "${BUILD_VIEW}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/views"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VIEW_FILE}" "${BUILD_VIEW}"
        DEPENDS "${VIEW_FILE}"
        COMMENT "Copying Raven XML view"
        VERBATIM
)

add_custom_target(CopyViews ALL DEPENDS "${BUILD_VIEW}")
add_dependencies(Minesweeper CopyViews)