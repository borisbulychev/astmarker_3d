cmake_minimum_required(VERSION 3.20)
project(3d_unrolling_cw LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Все бинарники и DLL от vcpkg — в одну папку
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/out)

find_package(OpenCV REQUIRED)
find_package(libigl CONFIG REQUIRED)

add_executable(unrolling_app
    src/main.cpp
    src/unrolling_3d_converter.cpp
)

target_include_directories(unrolling_app PRIVATE src)
target_include_directories(unrolling_app SYSTEM PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(unrolling_app PRIVATE
    ${OpenCV_LIBS}
    igl::igl_core
    igl::igl_glfw
    igl::igl_opengl
)

# ── Doxygen ───────────────────────────────────────────────────────
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN  ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Генерация документации Doxygen"
        VERBATIM
    )
endif()

# ── Install: исполняемый файл ─────────────────────────────────────
install(TARGETS unrolling_app
    DESTINATION bin
)

# ── Install: тестовые данные ──────────────────────────────────────
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
    DESTINATION bin/data
)

# ── Install: DLL-зависимости (только Windows) ─────────────────────
if(WIN32)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/out/
        DESTINATION bin
        FILES_MATCHING PATTERN "*.dll"
    )
endif()