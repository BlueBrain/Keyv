
/* Copyright (c) 2014-2016, Stefan.Eilemann@epfl.ch
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
#include <servus/uri.h>

// #define HISTOGRAM

namespace keyv
{
namespace detail
{
class Map
{
public:
    Map() : swap( false ) {}
    virtual ~Map() {}
    virtual size_t setQueueDepth( const size_t ) { return 0; }
    virtual bool insert( const std::string& key, const void* data,
                         const size_t size ) = 0;
    virtual std::string operator [] ( const std::string& key ) const = 0;
    virtual bool fetch( const std::string&, const size_t ) const
        { return true; }
    virtual void getValues( const Strings& keys,
                            const ConstValueFunc& func ) const = 0;
    virtual void takeValues( const Strings& keys,
                             const ValueFunc& func ) const = 0;
    virtual bool flush() = 0;

    bool swap;
#ifdef HISTOGRAM
    std::map< size_t, size_t > keys;
    std::map< size_t, size_t > values;
#endif
};
}
}

// Impls - need detail::Map interface above
#include "ceph/Map.h"
#include "leveldb/Map.h"
#include "memcached/Map.h"

namespace
{
keyv::detail::Map* _newImpl( const servus::URI& uri )
{
    // Update handles() below on any change here!
#ifdef KEYV_USE_RADOS
    if( keyv::ceph::Map::handles( uri ))
        return new keyv::ceph::Map( uri );
#endif
#ifdef KEYV_USE_LEVELDB
    if( keyv::leveldb::Map::handles( uri ))
        return new keyv::leveldb::Map( uri );
#endif
#ifdef KEYV_USE_LIBMEMCACHED
    if( keyv::memcached::Map::handles( uri ))
        return new keyv::memcached::Map( uri );
#endif

    LBTHROW( std::runtime_error(
                 std::string( "No suitable implementation found for: " ) +
                              std::to_string( uri )));
}
}

namespace keyv
{
Map::Map( const servus::URI& uri )
    : _impl( _newImpl( uri ))
{}

Map::~Map()
{
#ifdef HISTOGRAM
    std::cout << std::endl << "keys" << std::endl;
    for( std::pair< size_t, size_t > i : _impl->keys )
        std::cout << i.first << ", " << i.second << std::endl;
    std::cout << std::endl << "values" << std::endl;
    for( std::pair< size_t, size_t > i : _impl->values )
        std::cout << i.first << ", " << i.second << std::endl;
#endif
    delete _impl;
}

MapPtr Map::createCache()
{
#ifdef KEYV_USE_LIBMEMCACHED
    if( ::getenv( "MEMCACHED_SERVERS" ))
        return MapPtr( new Map(
                                     servus::URI( "memcached://" )));
#endif
#ifdef KEYV_USE_LEVELDB
    const char* leveldb = ::getenv( "LEVELDB_CACHE" );
    if( leveldb )
        return MapPtr( new Map( servus::URI(
                                      std::string( "leveldb://" ) + leveldb )));
#endif

    return MapPtr();
}

bool Map::handles( const servus::URI& uri LB_UNUSED )
{
#ifdef KEYV_USE_LEVELDB
    if( keyv::leveldb::Map::handles( uri ))
        return true;
#endif
#ifdef KEYV_USE_RADOS
    if( keyv::ceph::Map::handles( uri ))
        return true;
#endif
#ifdef KEYV_USE_LIBMEMCACHED
    if( keyv::memcached::Map::handles( uri ))
        return true;
#endif
    return false;
}

size_t Map::setQueueDepth( const size_t depth )
{
    return _impl->setQueueDepth( depth );
}

bool Map::insert( const std::string& key, const void* data,
                            const size_t size )
{
#ifdef HISTOGRAM
    ++_impl->keys[ key.size() ];
    ++_impl->values[ size ];
#endif
    return _impl->insert( key, data, size );
}

std::string Map::operator [] ( const std::string& key ) const
{
    return (*_impl)[ key ];
}

bool Map::fetch( const std::string& key, const size_t sizeHint ) const
{
    return _impl->fetch( key, sizeHint );
}

void Map::getValues( const Strings& keys,
                               const ConstValueFunc& func ) const
{
    _impl->getValues( keys, func );
}

void Map::takeValues( const Strings& keys,
                                const ValueFunc& func ) const
{
    _impl->takeValues( keys, func );
}

bool Map::flush()
{
    return _impl->flush();
}

void Map::setByteswap( const bool swap )
{
    _impl->swap = swap;
}

bool Map::_swap() const
{
    return _impl->swap;
}

}