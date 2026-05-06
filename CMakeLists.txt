cmake_minimum_required(VERSION 3.15)
project(lowhunt C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -pthread")

find_package(CURL REQUIRED)

include_directories(src/include)

set(SOURCES
    src/app/main.c
    src/config/config.c
    src/core/utils.c
    src/core/metadata.c
    src/core/resource_guard.c
    src/ui/banner.c
    src/ui/help_menu.c
    src/net/http.c
    src/net/scanner.c
    src/net/harvester.c
    src/output/output.c
    src/engines/engine_loader.c
    src/engines/parallel_engine.c
    src/engines/threadpool_engine.c
    src/engines/sync_engine.c
    src/engines/async_engine.c
    src/engines/fusion_engine.c
    src/engines/stabilizer_engine.c
    src/engines/intelligence_engine.c
    src/engines/brief_report_engine.c
)

add_executable(lowhunt ${SOURCES})
target_link_libraries(lowhunt CURL::libcurl pthread m)

install(TARGETS lowhunt DESTINATION /usr/local/bin)
install(DIRECTORY data/ DESTINATION /usr/share/lowhunt)
install(DIRECTORY platforms/ DESTINATION /usr/share/lowhunt/platforms)
install(FILES man/lowhunt.1 DESTINATION /usr/share/man/man1)
