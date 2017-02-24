
/* Copyright (c) 2014-2017, Stefan.Eilemann@epfl.ch
 *
 * This file is part of Keyv <https://github.com/BlueBrain/Keyv>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Eyescale Software GmbH nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define TEST_RUNTIME 600 // seconds

#include <keyv/Map.h>

#include <lunchbox/clock.h>
#include <lunchbox/os.h>
#include <lunchbox/rng.h>
#include <lunchbox/test.h>
#include <lunchbox/threadPool.h>

#ifdef KEYV_USE_LEVELDB
#include <leveldb/db.h>
#endif
#include <boost/format.hpp>

#include <stdexcept>

#define MAX_SIZE (1024 * 256)

using keyv::Map;

const int ints[] = {17, 53, 42, 65535, 32768};
const size_t numInts = sizeof(ints) / sizeof(int);
const int64_t loopTime = 1000;

template <class T>
void insertVector(Map& map)
{
    std::vector<T> vector;
    for (size_t i = 0; i < numInts; ++i)
        vector.push_back(T(ints[i]));
    TEST(map.insert(typeid(vector).name(), vector));
}

template <class T>
void readVector(const Map& map)
{
    const std::vector<T>& vector = map.getVector<T>(typeid(vector).name());
    TESTINFO(vector.size() == numInts, vector.size() << " != " << numInts);
    for (size_t i = 0; i < numInts; ++i)
        TEST(vector[i] == T(ints[i]));
}

template <class T>
void insertVector(Map& map, const size_t elems)
{
    std::vector<T> vector;
    for (size_t i = 0; i < elems; ++i)
        vector.push_back(T(i));
    TEST(map.insert(std::string("bulk") + typeid(vector).name(), vector));
}

template <class T>
void readVector(const Map& map, const size_t elems)
{
    const std::vector<T>& vector =
        map.getVector<T>(std::string("bulk") + typeid(vector).name());
    TESTINFO(vector.size() == elems, vector.size() << " != " << elems);
    for (size_t i = 0; i < numInts; ++i)
        TESTINFO(vector[i] == T(i), vector[i] << " != " << i);
}

void read(const Map& map)
{
    const std::set<uint32_t>& bigSet =
        map.getSet<uint32_t>("std::set< uint32_t >");
    TEST(bigSet.size() == 1000);
    for (uint32_t i = 1; i <= 1000; ++i)
        TEST(bigSet.find(i) != bigSet.end());

    TEST(map["foo"] == "bar");
    TEST(map["bar"].empty());
    TEST(map.get<bool>("bValue") == true);
    TEST(map.get<int>("iValue") == 42);

    readVector<int>(map);
    readVector<uint16_t>(map);

    const std::set<int>& set = map.getSet<int>("std::set< int >");
    TESTINFO(set.size() == numInts, set.size() << " != " << numInts);
    for (size_t i = 0; i < numInts; ++i)
        TESTINFO(set.find(ints[i]) != set.end(), ints[i]
                                                     << " not found in set");
}

void read(const std::string& uriStr)
{
    const servus::URI uri(uriStr);
    Map map(uri);
    read(map);
}

bool testAvailable(const std::string& uriStr)
{
    try
    {
        const servus::URI uri(uriStr);
        Map map(uri);
        if (!map.insert("foo", "bar"))
            return false;
        return map["foo"] == "bar";
    }
    catch (...)
    {
        return false;
    }
}

void setup(const std::string& uriStr)
{
    const servus::URI uri(uriStr);
    Map map(uri);
    TEST(map.insert("foo", "bar"));
    TESTINFO(map["foo"] == "bar", map["foo"] << " length "
                                             << map["foo"].length());
    TEST(map["bar"].empty());

    TEST(map.insert("the quick brown fox", "jumped over something"));
    TESTINFO(map["the quick brown fox"] == "jumped over something",
             map["the quick brown fox"]);

    TEST(map.insert("hans", std::string("dampf")));
    TESTINFO(map["hans"] == "dampf", map["hans"]);

    const bool bValue = true;
    TEST(map.insert("bValue", bValue));
    TEST(map.get<bool>("bValue") == bValue);

    const int iValue = 42;
    TEST(map.insert("iValue", iValue));
    TEST(map.get<int>("iValue") == iValue);

    TEST(map.insert("coffee", 0xC0FFEE));
    map.setByteswap(true);
    TEST(map.get<unsigned>("coffee") == 0xEEFFC000u);
    map.setByteswap(false);
    TEST(map.get<int>("coffee") == 0xC0FFEE);

    insertVector<int>(map);
    insertVector<uint16_t>(map);
    readVector<int>(map);
    readVector<uint16_t>(map);

    insertVector<int>(map, LB_128KB);
    insertVector<uint16_t>(map, LB_128KB);
    readVector<int>(map, LB_128KB);
    readVector<uint16_t>(map, LB_128KB);

    std::set<int> set(ints, ints + numInts);
    TEST(map.insert("std::set< int >", set));

    std::set<uint32_t> bigSet;
    for (uint32_t i = 1; i <= 1000; ++i)
        bigSet.insert(i);
    TEST(map.insert("std::set< uint32_t >", bigSet));

    const lunchbox::Strings keys = {"hans", "coffee"};
    size_t numResults = 0;
    map.takeValues(keys, [&](const std::string& key, char* data,
                             const size_t size) {
        TESTINFO(std::find(keys.begin(), keys.end(), key) != keys.end(), key);
        TEST(data);
        TESTINFO(size > 0, key << " in " << uriStr);
        ++numResults;
        free(data);
    });
    TEST(numResults == keys.size());

    numResults = 0;
    map.getValues(keys, [&](const std::string& key, const char* data,
                            const size_t size) {
        TEST(std::find(keys.begin(), keys.end(), key) != keys.end());
        TEST(data);
        TEST(size > 0);
        ++numResults;
    });
    TEST(numResults == keys.size());

    const std::string random = servus::make_UUID().getString();
    TEST(map.insert(random, "foobar"));
    TESTINFO(map[random] == "foobar", map[random]);
    map.erase(random);
    TEST(map[random].empty());
}

void benchmark(const std::string& uriStr, const uint64_t queueDepth,
               const size_t valueSize)
{
    static std::string lastURI;
    if (uriStr != lastURI)
    {
        std::cout
            << " " << uriStr << std::endl
            << " depth,     size,  writes/s,     MB/s,  reads/s,      MB/s"
            << std::endl;
        lastURI = uriStr;
    }

    // cppcheck-suppress zerodivcond
    std::cout << boost::format("%6i, %8i,") % queueDepth % valueSize
              << std::flush;

    const servus::URI uri(uriStr);
    Map map(uri);
    map.setQueueDepth(queueDepth);

    // Prepare keys and value
    lunchbox::Strings keys;
    keys.resize(queueDepth + 1);
    for (uint64_t i = 0; i <= queueDepth; ++i)
        keys[i].assign(reinterpret_cast<char*>(&i), 8);

    std::string value(valueSize, '*');
    lunchbox::RNG rng;
    for (size_t i = 0; i < valueSize; ++i)
        value[i] = rng.get<char>();

    // write performance
    lunchbox::Clock clock;
    uint64_t i = 0;
    while (clock.getTime64() < loopTime || i <= queueDepth)
    {
        map.insert(keys[i % (queueDepth + 1)], value);
        ++i;
    }
    map.flush();
    float time = clock.getTimef() / 1000.f;
    const uint64_t wOps = i;

    // cppcheck-suppress zerodivcond
    std::cout << boost::format("%9.2f, %9.2f,") % (wOps / time) %
                     (wOps / 1024.f / 1024.f * valueSize / time)
              << std::flush;

    // read performance
    clock.reset();
    if (queueDepth == 0) // sync read
    {
        for (i = 0; i < wOps && clock.getTime64() < loopTime; ++i) // read keys
            map[keys[i % (queueDepth + 1)]];
    }
    else // async read
    {
        const auto getValue = [](const std::string&, const char*, size_t) {};
        for (i = 0; i < wOps && clock.getTime64() < loopTime; i += queueDepth)
        {
            lunchbox::Strings subKeys;
            subKeys.reserve(queueDepth);
            for (size_t j = i; j < i + queueDepth; ++j)
                subKeys.push_back(keys[j % (queueDepth + 1)]);
            map.getValues(subKeys, getValue);
        }
    }
    time = clock.getTimef() / 1000.f;

    std::cout << boost::format("%9.2f, %9.2f") % (i / time) %
                     (i / 1024.f / 1024.f * valueSize / time)
              << std::endl;

    // try to make sure there's nothing outstanding if we messed up in our test
    map.flush();
}

void benchmarkMultithreaded(const std::string& uriStr, const size_t threadCount,
                            const size_t valueSize)
{
    static std::string lastURI;
    if (uriStr != lastURI)
    {
        std::cout
            << " #thr ,     size,  writes/s,     MB/s,  reads/s,      MB/s"
            << std::endl;
        lastURI = uriStr;
    }

    std::vector<Map> maps;

    const servus::URI uri(uriStr);

    for (size_t i = 0; i < threadCount; ++i)
    {
        maps.emplace_back(uri);
    }

    std::string value(valueSize, '*');

    lunchbox::RNG rng;
    for (size_t i = 0; i < valueSize; ++i)
        value[i] = rng.get<char>();

    auto writeTask = [&](size_t id) {

        std::pair<size_t, size_t> key;
        key.first = id;

        key.second = valueSize;
        std::string keyStr;
        keyStr.assign(reinterpret_cast<char*>(&key), sizeof(key));
        maps[id].insert(keyStr, value);
        maps[id].flush();
    };

    auto readTask = [&](size_t id) {

        std::pair<size_t, size_t> key;
        key.first = id;
        key.second = valueSize;
        std::string keyStr;
        keyStr.assign(reinterpret_cast<char*>(&key), sizeof(key));
        maps[id][keyStr];

        maps[id].flush();
    };

    lunchbox::ThreadPool threadPool{threadCount};

    std::vector<std::future<void>> status;
    lunchbox::Clock clock;

    for (size_t i = 0; i < threadCount; ++i)
    {
        status.push_back(threadPool.post(std::bind(writeTask, i)));
    }

    for (auto& f : status)
        f.get();
    status.clear();

    float writeTime = clock.getTimef() / 1000.f;

    clock.reset();

    for (size_t i = 0; i < threadCount; ++i)
    {
        status.push_back(threadPool.post(std::bind(readTask, i)));
    }

    for (auto& f : status)
        f.get();

    float readTime = clock.getTimef() / 1000.f;

    float dataSizeMB = valueSize / 1024.f / 1024.f;

    // threads,     size,  writes/s,     MB/s,  reads/s,      MB/s
    std::cout << boost::format("%6i, %8i,%9.2f, %9.2f,%9.2f, %9.2f") %
                     threadCount % valueSize % (threadCount / writeTime) %
                     (dataSizeMB * threadCount / writeTime) %
                     (threadCount / readTime) %
                     (dataSizeMB * threadCount / readTime)
              << std::endl;
}

void testGenericFailures()
{
    try
    {
        setup("foobar://");
    }
    catch (const std::runtime_error&)
    {
        return;
    }
    TESTINFO(false, "Missing exception");
}

void testLevelDBFailures()
{
#ifdef KEYV_USE_LEVELDB
    try
    {
        setup("leveldb://?store=/doesnotexist/deadbeef/coffee");
    }
    catch (const std::runtime_error&)
    {
        return;
    }
    TESTINFO(false, "Missing exception");
#endif
}

void testCephFailures()
{
#ifdef KEYV_USE_RADOS

    bool hasException = false;
    try
    {
        setup(
            "ceph://foo@bar?config=/doesnotexist/deadbeef/coffee&keyring=/a/b/"
            "c");
    }
    catch (const std::runtime_error&)
    {
        hasException = true;
    }
    TESTINFO(hasException, "Missing exception");

    hasException = false;
    try
    {
        std::string user = "vizks-nvme";
        std::string cluster = "cephx";

        servus::URI uri;
        uri.setScheme("ceph");
        uri.setUserInfo(user);
        uri.setHost(cluster);
        uri.addQuery("config", "");
        uri.addQuery("keyring", "");

        setup(std::to_string(uri));
    }
    catch (const std::runtime_error&)
    {
        hasException = true;
    }

    TESTINFO(hasException, "Missing exception");
#endif
}

size_t dup(const size_t value)
{
    return value == 0 ? 1 : value << 1;
}

struct TestSpec
{
    TestSpec(const std::string& uri_, const size_t depth_, const size_t size_,
             const size_t threadCount_ = 1)
        : uri(uri_)
        , depth(depth_)
        , size(size_)
        , threadCount(threadCount_)
    {
    }

    std::string uri;
    size_t depth;
    size_t size;
    size_t threadCount = 1;
};

int main(const int argc, char* argv[])
{
    if (argc == 4)
    {
        benchmark(argv[1], atoi(argv[2]), atoi(argv[3]));
        return EXIT_SUCCESS;
    }

    const bool perfTest =
        std::string(argv[0]).find("perf-") != std::string::npos;

    typedef std::vector<TestSpec> TestSpecs;
    TestSpecs tests;
#ifdef KEYV_USE_LEVELDB
    tests.push_back(TestSpec("", 0, MAX_SIZE));
    tests.push_back(TestSpec("leveldb://", 0, MAX_SIZE));
    tests.push_back(TestSpec("leveldb://?store=keyvMap2.leveldb", 0, MAX_SIZE));
#endif
#ifdef KEYV_USE_LIBMEMCACHED
    if (testAvailable("memcached://"))
        tests.push_back(TestSpec("memcached://", 65536, MAX_SIZE));
#endif
#ifdef KEYV_USE_RADOS
    std::string config =
        "[global]\n"
        "  auth cluster required = cephx\n"
        "  auth service required = cephx\n"
        "  auth client required = cephx\n"
        "  mon initial members = bbpocn01\n"
        "  keyring = vizks-nvme.keyring\n"
        "[mon.bbpocn01]\n"
        "  host = bbpocn01\n"
        "  mon addr = 10.80.11.11:6789\n"
        "[mon.bbpocn02]\n"
        "  host = bbpocn02\n"
        "  mon addr = 10.80.11.12:6789\n"
        "[mon.bbpocn03]\n"
        "  host = bbpocn03\n"
        "  mon addr = 10.80.11.13:6789\n"
        "[mon.bbpocn04]\n"
        "  host = bbpocn04\n"
        "  mon addr = 10.80.11.14:6789\n";

    std::string keyring =
        "[client.vizks-nvme]\n"
        "  key = AQBltNVWEYqKIxAAgUe2jN5dSRFioQ4Tko4JnQ==\n";

    std::string configFilePath =
        "/tmp/" + servus::make_UUID().getString() + ".conf";
    std::string keyringFilePath =
        "/tmp/" + servus::make_UUID().getString() + ".keyring";

    std::ofstream configFile(configFilePath);
    configFile << config;
    configFile.close();

    std::ofstream keyringFile(keyringFilePath);
    keyringFile << keyring;
    keyringFile.close();

    std::string user = "vizks-nvme";
    std::string cluster = "cephx";

    servus::URI uri;
    uri.setScheme("ceph");
    uri.setUserInfo(user);
    uri.setHost(cluster);
    uri.addQuery("config", configFilePath);
    uri.addQuery("keyring", keyringFilePath);

    tests.push_back(TestSpec(std::to_string(uri), 0, MAX_SIZE, 8));
#endif

    try
    {
        while (!tests.empty())
        {
            const TestSpec test = tests.back();
            tests.pop_back();

            setup(test.uri);
            read(test.uri);
            if (perfTest)
            {
                for (size_t i = 1; i <= test.size; i = i << 2)
                    benchmark(test.uri, test.depth, i);
                for (size_t i = 0; i <= test.depth; i = dup(i))
                    benchmark(test.uri, i, 1024);

                if (test.threadCount > 1)
                {
                    for (size_t threadCount = 1;
                         threadCount <= test.threadCount; ++threadCount)
                        for (size_t s = 1; s <= test.size; s = s << 2)
                            benchmarkMultithreaded(test.uri, threadCount, s);
                }
            }
        }
    }
#ifdef KEYV_USE_LEVELDB
    catch (const leveldb::Status& status)
    {
        TESTINFO(!"exception", status.ToString());
    }
#endif
    catch (const std::runtime_error& error)
    {
        TESTINFO(!"exception", error.what());
    }

    testGenericFailures();
    testLevelDBFailures();
    testCephFailures();

    return EXIT_SUCCESS;
}
