CMAKE_MINIMUM_REQUIRED(VERSION 3.24)
project(labwork-10-notoriginallink VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

if(WIN32)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

include(FetchContent)

FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.10.2
)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.10.2
)

FetchContent_MakeAvailable(cpr json)

add_subdirectory(lib/WeatherForecast)
add_executable( Weather main.cpp )
target_link_libraries(Weather PRIVATE cpr::cpr nlohmann_json::nlohmann_json WeatherForecast pdcurses)



