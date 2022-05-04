/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_JOIN_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_JOIN_H_

#include <algorithm>
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
  using Promise = Promise<std::tuple<Args...>>;

 public:
  explicit FutureJoin(Promise promise) : promise_(std::move(promise)) {}

  void AddFutures(Future<Args>... futures) {
    DCHECK(!started_);
    if (!started_) {
      started_ = true;
      AddHandlers(std::make_index_sequence<sizeof...(futures)>(),
                  std::move(futures)...);
    }
  }

 private:
  friend class base::RefCounted<FutureJoin>;

  ~FutureJoin() = default;

  template <std::size_t... Indexes, typename... Futures>
  void AddHandlers(std::index_sequence<Indexes...>, Futures... futures) {
    (...,
     futures.Then(base::BindOnce(
         &FutureJoin::OnComplete<Indexes, typename Futures::ValueType>, this)));
  }

  template <size_t Index, typename T>
  void OnComplete(T value) {
    std::get<Index>(optionals_) = std::move(value);
    if (AllValuesReceived()) {
      SetValue();
    }
  }

  bool AllValuesReceived() {
    return std::apply([](auto&&... opt) { return (... && opt); }, optionals_);
  }

  void SetValue() {
    promise_.SetValue(
        std::apply([](auto... opt) { return std::tuple(*std::move(opt)...); },
                   std::move(optionals_)));
  }

  Promise promise_;
  std::tuple<absl::optional<Args>...> optionals_;
  bool started_ = false;
};

template <typename T>
class FutureVectorJoin : public base::RefCounted<FutureVectorJoin<T>> {
  using Promise = Promise<std::vector<T>>;

 public:
  explicit FutureVectorJoin(Promise promise) : promise_(std::move(promise)) {}

  void AddFutures(std::vector<Future<T>>&& futures) {
    DCHECK(!started_);
    if (!started_) {
      started_ = true;

      if (futures.empty()) {
        promise_.SetValue({});
        return;
      }

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
    if (AllValuesReceived()) {
      std::vector<T> values;
      for (auto& optional : optionals_) {
        values.push_back(std::move(*optional));
      }
      promise_.SetValue(std::move(values));
    }
  }

  bool AllValuesReceived() {
    return std::all_of(optionals_.cbegin(), optionals_.cend(),
                       [](auto&& opt) { return opt.has_value(); });
  }

  Promise promise_;
  std::vector<absl::optional<T>> optionals_;
  bool started_ = false;
};

// Returns a |Future| for an |std::tuple| that contains the resolved values for
// all |Future|s supplied as arguments.
//
// Example:
//   Future<std::tuple<bool, int, std::string>> joined = JoinFutures(
//       MakeReadyFuture(true),
//       MakeReadyFuture(42),
//       MakeReadyFuture(std::string("hello world")));
template <typename... Args>
Future<std::tuple<Args...>> JoinFutures(Future<Args>... args) {
  Promise<std::tuple<Args...>> promise;
  auto future = promise.GetFuture();
  auto ref = base::MakeRefCounted<FutureJoin<Args...>>(std::move(promise));
  ref->AddFutures(std::move(args)...);
  return future;
}

// Returns a |Future| for an |std::vector| that contains the resolved values for
// all |Future|s in the supplied vector.
//
// Example:
//   std::vector<Future<int>> futures;
//   futures.push_back(MakeReadyFuture(1));
//   futures.push_back(MakeReadyFuture(2));
//
//   Future<std::vector<int>> joined = JoinFutures(std::move(futures));
template <typename T>
Future<std::vector<T>> JoinFutures(std::vector<Future<T>>&& futures) {
  Promise<std::vector<T>> promise;
  auto future = promise.GetFuture();
  auto ref = base::MakeRefCounted<FutureVectorJoin<T>>(std::move(promise));
  ref->AddFutures(std::move(futures));
  return future;
}

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_JOIN_H_
