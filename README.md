/ˈkiːwi/
=======

# Overview

Key-Value based storage and caching using a variety of backends. Keyv
provides a unified C++ keyv::Map frontend to store data in ceph, memcached
and leveldb.

# Building from Source

Keyv is a cross-platform library, designed to run on any modern operating
system, including all Unix variants. It requires a C++11 compiler and uses CMake
to create a platform-specific build environment. The following platforms and
build environments are tested:

* Linux: Ubuntu 16.04, RHEL 6.8 (Makefile, Ninja)
* Mac OS X: 10.9 (Makefile, Ninja)

Building from source is as simple as:

    git clone --recursive https://github.com/BlueBrain/Keyv.git
    mkdir Keyv/build
    cd Keyv/build
    cmake -GNinja -DCLONE_SUBPROJECTS=ON ..
    ninja

# Funding & Acknowledgment
 
The development of this software was supported by funding to the Blue Brain Project,
a research center of the École polytechnique fédérale de Lausanne (EPFL), from the
Swiss government’s ETH Board of the Swiss Federal Institutes of Technology.

Copyright (c) 2021 Blue Brain Project/EPFL
