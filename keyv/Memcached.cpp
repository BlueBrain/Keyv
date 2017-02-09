
/* Copyright (c) 2016-2017, Stefan.Eilemann@epfl.ch
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
#include <libmemcached/memcached.h>
#include <lunchbox/pluginRegisterer.h>
#include <lunchbox/uint128_t.h>
#include <pression/data/CompressorSnappy.h>
#include <pression/data/CompressorZSTD.h>
#include <pression/data/Registry.h>
#include <unordered_map>
#include <utility>

//#define KEYV_MEMCACHED_COMPRESSION

namespace keyv
{
class Memcached;
namespace
{
lunchbox::PluginRegisterer<Memcached> registerer;

memcached_st* _getInstance(const servus::URI& uri)
{
    const std::string& host = uri.getHost();
    const int16_t port = uri.getPort() ? uri.getPort() : 11211;
    memcached_st* instance = memcached_create(0);
    size_t nServers = 1;

    if (uri.getHost().empty())
    {
        const char* servers = ::getenv("MEMCACHED_SERVERS");
        if (servers)
        {
            instance->server_failure_limit = 10;
            std::string data = servers;
            while (!data.empty())
            {
                const size_t comma = data.find(',');
                const std::string& server = data.substr(0, comma);

                const size_t colon = server.find(':');
                if (colon == std::string::npos)
                    memcached_server_add(instance, server.c_str(), port);
                else
                    memcached_server_add(instance,
                                         server.substr(0, colon).c_str(),
                                         std::stoi(server.substr(colon + 1)));

                if (comma == std::string::npos)
                    data.clear();
                else
                    data = data.substr(comma + 1);
                ++nServers;
            }
            --nServers; // adjust for pre-set '1' in initialization
        }
        else
            memcached_server_add(instance, "127.0.0.1", port);
    }
    else
        memcached_server_add(instance, host.c_str(), port);

    // delivers better distribution and allows servers to be added to the
    // cluster with minimal cache losses
    memcached_behavior_set(instance, MEMCACHED_BEHAVIOR_DISTRIBUTION,
                           MEMCACHED_DISTRIBUTION_CONSISTENT);
    memcached_behavior_set(instance, MEMCACHED_BEHAVIOR_NO_BLOCK, 1); // nop?
    // fire-and-forget writes
    memcached_behavior_set(instance, MEMCACHED_BEHAVIOR_NOREPLY, 1);
    // buffer sizes
    memcached_behavior_set(instance, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE,
                           LB_1MB * nServers);
    memcached_behavior_set(instance, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE,
                           LB_1MB * nServers);
    return instance;
}

lunchbox::uint128_t _generateNamespace(const servus::URI& uri)
{
    const auto& path = uri.getPath();
    if (path.empty())
        return lunchbox::uint128_t();
    // OPT: hash path to limit string size used as key
    return lunchbox::make_uint128(path);
}
}

class Memcached : public Plugin
{
public:
    explicit Memcached(const servus::URI& uri)
        : _instance(_getInstance(uri))
        , _namespace(_generateNamespace(uri))
        , _lastError(MEMCACHED_SUCCESS)
    {
        if (!_instance)
            throw std::runtime_error(std::string("Open of ") +
                                     std::to_string(uri) + " failed");
    }

    virtual ~Memcached() { memcached_free(_instance); }
    static bool handles(const servus::URI& uri)
    {
        return uri.getScheme() == "memcached";
    }

    static std::string getDescription()
    {
        return "memcached://[host][:port][/namespace]";
    }

    bool insert(const std::string& key, const void* data,
                const size_t size) final
    {
        const std::string& hash = _hash(key);
#ifdef KEYV_MEMCACHED_COMPRESSION
        const lunchbox::Bufferb compressed{_compress(data, size)};
        const memcached_return_t ret =
            memcached_set(_instance, hash.c_str(), hash.length(),
                          (char*)compressed.getData(), compressed.getSize(),
                          (time_t)0, (uint32_t)0);
#else
        const memcached_return_t ret =
            memcached_set(_instance, hash.c_str(), hash.length(),
                          (const char*)data, size, (time_t)0, (uint32_t)0);
#endif

        if (ret != MEMCACHED_SUCCESS && _lastError != ret)
        {
            LBWARN << "memcached_set failed: "
                   << memcached_strerror(_instance, ret) << std::endl;
            _lastError = ret;
        }
        return ret == MEMCACHED_SUCCESS;
    }

    std::string operator[](const std::string& key) const final
    {
        const std::string& hash = _hash(key);
        size_t size = 0;
        uint32_t flags = 0;
        memcached_return_t ret = MEMCACHED_SUCCESS;
        char* data = memcached_get(_instance, hash.c_str(), hash.length(),
                                   &size, &flags, &ret);
        if (ret != MEMCACHED_SUCCESS)
            return std::string();

        std::string value;
#ifdef KEYV_MEMCACHED_COMPRESSION
        const uint64_t fullSize = *reinterpret_cast<uint64_t*>(data);
        value.resize(fullSize);
        _decompress((uint8_t*)value.data(), fullSize, (const uint8_t*)data,
                    size);
#else
        value.assign(data, data + size);
#endif
        ::free(data);
        return value;
    }

    void takeValues(const Strings& keys, const ValueFunc& func) const final
    {
        const auto decompress = [this](memcached_result_st* fetched,
                                       const size_t size) {
            char* data = memcached_result_take_value(fetched);
            if (!data)
                return std::make_pair<char*, size_t>(nullptr, 0);

#ifdef KEYV_MEMCACHED_COMPRESSION
            const uint64_t fullSize = *reinterpret_cast<const uint64_t*>(data);
            char* decompressed = (char*)::malloc(fullSize);
            _decompress((uint8_t*)decompressed, fullSize, (uint8_t*)data, size);
            ::free(data);
            return std::pair<char*, size_t>({decompressed, fullSize});
#else
            return std::pair<char*, size_t>({data, size});
#endif
        };
        _multiGet(keys, func, decompress);
    }

    void getValues(const Strings& keys, const ConstValueFunc& func) const final
    {
        lunchbox::Bufferb decompressed;
        const auto decompress = [&](memcached_result_st* fetched,
                                    const size_t size) {
            const char* data = memcached_result_value(fetched);
            if (!data)
                return std::make_pair<const char*, size_t>(nullptr, 0);

#ifdef KEYV_MEMCACHED_COMPRESSION
            const uint64_t fullSize = *reinterpret_cast<const uint64_t*>(data);
            decompressed.resize(fullSize);
            _decompress(decompressed.getData(), fullSize, (const uint8_t*)data,
                        size);
            return std::pair<const char*, size_t>((char*)decompressed.getData(),
                                                  fullSize);
#else
            return std::pair<const char*, size_t>({data, size});
#endif
        };
        _multiGet(keys, func, decompress);
    }

    bool flush() final
    {
        return memcached_flush_buffers(_instance) == MEMCACHED_SUCCESS;
    }

    void erase(const std::string& key) final
    {
        const std::string& hash = _hash(key);
        memcached_delete(_instance, hash.c_str(), hash.length(), 0);
    }

private:
    template <typename F, typename T>
    void _multiGet(const Strings& keys, const F& func, const T& getFunc) const
    {
        std::vector<const char*> keysArray;
        std::vector<size_t> keyLengths;
        keysArray.reserve(keys.size());
        keyLengths.reserve(keys.size());
        std::unordered_map<std::string, std::string> hashes;
        Strings hashCopy;
        hashCopy.reserve(keys.size());

        for (const auto& key : keys)
        {
            const std::string& hash = _hash(key);
            hashes[hash] = key;
            hashCopy.push_back(hash);
            keysArray.push_back(hashCopy.back().c_str());
            keyLengths.push_back(hashCopy.back().length());
        }

        memcached_mget(_instance, keysArray.data(), keyLengths.data(),
                       keysArray.size());

        memcached_return ret = MEMCACHED_SUCCESS;
        memcached_result_st* fetched;
        while ((fetched = memcached_fetch_result(_instance, nullptr, &ret)))
        {
            if (ret == MEMCACHED_SUCCESS)
            {
                const std::string key(memcached_result_key_value(fetched),
                                      memcached_result_key_length(fetched));
                const size_t size = memcached_result_length(fetched);
                const auto& result = getFunc(fetched, size);
                func(hashes[key], result.first, result.second);
            }
            memcached_result_free(fetched);
        }
    }

#ifdef KEYV_MEMCACHED_COMPRESSION
    lunchbox::Bufferb _compress(const void* data, const size_t size) const
    {
        lunchbox::Bufferb compressed;
        const auto& results = _compressor.compress((const uint8_t*)data, size);
        compressed.resize(sizeof(uint64_t) + // uncompressed size
                          results.size() * sizeof(uint64_t) +    // chunk sizes
                          pression::data::getDataSize(results)); // chunks
        uint8_t* ptr = compressed.getData();

        // uncompressed size
        *reinterpret_cast<uint64_t*>(ptr) = size;
        ptr += sizeof(uint64_t);

        for (const auto& result : results)
        {
            // chunk size
            *reinterpret_cast<uint64_t*>(ptr) = result.getSize();
            ptr += sizeof(uint64_t);
            // chunk
            ::memcpy(ptr, result.getData(), result.getSize());
            ptr += result.getSize();
        }
        return compressed;
    }

    void _decompress(uint8_t* decompressed, const size_t fullSize,
                     const uint8_t* data, const size_t size) const
    {
        std::vector<std::pair<const uint8_t*, size_t>> inputs;
        for (size_t i = sizeof(uint64_t); i < size;
             i += sizeof(uint64_t) + inputs.back().second)
        {
            const size_t chunkSize =
                *reinterpret_cast<const uint64_t*>(data + i);
            inputs.push_back({data + i + sizeof(uint64_t), chunkSize});
        }
        _compressor.decompress(inputs, decompressed, fullSize);
    }

    // memcached has relative strict requirements on keys (no whitespace or
    // control characters, max length). We therefore hash incoming keys and use
    // their string representation. If we compress data, the compressor name is
    // appended to the key to avoid name clashes.
    std::string _hash(const std::string& key) const
    {
        return lunchbox::uint128_t( _namespace +
            servus::make_uint128( key + _compressor.getName( )).getString();
    }
#else
    std::string _hash(const std::string& key) const
    {
        return lunchbox::uint128_t(_namespace + servus::make_uint128(key))
            .getString();
    }
#endif

    memcached_st* const _instance;
    const lunchbox::uint128_t _namespace;
    memcached_return_t _lastError;
#ifdef KEYV_MEMCACHED_COMPRESSION
    mutable pression::data::CompressorSnappy _compressor;
#endif
};
}
