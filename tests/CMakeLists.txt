# Copyright (c) BBP/EPFL 2016, Stefan.Eilemann@epfl.ch
# Change this number when adding tests to force a CMake run: 1

include(InstallFiles)

set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} Keyv)
if(LEVELDB_FOUND)
  list(APPEND TEST_LIBRARIES ${LEVELDB_LIBRARIES})
endif()

include(CommonCTest)
install_files(share/Hello/tests FILES ${TEST_FILES} COMPONENT examples)
