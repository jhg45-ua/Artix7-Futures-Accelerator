cmake_minimum_required(VERSION 3.10)
project(Artix7_Futures_Accelerator/software VERSION 0.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimizaciones agresivas para x86_64
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra -Wpedantic")

# Buscar pthreads en el sistema
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Rutas de inclusión
include_directories(include)
include_directories(/usr/local/include/twsapi)

# Declarar los fuentes
set(SOURCES
    src/main.cpp
    src/ibkr_client.cpp
    src/uart_interface.cpp
)

add_executable(hft_client ${SOURCES})

# Configurar rutas de búsqueda de librerías
link_directories(/usr/local/lib)

# Enlazado explícito de dependencias
target_link_libraries(hft_client
    PRIVATE
    twsapi
    bid
    protobuf
    Threads::Threads
)