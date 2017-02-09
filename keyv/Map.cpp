
/* Copyright (c) 2014-2017, Stefan.Eilemann@epfl.ch
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

#include "Map.h"
#include "Plugin.h"

#include <lunchbox/plugin.h>
#include <lunchbox/pluginFactory.h>
#include <servus/uri.h>

// #define HISTOGRAM

namespace keyv
{
namespace
{
using PluginFactory = lunchbox::PluginFactory<Plugin>;
}

class Map::Impl
{
public:
    Impl(const servus::URI& uri)
        : plugin(PluginFactory::getInstance().create(uri))
        , swap(false)
    {
    }

    std::unique_ptr<Plugin> plugin;
    bool swap;
#ifdef HISTOGRAM
    std::map<size_t, size_t> keys;
    std::map<size_t, size_t> values;
#endif
};

Map::Map(const servus::URI& uri)
    : _impl(new Impl(uri))
{
}

Map::Map(Map&& from)
    : _impl(std::move(from._impl))
{
}

Map& Map::operator=(Map&& from)
{
    if (this != &from)
        _impl = std::move(from._impl);
    return *this;
}

Map::~Map()
{
#ifdef HISTOGRAM
    std::cout << std::endl << "keys" << std::endl;
    for (std::pair<size_t, size_t> i : _impl->keys)
        std::cout << i.first << ", " << i.second << std::endl;
    std::cout << std::endl << "values" << std::endl;
    for (std::pair<size_t, size_t> i : _impl->values)
        std::cout << i.first << ", " << i.second << std::endl;
#endif
}

MapPtr Map::createCache()
{
    const servus::URI memcachedURI("memcached://");
    if (::getenv("MEMCACHED_SERVERS") && handles(memcachedURI))
        return MapPtr(new Map(servus::URI(memcachedURI)));

    const char* leveldb = ::getenv("LEVELDB_CACHE");
    if (leveldb && handles(servus::URI("leveldb://")))
        return MapPtr(new Map(
            servus::URI(std::string("leveldb:///cache/?store=") + leveldb)));
    return MapPtr();
}

bool Map::handles(const servus::URI& uri)
{
    return PluginFactory::getInstance().handles(uri);
}

std::string Map::getDescriptions()
{
    return PluginFactory::getInstance().getDescriptions();
}

size_t Map::setQueueDepth(const size_t depth)
{
    return _impl->plugin->setQueueDepth(depth);
}

bool Map::insert(const std::string& key, const void* data, const size_t size)
{
#ifdef HISTOGRAM
    ++_impl->keys[key.size()];
    ++_impl->values[size];
#endif
    return _impl->plugin->insert(key, data, size);
}

std::string Map::operator[](const std::string& key) const
{
    return (*_impl->plugin)[key];
}

void Map::getValues(const Strings& keys, const ConstValueFunc& func) const
{
    _impl->plugin->getValues(keys, func);
}

void Map::takeValues(const Strings& keys, const ValueFunc& func) const
{
    _impl->plugin->takeValues(keys, func);
}

bool Map::flush()
{
    return _impl->plugin->flush();
}

void Map::erase(const std::string& key)
{
    _impl->plugin->erase(key);
}

void Map::setByteswap(const bool swap)
{
    _impl->swap = swap;
}

bool Map::_swap() const
{
    return _impl->swap;
}
}
