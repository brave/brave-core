/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_H_

#include <type_traits>
#include <utility>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {

// Represents the result of an asynchronous operation.
//
// Example:
//   auto future = Future<int>::Create([](auto resolver) {
//     resolver.Complete(42);
//   });
//
//   future.Then(base::BindOnce([](int value) {}));
//
// Listeners are called on the current SequencedTaskRunner, and are guaranteed
// to be called asynchronously. Future and Resolver objects are internally
// reference counted and can be passed between sequences; the internal data
// structures are updated on the sequence that created the Resolver.
template <typename T>
class Future {
  static_assert(!std::is_reference<T>::value && !std::is_pointer<T>::value,
                "Future<T> is not supported for pointer or reference types");

  static_assert(!std::is_void<T>::value, "Future<void> is not supported");

  struct Store;

 public:
  using CompleteType = T;
  using CompleteCallback = base::OnceCallback<void(T)>;

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  void Then(CompleteCallback on_complete) {
    DCHECK(store_);
    Listener listener = {.on_complete = std::move(on_complete),
                         .task_runner = base::SequencedTaskRunnerHandle::Get()};

    task_runner_->PostTask(FROM_HERE, base::BindOnce(AddListenerInTask, store_,
                                                     std::move(listener)));
    store_.reset();
  }

  void DiscardValueThen(base::OnceCallback<void()> on_complete) {
    Then(base::BindOnce(
        [](base::OnceCallback<void()> cb, T) { std::move(cb).Run(); },
        std::move(on_complete)));
  }

  template <typename U>
  using MapCompleteCallback = base::OnceCallback<U(T)>;

  template <typename U>
  Future<U> Map(MapCompleteCallback<U> map_complete) {
    typename Future<U>::FuturePair pair;
    Then(base::BindOnce(MapFutureValue<U>, pair.resolver,
                        std::move(map_complete)));
    return std::move(pair.future);
  }

  template <typename U>
  using FlatMapCompleteCallback = base::OnceCallback<Future<U>(T)>;

  template <typename U>
  Future<U> FlatMap(FlatMapCompleteCallback<U> map_complete) {
    typename Future<U>::FuturePair pair;
    Then(base::BindOnce(FlatMapFutureValue<U>, pair.resolver,
                        std::move(map_complete)));
    return std::move(pair.future);
  }

  struct FuturePair;

  class Resolver {
   public:
    void Complete(T value) {
      task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(SetCompleteInTask, store_, std::move(value)));
    }

   private:
    friend FuturePair;

    explicit Resolver(const Future& future)
        : store_(future.store_), task_runner_(future.task_runner_) {}

    scoped_refptr<Store> store_;
    scoped_refptr<base::SequencedTaskRunner> task_runner_;
  };

  struct FuturePair {
    FuturePair() : future(), resolver(future) {}
    Future future;
    Resolver resolver;
  };

  static Future Completed(T value) {
    FuturePair pair;
    pair.resolver.Complete(std::move(value));
    return std::move(pair.future);
  }

  template <typename F>
  static Future Create(F fn) {
    FuturePair pair;
    fn(pair.resolver);
    return std::move(pair.future);
  }

 private:
  enum class State { kPending, kComplete, kEmpty };

  struct Listener {
    CompleteCallback on_complete;
    scoped_refptr<base::SequencedTaskRunner> task_runner;
  };

  struct Store : public base::RefCountedThreadSafe<Store> {
    Store() {}
    State state = State::kPending;
    absl::optional<T> value;
    absl::optional<Listener> listener;
  };

  Future()
      : store_(new Store()),
        task_runner_(base::SequencedTaskRunnerHandle::Get()) {}

  template <typename U>
  static void MapFutureValue(typename Future<U>::Resolver resolver,
                             MapCompleteCallback<U> map_complete,
                             T value) {
    resolver.Complete(std::move(map_complete).Run(std::move(value)));
  }

  template <typename U>
  static void FlatMapFutureValue(typename Future<U>::Resolver resolver,
                                 FlatMapCompleteCallback<U> map_complete,
                                 T value) {
    std::move(map_complete)
        .Run(std::move(value))
        .Then(base::BindOnce(FlatMapComplete<U>, resolver));
  }

  template <typename U>
  static void FlatMapComplete(typename Future<U>::Resolver resolver, U value) {
    resolver.Complete(std::move(value));
  }

  static void AddListenerInTask(scoped_refptr<Store> store, Listener listener) {
    switch (store->state) {
      case State::kComplete:
        store->state = State::kEmpty;
        DCHECK(store->value);
        listener.task_runner->PostTask(
            FROM_HERE,
            base::BindOnce(RunCompleteCallback, std::move(*store->value),
                           std::move(listener.on_complete)));
        break;
      case State::kPending:
        if (!store->listener) {
          store->listener = std::move(listener);
        }
        break;
      case State::kEmpty:
        break;
    }
  }

  static void SetCompleteInTask(scoped_refptr<Store> store, T value) {
    if (store->state != State::kPending) {
      return;
    }

    store->state = State::kComplete;
    store->value = std::move(value);

    if (store->listener) {
      store->state = State::kEmpty;
      Listener listener = std::move(*store->listener);
      listener.task_runner->PostTask(
          FROM_HERE,
          base::BindOnce(RunCompleteCallback, std::move(*store->value),
                         std::move(listener.on_complete)));
    }
  }

  static void RunCompleteCallback(T value, CompleteCallback on_complete) {
    std::move(on_complete).Run(std::move(value));
  }

  scoped_refptr<Store> store_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

template <typename T>
using FuturePair = typename Future<T>::FuturePair;

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_H_
