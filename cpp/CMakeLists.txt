cmake_minimum_required(VERSION 3.16)
project(SnakeMath CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── SFML (binaires pré-compilés dans C:/SFML/SFML-2.6.1) ─────────────────────
set(SFML_DIR "C:/SFML/SFML-2.6.1/lib/cmake/SFML")
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(snake_math snake_math.cpp problems.cpp)

target_link_libraries(snake_math PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
)

# UTF-8 source + optimisations
if (MSVC)
    target_compile_options(snake_math PRIVATE /utf-8 /W3 /O2)
else()
    target_compile_options(snake_math PRIVATE -Wall -O2)
endif()

# Copie des DLLs SFML à côté de l'exe
add_custom_command(TARGET snake_math POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "C:/SFML/SFML-2.6.1/bin"
        "$<TARGET_FILE_DIR:snake_math>"
)

# Copie du dossier assets à côté de l'exe
add_custom_command(TARGET snake_math POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/../assets"
        "$<TARGET_FILE_DIR:snake_math>/assets"
)
