/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_RECENTLY_USED_CACHE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_RECENTLY_USED_CACHE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "base/synchronization/lock.h"

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
    base::AutoLock create(lock_);

    data_[key] = value;
    // https://github.com/brave/brave-browser/issues/3193
    // std::string old = keysByAge.oldest();
    // if (!old.empty()) {
    //   keysByAge.data.erase(old);
    // }
    // keysByAge[key] = value;
  }

  bool get(const std::string& key, T* value) {
    base::AutoLock create(lock_);

    auto search = data_.find(key);
    if (search != data_.end()) {
      *value = search->second;
      return true;
    }
    return false;
  }

  void remove(const std::string& key) {
    data_.erase(key);
  }

  void clear() {
    data_.clear();
    // https://github.com/brave/brave-browser/issues/3193
    // keysByAge.clear();
  }

 private:
  std::unordered_map<std::string, T> data_;
  base::Lock lock_;
  RingBuffer<T> keysByAge;
};

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_RECENTLY_USED_CACHE_H_
