
/* Copyright (c) 2017, Stefan.Eilemann@epfl.ch
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

#pragma once

#include <keyv/types.h>

#include <servus/uri.h>

#include <lunchbox/compiler.h>

namespace keyv
{
/** Interface for all Map plugins */
class Plugin
{
public:
    Plugin() {}
    virtual ~Plugin() {}

    /** @internal Needed by the PluginRegisterer. */
    typedef Plugin InterfaceT;

    /** @internal Needed by the PluginRegisterer. */
    typedef servus::URI InitDataT;

    /** @copydoc Map::setQueueDepth */
    virtual size_t setQueueDepth( size_t size LB_UNUSED ) { return 0; }

    /** @copydoc Map::insert */
    virtual bool insert( const std::string& key, const void* data,
                         size_t size ) = 0;

    /** @copydoc Map::flush */
    virtual bool flush() = 0;

    /** @copydoc Map::operator[] */
    virtual std::string operator[]( const std::string& key ) const = 0;

    /** @copydoc Map::getValues */
    virtual void getValues( const Strings& keys,
                            const ConstValueFunc& func ) const = 0;

    /** @copydoc Map::takeValues */
    virtual void takeValues( const Strings& keys,
                             const ValueFunc& func ) const = 0;

private:
    Plugin( const Plugin& ) = delete;
    Plugin( Plugin&& ) = delete;
    Plugin& operator=( const Plugin& ) = delete;
    Plugin& operator=( Plugin&& ) = delete;
};
}
