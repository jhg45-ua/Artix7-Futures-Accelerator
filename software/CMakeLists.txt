cmake_minimum_required(VERSION 3.10)
project(Artix7_Futures_Accelerator/software VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Mantenemos tus flags sumando -O3 y -march=native para exprimir el hardware
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra -Wpedantic")

# Buscar pthreads en el sistema
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Rutas de inclusión (Equivalente a tu INCLUDES)
include_directories(include)
include_directories(/usr/local/include/twsapi)

# Declarar los fuentes distribuidos en carpetas
set(SOURCES
    src/main.cpp
    src/ibkr_client.cpp
)

add_executable(hft_client ${SOURCES})

# Configurar rutas de búsqueda de librerías (Equivalente a -L/usr/local/lib)
link_directories(/usr/local/lib)

# Enlazado explícito (Equivalente exacto a tu LDFLAGS)
target_link_libraries(hft_client
    PRIVATE
    twsapi
    bid
    protobuf
    Threads::Threads
)