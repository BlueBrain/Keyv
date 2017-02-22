
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

#include <lunchbox/log.h>
#include <lunchbox/pluginRegisterer.h>

#include <rados/librados.hpp>

#include <boost/filesystem.hpp>

#include <unordered_map>

#include <pwd.h>

namespace keyv
{
class Ceph;

namespace
{
lunchbox::PluginRegisterer<Ceph> registerer;

void _throw(const std::string& reason, const int error)
{
    LBTHROW(std::runtime_error(reason + ": " + ::strerror(-error)));
}
}

class Ceph : public Plugin
{
public:
    Ceph(const servus::URI& uri);

    ~Ceph();

    static bool handles(const servus::URI& uri);
    static std::string getDescription();

    size_t setQueueDepth(const size_t depth) final;

    bool insert(const std::string& key, const void* data,
                const size_t size) final;

    std::string operator[](const std::string& key) const final;

    void takeValues(const Strings& keys, const ValueFunc& func) const final;

    void getValues(const Strings& keys, const ConstValueFunc& func) const final;

    void erase(const std::string& key) final;

    bool flush() final { return _flush(0); }
private:
    librados::Rados _cluster;
    mutable librados::IoCtx _context;
    size_t _maxPendingOps;

    typedef std::unique_ptr<librados::AioCompletion> AioPtr;

    struct AsyncRead
    {
        AsyncRead() {}
        AioPtr op;
        librados::bufferlist bl;
    };

    typedef std::unordered_map<std::string, AsyncRead> ReadMap;
    typedef std::deque<librados::AioCompletion*> Writes;

    mutable ReadMap _reads;
    Writes _writes;

    bool _fetch(const std::string& key, const size_t sizeHint) const;

    bool _flush(const size_t maxPending);

