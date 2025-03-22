cmake_minimum_required(VERSION 3.15)
project(MPointers2.0 CXX)

set(CMAKE_CXX_STANDARD 17)

# Agregar ejecutables
add_executable(servidor servidor.cpp)
add_executable(cliente cliente.cpp)

# Forzar el uso de Winsock en Windows
if (WIN32)
    target_link_libraries(servidor PRIVATE ws2_32)
    target_link_libraries(cliente PRIVATE ws2_32)
endif()

