cmake_minimum_required(VERSION 3.10)
project(NeroHTTP VERSION 0.1.0 LANGUAGES C)

# Define modo padrão se não informado
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable(NeroHTTP ${SOURCES})

target_include_directories(NeroHTTP
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(NeroHTTP
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

# Link extra no Windows
if(WIN32)
    target_link_libraries(NeroHTTP PRIVATE ws2_32 shlwapi)
endif()
