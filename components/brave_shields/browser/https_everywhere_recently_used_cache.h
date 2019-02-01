/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_RECENTLY_USED_CACHE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_RECENTLY_USED_CACHE_H_

#include <unordered_map>
#include <string>
#include <vector>

template <class T> class RingBuffer {
 public:
  explicit RingBuffer(int fixedSize) : count(fixedSize), data(count) {}

  const T& at(int i) {
    return data[(currentIdx - (i % count) + count) % count];
  }

  void add(const T& newValue) {
    currentIdx = (currentIdx + 1) % count;
    data[currentIdx] = newValue;
  }

  T oldest() {
    return data[(currentIdx + 1) % count];
  }

  void clear() {
    data = std::vector<T>(count);
  }

 private:
  int currentIdx = 0;
  int count;
  std::vector<T> data;
};

template <class T> class HTTPSERecentlyUsedCache {
 public:
  explicit HTTPSERecentlyUsedCache(unsigned int size = 100) : keysByAge(size) {}

  void add(const std::string& key, const T& value) {
    std::string old = keysByAge.oldest();
    if (!old.empty()) {
      keysByAge.data.erase(old);
    }
    keysByAge[key] = value;
  }

  void clear() {
    data.clear();
    keysByAge.clear();
  }

  std::unordered_map<std::string, T> data;

 private:
  RingBuffer<T> keysByAge;
};

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_RECENTLY_USED_CACHE_H_
