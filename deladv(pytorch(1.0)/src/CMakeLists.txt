cmake_minimum_required(VERSION 2.8.10)
project(radio)

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
endif()

message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE release CACHE STRING "Choose the type of build" FORCE)
endif()

find_package(OpenMP)
if(OpenMP_CXX_FOUND OR OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

add_definitions(-w)
add_definitions(-fPIC)
add_definitions(-std=c++11)

#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -ggdb ")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -DUSE_FBGEMM -DUSE_QNNPACK -DUSE_PYTORCH_QNNPACK")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "CMAKE_CURRENT_BINARY_DIR = ${CMAKE_CURRENT_BINARY_DIR}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/librosa")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/utils")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/mel")

include_directories(/root/pytorch)
include_directories(/root/pytorch/build)
include_directories(/root/pytorch/third_party/protobuf/src)
include_directories(/root/pytorch/third_party/eigen)
include_directories(/root/pytorch/torch/include)
include_directories(/root/pytorch/torch/csrc/api/include/)

include_directories(/root/lame/include/lame)
include_directories(/root/openBlas/include)


set(SRCS 
    librosa/AudioFFT.cpp
	mel/mel.cpp
	main.cpp)

 
function(import_share path name)

  add_library(${name} SHARED IMPORTED)
  set_property(
      TARGET ${name}
      PROPERTY IMPORTED_LOCATION
      ${path}/${name}.so)
endfunction(import_share)

function(import_static path name)

  add_library(${name} STATIC IMPORTED)
  set_property(
      TARGET ${name}
      PROPERTY IMPORTED_LOCATION
      ${path}/${name}.a)
endfunction(import_static)

set(caffe2path /root/pytorch/build/lib)
# caffe2 lib

import_static(${caffe2path} libqnnpack)
import_static(${caffe2path} libpytorch_qnnpack)
import_static(${caffe2path} libmkldnn)
import_static(${caffe2path} libprotobuf)
import_share(${caffe2path} libtorch)
import_share(${caffe2path} libc10)
import_share(${caffe2path} libcaffe2_observers)
import_static(/root/lame/lib libmp3lame)
import_static(/root/openBlas/lib libopenblas)



#compile
add_executable(radio ${SRCS})


target_link_libraries(radio
                      -Wl,-rpath=${caffe2path}
					  libtorch
					  libprotobuf
					  libc10
					  libmkldnn
					  libcaffe2_observers					  
					  -lpthread 
					  libmp3lame
					  libopenblas)

install(TARGETS
	radio
	DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../bin/ COMPONENT radio)



