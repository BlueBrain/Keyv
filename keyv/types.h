
/* Copyright (c) 2016, Stefan Eilemann <eile@equalizergraphics.com>
 *
 * This file is part of Keyv <https://github.com/BlueBrain/Keyv>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file keyv/types.h
 * Basic type definitions not provided by the operating system.
 */

/**
 * @namespace keyv
 * @brief Key-Value based storage and caching using a variety of backends
 *
 * Key-Value based storage and caching using a variety of backends. Keyv
 * provides a unified C++ keyv::Map frontend to store data in ceph, memcached
 * and leveldb.
 */

#ifndef KEYV_TYPES_H
#define KEYV_TYPES_H

#include <keyv/defines.h>
#include <lunchbox/types.h>
namespace keyv
{
using lunchbox::Strings;
}

#endif //KEYV_TYPES_H