    librados::bufferlist _get(const std::string& key) const;
};

inline Ceph::Ceph(const servus::URI& uri)
    : _maxPendingOps(256)
{
    const auto poolName = uri.getUserinfo();
    const auto userName = "client." + poolName;
    u_int64_t flags = 0;
    int ret = _cluster.init2(userName.c_str(), uri.getHost().c_str(), flags);
    if (ret < 0)
        _throw("Cannot initialize rados cluster", ret);

    static const passwd* pw = getpwuid(getuid());
    static const std::string homeDir(pw->pw_dir);

    auto pos = uri.findQuery("config");
    std::string configFile;
    if (pos != uri.queryEnd())
    {
        configFile = pos->second;
    }
    else {
        configFile = homeDir+"/.ceph/config";
        if(!boost::filesystem::exists(configFile))
        {
            configFile.clear();
        }
    }
    if (!configFile.empty())
    {
        ret = _cluster.conf_read_file(configFile.c_str());
        if (ret < 0)
            _throw("Cannot read ceph config '" + configFile + "'", ret);
    }

    pos = uri.findQuery("keyring");
    std::string keyringFile;
    if (pos != uri.queryEnd())
    {
        keyringFile = pos->second;
    }
    else {
        keyringFile = homeDir+"/.ceph/keyring";
        if(!boost::filesystem::exists(keyringFile))
        {
            keyringFile.clear();
        }
    }
    if (!keyringFile.empty())
    {
        ret = _cluster.conf_set("keyring", keyringFile.c_str());
        if (ret < 0)
        {
            _throw("Cannot read ceph keyring '" + keyringFile + "'", ret);
        }
    }

    ret = _cluster.connect();
    if (ret < 0)
        _throw("Could not connect rados cluster", ret);

    ret = _cluster.ioctx_create(poolName.c_str(), _context);
    if (ret < 0)
        _throw("Could not create io context", ret);
}

inline Ceph::~Ceph()
{
    _context.close();
    _cluster.shutdown();
}

inline bool Ceph::handles(const servus::URI& uri)
{
    return uri.getScheme() == "ceph";
}

inline std::string Ceph::getDescription()
{
    return "ceph://user@cluster?config=path&keyring=path";
}

inline size_t Ceph::setQueueDepth(const size_t depth)
{
    _maxPendingOps = depth;
    return _maxPendingOps;
}

inline bool Ceph::insert(const std::string& key, const void* data,
                         const size_t size)
{
    librados::bufferlist bl;
    bl.append((const char*)data, size);

    if (_maxPendingOps == 0) // sync write
    {
        const int write = _context.write_full(key, bl);
        if (write >= 0)
            return true;

        std::cerr << "Write failed: " << ::strerror(-write) << std::endl;
        return false;
    }

    librados::AioCompletion* op = librados::Rados::aio_create_completion();
    const int write = _context.aio_write_full(key, op, bl);
    if (write < 0)
    {
        std::cerr << "Write failed: " << ::strerror(-write) << std::endl;
        delete op;
        return false;
    }
    _writes.push_back(op);
    return _flush(_maxPendingOps);
}

inline std::string Ceph::operator[](const std::string& key) const
{
    const librados::bufferlist bl = _get(key);
    std::string value;
    if (bl.length() > 0)
        bl.copy(0, bl.length(), value);
    return value;
}

inline void Ceph::takeValues(const lunchbox::Strings& keys,
                             const ValueFunc& func) const
{
    size_t fetchedIndex = 0;
    for (const auto& key : keys)
    {
        // pre-fetch requested values
        while (_reads.size() >= _maxPendingOps && fetchedIndex < keys.size())
        {
            _fetch(keys[fetchedIndex++], 0);
        }

        // get current key
        librados::bufferlist bl = _get(key);
        if (bl.length() == 0)
            continue;

        char* copy = (char*)malloc(bl.length());
        bl.copy(0, bl.length(), copy);
        func(key, copy, bl.length());
    }
}

inline void Ceph::getValues(const lunchbox::Strings& keys,
                            const ConstValueFunc& func) const
{
    size_t fetchedIndex = 0;
    for (const auto& key : keys)
    {
        // pre-fetch requested values
        while (_reads.size() >= _maxPendingOps && fetchedIndex < keys.size())
        {
            _fetch(keys[fetchedIndex++], 0);
        }

        // get current key
        const auto& value = (*this)[key];
        func(key, value.data(), value.size());
    }
}

inline void Ceph::erase(const std::string& key)
{
    const int write = _context.remove(key);
    if (write < 0)
    {
        std::cerr << "Erase failed: " << ::strerror(-write) << std::endl;
    }
}

inline bool Ceph::_fetch(const std::string& key, const size_t sizeHint) const
{
    if (_reads.size() >= _maxPendingOps)
        return true;

    AsyncRead& asyncRead = _reads[key];
    if (asyncRead.op)
        return true; // fetch for key already pending

    asyncRead.op.reset(librados::Rados::aio_create_completion());
    uint64_t size = sizeHint;
    if (size == 0)
    {
        time_t time;
        const int stat = _context.stat(key, &size, &time);
        if (stat < 0 || size == 0)
        {
            std::cerr << "Stat " << key << " failed: " << ::strerror(-stat)
                      << std::endl;
            return false;
        }
    }

    const int read =
        _context.aio_read(key, asyncRead.op.get(), &asyncRead.bl, size, 0);
    if (read >= 0)
        return true;

    std::cerr << "Fetch failed: " << ::strerror(-read) << std::endl;
    return false;
}

inline bool Ceph::_flush(const size_t maxPending)
{
    if (maxPending == 0)
    {
        const int flushAll = _context.aio_flush();

        while (!_writes.empty())
        {
            delete _writes.front();
            _writes.pop_front();
        }
        if (flushAll >= 0)
            return true;

        std::cerr << "Flush all writes failed: " << ::strerror(-flushAll)
                  << std::endl;
        return false;
    }

    bool ok = true;
    while (_writes.size() > maxPending)
    {
        _writes.front()->wait_for_complete();
        const int write = _writes.front()->get_return_value();
        if (write < 0)
        {
            std::cerr << "Finish write failed: " << ::strerror(-write)
                      << std::endl;
            ok = false;
        }
        delete _writes.front();
        _writes.pop_front();
    }
    return ok;
}

inline ceph::bufferlist Ceph::_get(const std::string& key) const
{
    librados::bufferlist bl;
    ReadMap::iterator i = _reads.find(key);
    if (i == _reads.end())
    {
        uint64_t size = 0;
        time_t time;
        const int stat = _context.stat(key, &size, &time);
        if (stat < 0 || size == 0)
        {
            std::cerr << "Stat '" << key << "' failed: " << ::strerror(-stat)
                      << std::endl;
            return std::move(bl);
        }

        const int read = _context.read(key, bl, size, 0);
        if (read < 0)
        {
            std::cerr << "Read '" << key << "' failed: " << ::strerror(-read)
                      << std::endl;
            return bl;
        }
    }
    else
    {
        i->second.op->wait_for_complete();
        const int read = i->second.op->get_return_value();
        if (read < 0)
        {
            std::cerr << "Finish read '" << key
                      << "' failed: " << ::strerror(-read) << std::endl;
            return bl;
        }

        i->second.bl.swap(bl);
        _reads.erase(i);
    }
    return bl;
}
}
