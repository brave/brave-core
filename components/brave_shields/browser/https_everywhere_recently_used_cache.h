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

template <class T>
class FixedSizeAgeQueue {
 public:
  explicit FixedSizeAgeQueue(size_t max_size) : max_size_(max_size) {}

  T add(const T& value) {
    T t;
    // If we already have this value, just mark it the youngest.
    if (!make_youngest(value)) {
      // Check if at the size limit and if so pop the oldest value.
      if (data_.size() == max_size_) {
        t = data_.front();
        data_.pop_front();
      }
      data_.push_back(value);
    }
    return t;
  }

  bool make_youngest(const T& value){
    auto it = std::find(data_.begin(), data_.end(), value);
    if (it != data_.end()) {
      std::rotate(it, std::next(it), data_.end());
      return true;
    }
    return false;
  }

  void erase(const T& value) {
    auto it = std::find(data_.begin(), data_.end(), value);
    if (it != data_.end())
      data_.erase(it);
  }

  void clear() { data_.clear(); }

 private:
  size_t max_size_;
  std::deque<T> data_;
};

template <class T> class HTTPSERecentlyUsedCache {
 public:
  explicit HTTPSERecentlyUsedCache(size_t size = 100) : keys_by_age_(size) {}

  void add(const std::string& key, const T& value) {
    base::AutoLock create(lock_);
    data_[key] = value;
    T t = keys_by_age_.add(key);
    if (t != T())
      data_.erase(t);
  }

  bool get(const std::string& key, T* value) {
    base::AutoLock create(lock_);
    auto search = data_.find(key);
    if (search != data_.end()) {
      *value = search->second;
      keys_by_age_.make_youngest(key);
      return true;
    }
    return false;
  }

  void remove(const std::string& key) {
    base::AutoLock lock(lock_);
    data_.erase(key);
    keys_by_age_.erase(key);
  }

  void clear() {
    base::AutoLock lock(lock_);
    data_.clear();
    keys_by_age_.clear();
  }

 private:
  std::unordered_map<std::string, T> data_;
  base::Lock lock_;
  FixedSizeAgeQueue<T> keys_by_age_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_RECENTLY_USED_CACHE_H_
