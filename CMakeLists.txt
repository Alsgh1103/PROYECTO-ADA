cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE "C:/Users/alex_/Desktop/ADA/GUI/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain")

project(LogisticaVRP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Users/alex_/Desktop/ADA/GUI/vcpkg/installed/x64-windows")

find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System)
find_package(imgui CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)

add_executable(LogisticaVRP
    main.cpp
    Greedy.cpp
    FuerzaBruta.cpp
    Genetico.cpp
)

target_link_libraries(LogisticaVRP PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    imgui::imgui
    ImGui-SFML::ImGui-SFML
)
