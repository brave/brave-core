/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_SHARED_FUTURE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_SHARED_FUTURE_H_

#include <list>
#include <type_traits>
#include <utility>

#include "bat/ledger/internal/core/future.h"

namespace ledger {

template <typename T>
class SharedFutureState
    : public base::RefCountedThreadSafe<SharedFutureState<T>> {
 public:
  SharedFutureState() : task_runner_(base::SequencedTaskRunnerHandle::Get()) {}

  void SetValue(T value) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&SharedFutureState::SetValueInTask, this,
                                  std::move(value)));
  }

  void AddListener(base::OnceCallback<void(const T&)> on_complete) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &SharedFutureState::AddListenerInTask, this,
            Listener{.on_complete = std::move(on_complete),
                     .task_runner = base::SequencedTaskRunnerHandle::Get()}));
  }

 private:
  friend class base::RefCountedThreadSafe<SharedFutureState>;

  ~SharedFutureState() = default;

  struct Listener {
    base::OnceCallback<void(const T&)> on_complete;
    scoped_refptr<base::SequencedTaskRunner> task_runner;
  };

  void AddListenerInTask(Listener listener) {
    if (value_) {
      listener.task_runner->PostTask(
          FROM_HERE, base::BindOnce(RunCompleteCallback, *value_,
                                    std::move(listener.on_complete)));
    } else {
      listeners_.push_back(std::move(listener));
    }
  }

  void SetValueInTask(T value) {
    if (value_) {
      return;
    }

    value_ = std::move(value);

    for (auto& listener : listeners_) {
      listener.task_runner->PostTask(
          FROM_HERE, base::BindOnce(RunCompleteCallback, *value_,
                                    std::move(listener.on_complete)));
    }

    listeners_.clear();
  }

  static void RunCompleteCallback(
      const T& value,
      base::OnceCallback<void(const T&)> on_complete) {
    std::move(on_complete).Run(value);
  }

  absl::optional<T> value_;
  std::list<Listener> listeners_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

// Like Future<T>, except that it is is copyable and the result of the
// asynchronous operation is revealed to continuations as a const reference.
// Always prefer |Future| to |SharedFuture|. In general, |SharedFuture| should
// only be used when the result needs to be cached or deduped.
//
// Example:
//   Future<int> future = MakeReadyFuture(42);
//   SharedFuture shared_future(std::move(future));
//   shared_future.Then(base::BindOnce([](const int& value) {}));
template <typename T>
class SharedFuture {
 private:
  static_assert(!std::is_reference<T>::value && !std::is_pointer<T>::value,
                "SharedFuture<T> is not supported for pointer or reference "
                "types");

  static_assert(!std::is_void<T>::value, "SharedFuture<void> is not supported");

 public:
  using ValueType = T;

  explicit SharedFuture(Future<T> future) : state_(new SharedFutureState<T>()) {
    future.Then(
        base::BindOnce([](scoped_refptr<SharedFutureState<T>> state,
                          T value) { state->SetValue(std::move(value)); },
                       state_));
  }

  SharedFuture(const SharedFuture& other) = default;
  SharedFuture& operator=(const SharedFuture& other) = default;

  SharedFuture(SharedFuture&& other) = default;
  SharedFuture& operator=(SharedFuture&& other) = default;

  // Attaches a callback that will be executed when the shared future value is
  // available. The callback will be executed on the caller's task runner.
  void Then(base::OnceCallback<void(const T&)> on_complete) {
    state_->AddListener(std::move(on_complete));
  }

  // Attaches a transforming callback that will be executed when the shared
  // future value is available. Returns a non-shared future for the transformed
  // value.
  template <typename U>
  Future<U> Then(base::OnceCallback<Future<U>(const T&)> transform) {
    Promise<U> promise;
    Future<U> future = promise.GetFuture();
    Then(base::BindOnce(TransformAndUnwrapFutureValue<U>, std::move(promise),
                        std::move(transform)));
    return future;
  }

  // Attaches a transforming callback that will be executed when the shared
  // future value is available. Returns a non-shared future for the transformed
  // value.
  template <typename U>
  Future<U> Then(base::OnceCallback<U(const T&)> transform) {
    Promise<U> promise;
    Future<U> future = promise.GetFuture();
    Then(base::BindOnce(TransformFutureValue<U>, std::move(promise),
                        std::move(transform)));
    return future;
  }

 private:
  template <typename U>
  static void TransformFutureValue(Promise<U> promise,
                                   base::OnceCallback<U(const T&)> transform,
                                   const T& value) {
    promise.SetValue(std::move(transform).Run(value));
  }

  template <typename U>
  static void TransformAndUnwrapFutureValue(
      Promise<U> promise,
      base::OnceCallback<Future<U>(const T&)> transform,
      const T& value) {
    std::move(transform).Run(value).Then(
        base::BindOnce(UnwrapFutureValue<U>, std::move(promise)));
  }

  template <typename U>
  static void UnwrapFutureValue(Promise<U> promise, U value) {
    promise.SetValue(std::move(value));
  }

  scoped_refptr<SharedFutureState<T>> state_;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_SHARED_FUTURE_H_
