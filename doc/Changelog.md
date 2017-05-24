# Changelog {#Changelog}

# Release 1.1 (24-05-2017)

* [15](https://github.com/BlueBrain/Keyv/pull/15):
  Add keyv::Map::erase()
* [14](https://github.com/BlueBrain/Keyv/pull/14):
  Add link-time plugin support by using lunchbox plugins
* [13](https://github.com/BlueBrain/Keyv/pull/13):
  Change leveldb URI to
  ```leveldb://[/namespace][?store=path_to_leveldb_dir]```
  and memcached URI to
  ```memcached://[host][:port][/namespace]```
* [12](https://github.com/BlueBrain/Keyv/pull/12):
  Remove Map::fetch, superseeded by getValues and takeValues
* [10](https://github.com/BlueBrain/Keyv/pull/10):
  Tune memcached performance
* [9](https://github.com/BlueBrain/Keyv/pull/9):
  Fix reading large amount of data from memcached
  Use faster Snappy as default compressor
* [8](https://github.com/BlueBrain/Keyv/pull/8):
  Add compression for memcached
* [7](https://github.com/BlueBrain/Keyv/pull/7):
  Fix takeValues on OS X or latest memcached

# Release 1.0 (09-12-2016)

* [2](https://github.com/BlueBrain/Keyv/pull/2):
  Import code from lunchbox::PersistentMap
