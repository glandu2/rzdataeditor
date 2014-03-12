cmake_minimum_required(VERSION 2.8)
project(RappelzDataEditor)


#set(CMAKE_VERBOSE_MAKEFILE true)
set(BUILD_SHARED_LIBS true)
set(CMAKE_BUILD_TYPE Debug)

#######################################


set(SHARED_LIB_DIR "shared-lib")
set(KIOSLAVE_LIB_DIR "kioslave-rpz")
set(CLIENTS_DIR "clients")


set(CMAKE_SHARED_LIBRARY_PREFIX "")

get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/${BUILD_DIR_NAME}-bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/${BUILD_DIR_NAME}-bin")

set(SHARED_LIB_PATH "${CMAKE_CURRENT_LIST_DIR}/${SHARED_LIB_DIR}")
set(KIOSLAVE_LIB_PATH "${CMAKE_CURRENT_LIST_DIR}/${KIOSLAVE_LIB_DIR}")
set(CLIENTS_PATH "${CMAKE_CURRENT_LIST_DIR}/${CLIENTS_DIR}")

set(SHARED_LIB_NAME "${PROJECT_NAME}")

include_directories("${SHARED_LIB_PATH}")

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "/MANIFEST:NO /INCREMENTAL:NO")
  set(CMAKE_MODULE_LINKER_FLAGS "/MANIFEST:NO /INCREMENTAL:NO")
  set(CMAKE_SHARED_LINKER_FLAGS "/MANIFEST:NO /INCREMENTAL:NO")

  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

  if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -Wl,--add-stdcall-alias")
  endif()

  #for qtcreator
  add_definitions("-D__GXX_EXPERIMENTAL_CXX0X__")
endif()

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath,.")
endif()

add_subdirectory("${SHARED_LIB_PATH}" ${SHARED_LIB_DIR})
add_subdirectory("${CLIENTS_PATH}" ${CLIENTS_DIR})

if(UNIX)
  add_subdirectory("${KIOSLAVE_LIB_PATH}" ${KIOSLAVE_LIB_DIR})
endif()
