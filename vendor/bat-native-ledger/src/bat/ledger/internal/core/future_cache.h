/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_CACHE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_CACHE_H_

#include <list>
#include <map>
#include <type_traits>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

// Caches operations that return futures. Operations can be keyed and results
// can be cached for a user-specified amount of time.
//
// Example:
//
//   FutureCache<int> cache;
//
//   cache.GetFuture([]() {
//     return Future<int>(42);
//   }).Then(base::BindOnce([](int value) {
//     LOG(INFO) << "Value is: " << value;
//   }));
//
// The value returned from the function object supplied to |GetFuture| can be
// one of:
//
//   - Future<T>
//   - Future<std::pair<T, base::TimeDelta>>
//
// When an |std::pair| is returned, the second value determines the length of
// time that the result will be cached for. Otherwise, the value is not cached
// after it has been completed and subsequent calls to |GetFuture| will
// reexecute the function object.
template <typename T, typename Key = int>
class FutureCache {
 private:
  static_assert(std::is_copy_constructible<T>::value,
                "FutureCache<T> requires that T is copy constructible");

  struct Entry {
    absl::optional<T> value;
    base::Time expires_at;
    std::list<Promise<T>> promises;
  };

 public:
  // Returns a cached Future using a default key. If the cache entry does
  // exist, then the specified function object is executed. The Future
  // returned by the function object is stored as the current cache entry. The
  // |Key| type must be default-constructible.
  template <typename F>
  Future<T> GetFuture(F&& fn) {
    return GetFuture(Key(), std::forward<F>(fn));
  }

  // Returns a cached Future for the specified key. If the cache entry does
  // exist, then the specified function object is executed. The Future
  // returned by the function object is stored as the current cache entry for
  // the given key.
  template <typename F>
  Future<T> GetFuture(Key key, F fn) {
    MaybePurgeStaleEntries();

    Promise<T> promise;
    Future<T> future = promise.GetFuture();

    auto iter = entries_.find(key);
    if (iter == entries_.end()) {
      auto pair = entries_.emplace(key, Entry());
      iter = pair.first;
    } else {
      Entry& entry = iter->second;
      if (entry.value && !EntryIsStale(entry)) {
        promise.SetValue(*entry.value);
        return future;
      }
    }

    Entry& entry = iter->second;
    entry.promises.push_back(std::move(promise));
    if (entry.promises.size() == 1) {
      auto init_future = fn();
      using ValueType = typename decltype(init_future)::ValueType;
      init_future.Then(base::BindOnce(&FutureCache::OnComplete<ValueType>,
                                      weak_factory_.GetWeakPtr(), key));
    }

    return future;
  }

 private:
  template <typename U>
  void OnComplete(Key key, U value) {
    SetValue(key, std::move(value));
  }

  void SetValue(Key key, T value) {
    SetValue(key, std::make_pair(std::move(value), base::TimeDelta()));
  }

  void SetValue(Key key, std::pair<T, base::TimeDelta> pair) {
    auto iter = entries_.find(key);
    DCHECK(iter != entries_.end());

    Entry& entry = iter->second;
    entry.value = std::move(pair.first);
    entry.expires_at = base::Time::Now() + pair.second;

    std::list<Promise<T>> promises = std::move(entry.promises);
    for (auto& promise : promises) {
      promise.SetValue(*entry.value);
    }
  }

  void MaybePurgeStaleEntries() {
    base::Time now = base::Time::Now();
    if (last_purge_ + base::Seconds(30) < now)
      return;

    last_purge_ = now;

    for (auto iter = entries_.begin(); iter != entries_.end();) {
      if (EntryIsStale(iter->second)) {
        iter = entries_.erase(iter);
      } else {
        ++iter;
      }
    }
  }

  bool EntryIsStale(const Entry& entry) {
    return entry.value && entry.expires_at <= base::Time::Now();
  }

  std::map<Key, Entry> entries_;
  base::Time last_purge_ = base::Time::Now();
  base::WeakPtrFactory<FutureCache> weak_factory_{this};
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_CACHE_H_
