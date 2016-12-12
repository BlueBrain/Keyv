# Copyright (c) BBP/EPFL 2016 Stefan.Eilemann@epfl.ch

set(KEYV_PUBLIC_HEADERS
  Map.h
  types.h
  )
set(KEYV_HEADERS
  ceph/Plugin.h
  leveldb/Plugin.h
  memcached/Plugin.h
)
set(KEYV_SOURCES Map.cpp)

set(KEYV_LINK_LIBRARIES PUBLIC Lunchbox PRIVATE PressionData)

if(LEVELDB_FOUND)
  list(APPEND KEYV_LINK_LIBRARIES ${LEVELDB_LIBRARIES})
endif()
if(libmemcached_FOUND)
  list(APPEND KEYV_LINK_LIBRARIES ${libmemcached_LIBRARIES})
endif()
if(RADOS_FOUND)
  list(APPEND KEYV_LINK_LIBRARIES rados)
endif()

common_library(Keyv)
