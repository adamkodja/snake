cmake_minimum_required(VERSION 3.16)
project(SnakeMath CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Génère compile_commands.json (IntelliSense sous Ninja/Make)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── SFML ──────────────────────────────────────────────────────────────────────
# Indiquer la racine SFML par ordre de priorité :
#   1. cmake -DSFML_ROOT=/chemin/vers/SFML-x.y.z  ..
#   2. Variable d'environnement  SFML_ROOT
#   3. SFML installé en système (vcpkg, Homebrew, apt…) — aucune variable requise

set(SFML_ROOT "" CACHE PATH
    "Racine de SFML — dossier qui contient include/ lib/ bin/ (ex: C:/SFML/SFML-2.6.1)")

# Lecture depuis la variable d'environnement si non fournie en ligne de commande
if(SFML_ROOT STREQUAL "" AND DEFINED ENV{SFML_ROOT})
    set(SFML_ROOT "$ENV{SFML_ROOT}" CACHE PATH "" FORCE)
endif()

# Quand SFML_ROOT est connu, on en déduit SFML_DIR (chemin vers SFMLConfig.cmake)
if(NOT SFML_ROOT STREQUAL "")
    set(SFML_DIR "${SFML_ROOT}/lib/cmake/SFML")
endif()

find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(snake_math snake_math.cpp problems.cpp)

target_link_libraries(snake_math PRIVATE
    sfml-graphics sfml-window sfml-system)

if(MSVC)
    target_compile_options(snake_math PRIVATE /utf-8 /W3 /O2)
else()
    target_compile_options(snake_math PRIVATE -Wall -O2)
endif()

# RPATH Linux/macOS : l'exe trouve les .so/.dylib SFML même hors system path
if(UNIX AND NOT SFML_ROOT STREQUAL "")
    set_target_properties(snake_math PROPERTIES
        BUILD_RPATH   "${SFML_ROOT}/lib"
        INSTALL_RPATH "${SFML_ROOT}/lib"
    )
endif()

# ── Post-build : copier les DLLs SFML (Windows) ───────────────────────────────
if(WIN32)
    if(NOT SFML_ROOT STREQUAL "")
        # Binaires fournis dans SFML_ROOT/bin
        add_custom_command(TARGET snake_math POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${SFML_ROOT}/bin"
                "$<TARGET_FILE_DIR:snake_math>"
        )
    else()
        # Fallback : copie via les DLLs des cibles importées (vcpkg, etc.)
        add_custom_command(TARGET snake_math POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:snake_math>
                $<TARGET_FILE_DIR:snake_math>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

# ── Post-build : copier les assets ───────────────────────────────────────────
add_custom_command(TARGET snake_math POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/../assets"
        "$<TARGET_FILE_DIR:snake_math>/assets"
)
