
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

#include <keyv/Plugin.h>

#include <lunchbox/compiler.h>
#include <lunchbox/log.h>
#include <lunchbox/pluginRegisterer.h>

#include <leveldb/db.h>

namespace keyv
{
namespace db = ::leveldb;
class LevelDB;

namespace
{
lunchbox::PluginRegisterer< LevelDB > registerer;

db::DB* _open( const servus::URI& uri )
{
    db::DB* db = 0;
    db::Options options;
    options.create_if_missing = true;
    const auto store = uri.findQuery( "store" );
    const std::string& path = store == uri.queryEnd() ? "keyvMap.leveldb" :
                                                        store->second;
    const db::Status status = db::DB::Open( options, path, &db );
    if( !status.ok( ))
        LBTHROW( std::runtime_error( status.ToString( )));
    return db;
}
}

class LevelDB : public Plugin
{
public:
    explicit LevelDB( const servus::URI& uri )
        : _db( _open( uri ))
        , _path( uri.getPath() + "/" )
    {}

    virtual ~LevelDB() { delete _db; }

    static bool handles( const servus::URI& uri )
        { return uri.getScheme() == "leveldb" || uri.getScheme().empty(); }

    static std::string getDescription()
        { return "leveldb://[/namespace][?store=path_to_leveldb_dir]"; }

    bool insert( const std::string& key, const void* data, const size_t size )
        final
    {
        const db::Slice value( (const char*)data, size );
        return _db->Put( db::WriteOptions(), _path + key, value ).ok();
    }

    std::string operator [] ( const std::string& key ) const final
    {
        std::string value;
        if( _db->Get( db::ReadOptions(), _path + key, &value ).ok( ))
            return value;
        return std::string();
    }

    void takeValues( const Strings& keys, const ValueFunc& func ) const final
    {
        for( const auto& key: keys )
        {
            std::string value;
            if( !_db->Get( db::ReadOptions(), _path + key, &value ).ok( ))
                continue;

            char* copy = (char*)malloc( value.size( ));
            memcpy( copy, value.data(), value.size( ));
            func( key, copy, value.size( ));
        }
    }

    void getValues( const Strings& keys, const ConstValueFunc& func ) const
        final
    {
        for( const auto& key: keys )
        {
            std::string value;
            if( !_db->Get( db::ReadOptions(), _path + key, &value ).ok( ))
                continue;

            func( key, value.data(), value.size( ));
        }
    }

    bool flush() final { /*NOP?*/ return true; }

private:
    db::DB* const _db;
    const std::string _path;
};
}
