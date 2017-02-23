
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

    bool insert(const std::string& key, const void* data,
                const size_t size) final;

    std::string operator[](const std::string& key) const final;

    void takeValues(const Strings& keys, const ValueFunc& func) const final;

    void getValues(const Strings& keys, const ConstValueFunc& func) const final;

    void erase(const std::string& key) final;

    bool flush() final { return true; }
private:
    librados::Rados _cluster;
    mutable librados::IoCtx _context;
    std::string _storeName;

    using IOMap = std::map<std::string, librados::bufferlist>;
};

inline Ceph::Ceph(const servus::URI& uri)
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
    else
    {
        configFile = homeDir + "/.ceph/config";
        if (!boost::filesystem::exists(configFile))
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
    else
    {
        keyringFile = homeDir + "/.ceph/keyring";
        if (!boost::filesystem::exists(keyringFile))
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

    pos = uri.findQuery("store");
    _storeName = (pos == uri.queryEnd())
                     ? (std::string("keyvMap.") + pw->pw_name)
                     : pos->second;
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
    return "ceph://user@cluster?store=storeName&config=path&keyring=path";
}

inline bool Ceph::insert(const std::string& key, const void* data,
                         const size_t size)
{
    librados::bufferlist bl;
    bl.append((const char*)data, size);

    int ret = _context.omap_set(_storeName, {{key, std::move(bl)}});
    if (ret < 0)
    {
        std::cerr << "Write failed: " << ::strerror(-ret) << std::endl;
        return false;
    }
    return true;
}

inline std::string Ceph::operator[](const std::string& key) const
{
    IOMap map;
    int ret = _context.omap_get_vals_by_keys(_storeName, {key}, &map);
    if (ret < 0)
    {
        std::cerr << "Get failed: " << ::strerror(-ret) << std::endl;
        return std::string();
    }

    auto pos = map.find(key);
    if (pos == map.end())
    {
        return std::string();
    }
    return pos->second.to_str();
}

inline void Ceph::takeValues(const lunchbox::Strings& keys,
                             const ValueFunc& func) const
{
    IOMap map;
    int ret = _context.omap_get_vals_by_keys(
        _storeName, std::set<std::string>(keys.begin(), keys.end()), &map);
    if (ret < 0)
    {
        std::cerr << "Take failed: " << ::strerror(-ret) << std::endl;
        return;
    }

    for (const auto& pair : map)
    {
        librados::bufferlist bl = pair.second;
        if (bl.length() == 0)
            continue;

        char* copy = (char*)malloc(bl.length());
        std::copy(bl.begin(), bl.end(), copy);
        func(pair.first, copy, bl.length());
    }
}

inline void Ceph::getValues(const lunchbox::Strings& keys,
                            const ConstValueFunc& func) const
{
    IOMap map;
    int ret = _context.omap_get_vals_by_keys(
        _storeName, std::set<std::string>(keys.begin(), keys.end()), &map);
    if (ret < 0)
    {
        std::cerr << "Get failed: " << ::strerror(-ret) << std::endl;
        return;
    }

    for (const auto& pair : map)
    {
        if (pair.second.length() == 0)
            continue;
        std::string data = pair.second.to_str();
        func(pair.first, data.c_str(), pair.second.length());
    }
}

inline void Ceph::erase(const std::string& key)
{
    const int ret = _context.omap_rm_keys(_storeName, {key});
    if (ret < 0)
    {
        std::cerr << "Erase failed: " << ::strerror(-ret) << std::endl;
    }
}
}
