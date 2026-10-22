cmake_minimum_required(VERSION 3.15)
project(DisasterReliefSystem VERSION 1.0.0 LANGUAGES C CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(include)

# Compile SQLite3 as a pure C static library to avoid C++ name mangling/syntax issues
add_library(sqlite3 STATIC src/sqlite3.c)
target_include_directories(sqlite3 PUBLIC include)

# Application executable
add_executable(DisasterReliefSystem
    src/main.cpp
    src/database.cpp
    src/disaster.cpp
    src/resource.cpp
    src/relief_request.cpp
    src/distribution.cpp
    src/system.cpp
)

# Link SQLite3 and threading
target_link_libraries(DisasterReliefSystem PRIVATE sqlite3)

find_package(Threads REQUIRED)
target_link_libraries(DisasterReliefSystem PRIVATE Threads::Threads)

# Windows Sockets & networking for Crow
if(WIN32)
    target_link_libraries(DisasterReliefSystem PRIVATE
        ws2_32
        mswsock
    )
endif()

# Compiler flags
target_compile_definitions(DisasterReliefSystem PRIVATE
    _WIN32_WINNT=0x0601
    CROW_STATIC_ENDPOINT
)
