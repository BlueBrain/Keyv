# Copyright (c) BBP/EPFL 2016 Stefan.Eilemann@epfl.ch

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Keyv VERSION 1.1.0)
set(Keyv_VERSION_ABI 2)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
                              ${CMAKE_SOURCE_DIR}/CMake/common)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/CMake/common/Common.cmake)
  message(FATAL_ERROR "CMake/common missing, run: git submodule update --init")
endif()

set(KEYV_DESCRIPTION
  "Key-Value based storage and caching using a variety of backends")
set(KEYV_MAINTAINER "Blue Brain Project <bbp-open-source@googlegroups.com>")
set(KEYV_LICENSE LGPL)

set(KEYV_DEB_DEPENDS libboost-filesystem-dev libboost-test-dev
  libleveldb-dev libmemcached-dev libmemcached-tools librados-dev memcached)
set(KEYV_PORT_DEPENDS boost libmemcached memcached)

set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
include(Common)

common_find_package(Boost REQUIRED COMPONENTS filesystem unit_test_framework)
common_find_package(Lunchbox REQUIRED)
common_find_package(Servus REQUIRED)
common_find_package(leveldb)
common_find_package(libmemcached 1.0.12)
if(libmemcached_FOUND)
  option(KEYV_MEMCACHED_COMPRESSION "Use compression in memcached backend" OFF)
  if(KEYV_MEMCACHED_COMPRESSION)
    git_subproject(Pression https://github.com/Eyescale/Pression.git dc49d02)
    common_find_package(Pression REQUIRED)
  endif()
endif()
common_find_package(rados)
common_find_package_post()

add_subdirectory(keyv)
add_subdirectory(tests)

set(DOXYGEN_MAINPAGE_MD README.md)
set(DOXYGEN_EXTRA_INPUT ${PROJECT_SOURCE_DIR}/README.md
  ${PROJECT_SOURCE_DIR}/doc/)
include(CPackConfig)
include(DoxygenRule)
