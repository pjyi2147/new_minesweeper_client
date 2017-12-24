cmake_minimum_required (VERSION 3.5)
project (minesweeper_client)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

file(GLOB MAIN src/main.cpp)
file(GLOB COMMON src/common/*.cpp)
file(GLOB CLIENT src/client/*.cpp)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(m_client ${MAIN} ${COMMON} ${CLIENT})
    target_link_libraries(m_client ${Boost_LIBRARIES} 
                                   ${CMAKE_THREAD_LIBS_INIT})
endif()