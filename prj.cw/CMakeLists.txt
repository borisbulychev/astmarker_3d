cmake_minimum_required(VERSION 3.20)
project(3d_unrolling_cw LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCV_DIR "$ENV{HOME}/vcpkg/installed/x64-osx/share/opencv4" CACHE PATH "")

find_package(OpenCV REQUIRED)
find_package(libigl CONFIG REQUIRED)

add_executable(unrolling_app
    src/main.cpp
    src/unrolling_3d_converter.cpp
)

target_include_directories(unrolling_app PRIVATE src)
target_include_directories(unrolling_app SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_libraries(unrolling_app PRIVATE
    ${OpenCV_LIBS}
    igl::igl_core
    igl::igl_glfw
    igl::igl_opengl
)

message(STATUS "✅ OpenCV linked successfully")

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Генерация документации Doxygen"
        VERBATIM
    )
endif()

install(TARGETS unrolling_app DESTINATION bin)