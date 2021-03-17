/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_JOIN_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_JOIN_H_

#include <tuple>
#include <utility>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "bat/ledger/internal/core/future.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {

template <typename... Args>
class FutureJoin : public base::RefCounted<FutureJoin<Args...>> {
 public:
  using Resolver = typename Future<std::tuple<Args...>>::Resolver;

  explicit FutureJoin(Resolver resolver) : resolver_(resolver) {}

  void AddFutures(Future<Args>... futures) {
    DCHECK_EQ(remaining_, sizeof...(Args));
    DCHECK(!started_);
    if (!started_) {
      started_ = true;
      AddHandlers<0>(std::move(futures)...);
    }
  }

 private:
  friend class base::RefCounted<FutureJoin>;

  ~FutureJoin() = default;

  template <size_t Index, typename T, typename... Rest>
  void AddHandlers(Future<T> future, Rest... rest) {
    future.Then(base::BindOnce(&FutureJoin::OnComplete<Index, T>, this));
    AddHandlers<Index + 1>(std::move(rest)...);
  }

  template <size_t Index>
  void AddHandlers() {}

  template <size_t Index, typename T>
  void OnComplete(T value) {
    std::get<Index>(optionals_) = std::move(value);
    DCHECK_GT(remaining_, static_cast<size_t>(0));
    if (--remaining_ == 0) {
      Complete(std::make_index_sequence<sizeof...(Args)>{});
    }
  }

  template <size_t... Indexes>
  void Complete(std::index_sequence<Indexes...>) {
    DCHECK_EQ(remaining_, static_cast<size_t>(0));
    resolver_.Complete(
        std::make_tuple(std::move(*std::get<Indexes>(optionals_))...));
  }

  Resolver resolver_;
  std::tuple<absl::optional<Args>...> optionals_;
  size_t remaining_ = sizeof...(Args);
  bool started_ = false;
};

template <typename T>
class FutureVectorJoin : public base::RefCounted<FutureVectorJoin<T>> {
 public:
  using Resolver = typename Future<std::vector<T>>::Resolver;

  explicit FutureVectorJoin(Resolver resolver) : resolver_(resolver) {}

  void AddFutures(std::vector<Future<T>>&& futures) {
    DCHECK(!started_);
    if (!started_) {
      started_ = true;
      remaining_ = futures.size();
      for (size_t i = 0; i < futures.size(); ++i) {
        optionals_.push_back({});
        futures[i].Then(base::BindOnce(&FutureVectorJoin::OnComplete, this, i));
      }
    }
  }

 private:
  friend class base::RefCounted<FutureVectorJoin>;

  ~FutureVectorJoin() = default;

  void OnComplete(size_t index, T value) {
    optionals_[index] = std::move(value);
    DCHECK_GT(remaining_, static_cast<size_t>(0));
    if (--remaining_ == 0) {
      std::vector<T> values;
      for (auto& optional : optionals_) {
        values.push_back(std::move(*optional));
      }
      resolver_.Complete(std::move(values));
    }
  }

  Resolver resolver_;
  std::vector<absl::optional<T>> optionals_;
  size_t remaining_ = 0;
  bool started_ = false;
};

// Returns a |Future| for an |std::tuple| that contains the resolved values for
// all |Future|s supplied as arguments.
//
// Example:
//   Future<std::tuple<bool, int, std::string>> joined = JoinFutures(
//       Future<bool>::Completed(true),
//       Future<int>::Completed(42),
//       Future<std::string>::Completed("hello world"));
template <typename... Args>
Future<std::tuple<Args...>> JoinFutures(Future<Args>... args) {
  return Future<std::tuple<Args...>>::Create([&](auto resolver) {
    auto ref = base::MakeRefCounted<FutureJoin<Args...>>(resolver);
    ref->AddFutures(std::move(args)...);
  });
}

// Returns a |Future| for an |std::vector| that contains the resolved values for
// all |Future|s in the supplied vector.
//
// Example:
//   std::vector<Future<int>> futures;
//   futures.push_back(Future<int>::Completed(1));
//   futures.push_back(Future<int>::Completed(2));
//
//   Future<std::vector<int>> joined = JoinFutures(std::move(futures));
template <typename T>
Future<std::vector<T>> JoinFutures(std::vector<Future<T>>&& futures) {
  return Future<std::vector<T>>::Create([&](auto resolver) {
    auto ref = base::MakeRefCounted<FutureVectorJoin<T>>(resolver);
    ref->AddFutures(std::move(futures));
  });
}

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_JOIN_H_
