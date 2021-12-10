cmake_minimum_required (VERSION 3.2)
project(sharedmemclient C CXX)

SET (PROJECT_SOURCES
    main.cpp
    ../common/sharedmem.h
    ../common/sharedmem.cpp
)

# reduce the size of the Windows header files by excluding APIs such as Cryptography, DDE, RPC, Shell, and Windows Sockets
add_definitions(-DWIN32_LEAN_AND_MEAN)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

#target_include_directories(${PROJECT_NAME} PRIVATE ../common)