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

template <typename T>
class FutureState : public base::RefCountedThreadSafe<FutureState<T>> {
  static_assert(!std::is_reference<T>::value && !std::is_pointer<T>::value,
                "Future<T> is not supported for pointer or reference types");

  static_assert(!std::is_void<T>::value, "Future<void> is not supported");

 public:
  FutureState() : task_runner_(base::SequencedTaskRunnerHandle::Get()) {}

  void SetValue(T value) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&FutureState::SetValueInTask, this, std::move(value)));
  }

  void SetListener(base::OnceCallback<void(T)> on_complete) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &FutureState::SetListenerInTask, this,
            Listener{.on_complete = std::move(on_complete),
                     .task_runner = base::SequencedTaskRunnerHandle::Get()}));
  }

 private:
  friend class base::RefCountedThreadSafe<FutureState>;

  ~FutureState() = default;

  enum class Status { kPending, kComplete, kEmpty };

  struct Listener {
    base::OnceCallback<void(T)> on_complete;
    scoped_refptr<base::SequencedTaskRunner> task_runner;
  };

  void SetListenerInTask(Listener listener) {
    switch (status_) {
      case Status::kComplete:
        status_ = Status::kEmpty;
        DCHECK(value_);
        listener.task_runner->PostTask(
            FROM_HERE, base::BindOnce(RunCompleteCallback, std::move(*value_),
                                      std::move(listener.on_complete)));
        break;
      case Status::kPending:
        if (!listener_) {
          listener_ = std::move(listener);
        }
        break;
      case Status::kEmpty:
        break;
    }
  }

  void SetValueInTask(T value) {
    if (status_ != Status::kPending) {
      return;
    }

    status_ = Status::kComplete;
    value_ = std::move(value);

    if (listener_) {
      status_ = Status::kEmpty;
      Listener listener = std::move(*listener_);
      listener.task_runner->PostTask(
          FROM_HERE, base::BindOnce(RunCompleteCallback, std::move(*value_),
                                    std::move(listener.on_complete)));
    }
  }

  static void RunCompleteCallback(T value,
                                  base::OnceCallback<void(T)> on_complete) {
    std::move(on_complete).Run(std::move(value));
  }

  Status status_ = Status::kPending;
  absl::optional<T> value_;
  absl::optional<Listener> listener_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

template <typename T>
class Promise;

// Represents the result of an asynchronous operation.
//
// Example:
//   Promise<int> promise;
//   promise.SetValue(10);
//   Future<int> future = promise.GetFuture();
//   future.Then(base::BindOnce([](int value) {}));
template <typename T>
class Future {
 public:
  using ValueType = T;

  // Attaches a callback that will be executed when the future value is
  // available. The callback will be executed on the caller's task runner.
  void Then(base::OnceCallback<void(T)> on_complete) {
    state_->SetListener(std::move(on_complete));
    state_.reset();
  }

  // Attaches a transforming callback that will be executed when the future
  // value is available. Returns a future for the transformed value.
  template <typename U>
  Future<U> Then(base::OnceCallback<Future<U>(T)> transform) {
    Promise<U> promise;
    Future<U> future = promise.GetFuture();
    Then(base::BindOnce(TransformAndUnwrapFutureValue<U>, std::move(promise),
                        std::move(transform)));
    return future;
  }

  // Attaches a transforming callback that will be executed when the future
  // value is available. Returns a future for the transformed value.
  template <typename U>
  Future<U> Then(base::OnceCallback<U(T)> transform) {
    Promise<U> promise;
    Future<U> future = promise.GetFuture();
    Then(base::BindOnce(TransformFutureValue<U>, std::move(promise),
                        std::move(transform)));
    return future;
  }

 private:
  friend class Promise<T>;

  explicit Future(scoped_refptr<FutureState<T>> state)
      : state_(std::move(state)) {
    DCHECK(state_);
  }

  template <typename U>
  static void TransformFutureValue(Promise<U> promise,
                                   base::OnceCallback<U(T)> transform,
                                   T value) {
    promise.SetValue(std::move(transform).Run(std::move(value)));
  }

  template <typename U>
  static void TransformAndUnwrapFutureValue(
      Promise<U> promise,
      base::OnceCallback<Future<U>(T)> transform,
      T value) {
    std::move(transform)
        .Run(std::move(value))
        .Then(base::BindOnce(UnwrapFutureValue<U>, std::move(promise)));
  }

  template <typename U>
  static void UnwrapFutureValue(Promise<U> promise, U value) {
    promise.SetValue(std::move(value));
  }

  scoped_refptr<FutureState<T>> state_;
};

template <typename T>
class Promise {
 public:
  Promise() : state_(new FutureState<T>), future_(Future<T>(state_)) {}

  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  // Gets the associated future for this promise. This function may only be
  // called once; additional calls will result in a crash.
  Future<T> GetFuture() {
    CHECK(future_);
    Future<T> future(std::move(*future_));
    future_.reset();
    return future;
  }

  // Sets the completed value of the associated future.
  void SetValue(T value) { state_->SetValue(std::move(value)); }

 private:
  scoped_refptr<FutureState<T>> state_;
  absl::optional<Future<T>> future_;
};

// Returns an already-completed future that wraps the provided value.
template <typename T>
Future<T> MakeReadyFuture(T value) {
  Promise<T> promise;
  promise.SetValue(std::move(value));
  return promise.GetFuture();
}

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_H_
