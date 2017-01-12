
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

#ifndef KEYV_MAP_H
#define KEYV_MAP_H

#include <keyv/api.h>
#include <keyv/types.h>

#include <lunchbox/bitOperation.h> // byteswap()
#include <lunchbox/debug.h> // className
#include <lunchbox/log.h> // LBTHROW
#include <servus/uri.h>

#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace keyv
{
/**
 * Unified interface to save key-value pairs in a store.
 *
 * Example: @include tests/Map.cpp
 */
class Map
{
public:
    /**
     * Construct a new  map.
     *
     * Depending on the URI scheme an implementation backend is chosen. If no
     * URI is given, a default one is selected. Available implementations are:
     * * ceph://path_to_ceph.conf (if KEYV_USE_RADOS is defined)
     * * leveldb://path (if KEYV_USE_LEVELDB is defined)
     * * memcached://[server] (if KEYV_USE_LIBMEMCACHED is defined)
     *
     * If no path is given for leveldb, the implementation uses
     * keyvMap.leveldb in the current working directory.
     *
     * If no servers are given for memcached, the implementation uses all
     * servers in the MEMCACHED_SERVERS environment variable, or
     * 127.0.0.1. MEMCACHED_SERVERS contains a comma-separated list of
     * servers. Each server contains the address, and optionally a
     * colon-separated port number.
     *
     * @param uri the storage backend and destination.
     * @throw std::runtime_error if no suitable implementation is found.
     * @throw std::runtime_error if opening the leveldb failed.
     * @version 1.9.2
     */
    KEYV_API explicit Map( const servus::URI& uri );
    KEYV_API Map( Map&& from );
    KEYV_API Map& operator = ( Map&& from );

    /** Destruct the  map. @version 1.9.2 */
    KEYV_API ~Map();

    /**
     * @return true if an implementation for the given URI is available.
     * @version 1.9.2
     */
    KEYV_API static bool handles( const servus::URI& uri );

    /** @return the descriptions of all available backends. @version 1.1 */
    KEYV_API static std::string getDescriptions();

    /**
     * Create a map which can be used for caching IO on the local system.
     *
     * The concrete implementation used depends on the system setup and
     * available backend implementations. If no suitable implementation is
     * found, a null pointer is returned.
     *
     * The current implementation returns:
     * * A memcached-backed cache if libmemcached is available and the
     *   environment variable MEMCACHED_SERVERS is set (see constructor
     *   documentation for details).
     * * A leveldb-backed cache if leveldb is available and LEVELDB_CACHE is set
     *   to the path for the leveldb storage.
     *
     * @return a Map for caching IO, or 0.
     */
    KEYV_API static MapPtr createCache();

    /**
     * Set the maximum number of asynchronous outstanding write operations.
     *
     * Some backend implementations support asynchronous writes, which can be
     * enabled by setting a non-zero queue depth. Applications then need to
     * quarantee that the inserted values stay valid until 'depth' other
     * elements have been inserted or flush() has been called. Implementations
     * which do not support asynchronous writes return 0.
     *
     * @return the queue depth chosen by the implementation, smaller or equal to
     *         the given depth.
     * @version 1.11
     */
    KEYV_API size_t setQueueDepth( const size_t depth );

    /**
     * Insert or update a value in the database.
     *
     * @param key the key to store the value.
     * @param value the value stored at the key.
     * @return true on success, false otherwise
     * @throw std::runtime_error if the value is not copyable
     * @version 1.9.2
     */
    template< class V > bool insert( const std::string& key, const V& value )
#if __GNUG__ && __GNUC__ < 5
        { return __has_trivial_copy(V) ? _insert( key, value, std::true_type( )) : _insert( key, value, std::false_type( ));}
#else
        { return _insert( key, value, std::is_trivially_copyable< V >( ));}
#endif
    KEYV_API bool insert( const std::string& key, const void* data,
                              size_t size );

    /**
     * Insert or update a vector of values in the database.
     *
     * @param key the key to store the value.
     * @param values the values stored at the key.
     * @return true on success, false otherwise
     * @throw std::runtime_error if the vector values are not copyable
     * @version 1.9.2
     */
    template< class V >
    bool insert( const std::string& key, const std::vector< V >& values )
#if __GNUG__ && __GNUC__ < 5
        { return __has_trivial_copy(V) ? _insert( key, values, std::true_type( )) : _insert( key, values, std::false_type( ));}
#else
        { return _insert( key, values, std::is_trivially_copyable< V >( ));}
#endif

    /**
     * Insert or update a set of values in the database.
     *
     * @param key the key to store the value.
     * @param values the values stored at the key.
     * @return true on success, false otherwise
     * @throw std::runtime_error if the set values are not copyable
     * @version 1.9.2
     */
    template< class V >
    bool insert( const std::string& key, const std::set< V >& values )
        { return insert( key, std::vector<V>( values.begin(), values.end( ))); }

    /**
     * Retrieve a value for a key.
     *
     * @param key the key to retrieve.
     * @return the value, or an empty string if the key is not available.
     * @version 1.9.2
     */
    KEYV_API std::string operator [] ( const std::string& key ) const;

    /**
     * Retrieve a value for a key.
     *
     * @param key the key to retrieve.
     * @return the value, or an empty string if the key is not available.
     * @version 1.11
     */
    template< class V > V get( const std::string& key ) const
        { return _get< V >( key ); }

    /**
     * Retrieve a value as a vector for a key.
     *
     * @param key the key to retrieve.
     * @return the values, or an empty vector if the key is not available.
     * @version 1.9.2
     */
    template< class V >
    std::vector< V > getVector( const std::string& key ) const;

    /**
     * Retrieve a value as a set for a key.
     *
     * @param key the key to retrieve.
     * @return the values, or an empty set if the key is not available.
     * @version 1.9.2
     */
    template< class V > std::set< V > getSet( const std::string& key ) const;

    /**
     * Retrieve values from a list of keys and calls back for each found value.
     *
     * Depending on the backend implementation, this is more optimal than
     * calling get() for each key.
     *
     * The ownership of the returned data in the callback is not transfered, so
     * the value needs to be copied if needed.
     *
     * @param keys list of keys to obtain
     * @param func callback function which is called for each found key
     * @version 1.14
     */
    KEYV_API void getValues( const Strings& keys,
                             const ConstValueFunc& func ) const;

    /**
     * Retrieve values from a list of keys and calls back for each found value.
     *
     * Depending on the backend implementation, this is more optimal than
     * calling get() for each key.
     *
     * The ownership of the returned data in the callback is transfered, so the
     * data must be free'd by the caller.
     *
     * @param keys list of keys to obtain
     * @param func callback function which is called for each found key
     * @version 1.14
     */
    KEYV_API void takeValues( const Strings& keys, const ValueFunc& func )const;

    /** Erase the given key from the store. @version 1.1 */
    KEYV_API void erase( const std::string& key );

    /** Flush outstanding operations to the backend storage. @version 1.0 */
    KEYV_API bool flush();

    /** Enable or disable endianness conversion on reads. @version 1.0 */
    KEYV_API void setByteswap( const bool swap );

private:
    Map( const Map& ) = delete;
    Map& operator = ( const Map& ) = delete;

    class Impl;
    std::unique_ptr< Impl > _impl;

    KEYV_API bool _swap() const;


    // Enables map.insert( "foo", "bar" ); bar is a char[4]. The funny braces
    // declare v as a "const ref to array of four chars", not as a "const array
    // to four char refs". Long live Bjarne!
    template< size_t N > bool
    _insert( const std::string& k, char const (& v)[N], const std::true_type&)
    {
        return insert( k, (void*)v, N - 1 ); // strip '0'
    }

    template< class V >
    bool _insert( const std::string& k, const V& v, const std::true_type& )
    {
        if( std::is_pointer< V >::value )
            LBTHROW( std::runtime_error( "Can't insert pointers" ));
        return insert( k, &v, sizeof( V ));
    }

    template< class V >
    bool _insert( const std::string&, const V& v, const std::false_type& )
    { LBTHROW( std::runtime_error( "Can't insert non-POD " +
                                   lunchbox::className( v ))); }
    template< class V >
    bool _insert( const std::string& key, const std::vector< V >& values,
                  const std::true_type& )
        { return insert( key, values.data(), values.size() * sizeof( V )); }

    template< class V > V _get( const std::string& k ) const
    {
#if __GNUG__ && __GNUC__ < 5
        if( !__has_trivial_copy(V) )
#else
        if( !std::is_trivially_copyable< V >::value )
#endif
            LBTHROW( std::runtime_error( "Can't retrieve non-POD " +
                                         lunchbox::className( V( ))));
        if( std::is_pointer< V >::value )
            LBTHROW( std::runtime_error( "Can't retrieve pointers" ));

        const std::string& value = (*this)[ k ];
        if( value.size() != sizeof( V ))
            LBTHROW( std::runtime_error( std::string( "Wrong value size " ) +
                                         std::to_string( value.size( )) +
                                         " for type " +
                                         lunchbox::className( V( ))));

        V v( *reinterpret_cast< const V* >( value.data( )));
        if( _swap( ))
            lunchbox::byteswap( v );
        return v;
    }
};

template<> inline
bool Map::_insert( const std::string& k, const std::string& v,
                             const std::false_type& )
{
    return insert( k, v.data(), v.length( ));
}

template< class V > inline
std::vector< V > Map::getVector( const std::string& key ) const
{
    const std::string& value = (*this)[ key ];
    std::vector< V > vector( reinterpret_cast< const V* >( value.data( )),
                   reinterpret_cast< const V* >( value.data() + value.size( )));
    if( _swap() && sizeof( V ) != 1 )
    {
        for( V& elem : vector )
            lunchbox::byteswap( elem );
    }
    return vector;
}

template< class V > inline
std::set< V > Map::getSet( const std::string& key ) const
{
    std::string value = (*this)[ key ];
    V* const begin = reinterpret_cast< V* >(
                         const_cast< char * >( value.data( )));
    V* const end = begin + value.size() / sizeof( V );

    if( _swap() && sizeof( V ) != 1 )
        for( V* i = begin; i < end; ++i )
            lunchbox::byteswap( *i );

    return std::set< V >( begin, end );
}

}

#endif //KEYV_MAP_H
