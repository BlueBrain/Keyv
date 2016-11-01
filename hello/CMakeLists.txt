# Copyright (c) BBP/EPFL 2011-2014
#                        Stefan.Eilemann@epfl.ch
# All rights reserved. Do not distribute without further notice.

source_group(\\ FILES CMakeLists.txt)

set(HELLO_PUBLIC_HEADERS world.h)
set(HELLO_HEADERS)
set(HELLO_SOURCES world.cpp)
set(HELLO_LINK_LIBRARIES PRIVATE ${Boost_PROGRAM_OPTIONS_LIBRARY})

common_library(Hello)
