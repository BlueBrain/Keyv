# Copyright (c) BBP/EPFL 2011-2014
#                        Stefan.Eilemann@epfl.ch
# All rights reserved. Do not distribute without further notice.

add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK) # Fix for windows and shared boost.
set(RULE_HEADERS)
set(RULE_SOURCES main.cpp)
set(RULE_LINK_LIBRARIES Hello ${Boost_PROGRAM_OPTIONS_LIBRARY})

common_application(rule)
