cmake_minimum_required(VERSION 3.20)

project(FitnessTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Crow CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(fitness_tracker
    src/main.cpp
    src/database.cpp
)

target_include_directories(fitness_tracker PRIVATE
    include
)

target_link_libraries(fitness_tracker PRIVATE
    Crow::Crow
    unofficial::sqlite3::sqlite3
)