# Copyright (c) BBP/EPFL 2016-2017, Stefan.Eilemann@epfl.ch
# Change this number when adding tests to force a CMake run: 1

include(InstallFiles)

set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} Keyv)

if(LEVELDB_FOUND)
  list(APPEND TEST_LIBRARIES ${LEVELDB_LIBRARIES})
endif()

if(RADOS_FOUND)
  list(APPEND TEST_LIBRARIES ${RADOS_LIBRARIES})
endif()

set(UNIT_AND_PERF_TESTS Map.cpp)

include(CommonCTest)
install_files(share/Keyv/tests FILES ${TEST_FILES} COMPONENT examples)
