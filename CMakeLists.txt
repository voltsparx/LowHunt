cmake_minimum_required(VERSION 3.15)
project(lowhunt C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -pthread")

find_package(CURL REQUIRED)

include_directories(include)

set(SOURCES
    src/main.c
    src/http.c
    src/scanner.c
    src/harvester.c
    src/output.c
    src/config.c
    src/utils.c
)

add_executable(lowhunt ${SOURCES})
target_link_libraries(lowhunt CURL::libcurl pthread m)

install(TARGETS lowhunt DESTINATION /usr/local/bin)
install(DIRECTORY data/ DESTINATION /usr/share/lowhunt)
