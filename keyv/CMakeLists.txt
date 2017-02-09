# Copyright (c) BBP/EPFL 2016-2017 Stefan.Eilemann@epfl.ch

set(KEYV_PUBLIC_HEADERS Map.h Plugin.h types.h)
set(KEYV_SOURCES Map.cpp)

set(KEYV_LINK_LIBRARIES PUBLIC Lunchbox)

if(LEVELDB_FOUND)
  list(APPEND KEYV_LINK_LIBRARIES PRIVATE ${LEVELDB_LIBRARIES})
  list(APPEND KEYV_SOURCES LevelDB.cpp)
endif()
if(libmemcached_FOUND)
  list(APPEND KEYV_LINK_LIBRARIES PRIVATE ${libmemcached_LIBRARIES})
  if(TARGET PressionData)
    list(APPEND KEYV_LINK_LIBRARIES PressionData)
  endif()
  list(APPEND KEYV_SOURCES Memcached.cpp)
endif()
if(RADOS_FOUND)
  list(APPEND KEYV_LINK_LIBRARIES PRIVATE rados)
  list(APPEND KEYV_SOURCES Ceph.cpp)
endif()

common_library(Keyv)
