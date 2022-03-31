/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ASYNC_RESULT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ASYNC_RESULT_H_

#include <list>
#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {

// Represents the result of an asynchronous operation.
//
// Example:
//   AsyncResult<int>::Resolver resolver;
//   resolver.Complete(42);
//
//   AsyncResult<int> result = resolver.result();
//   result.Then(base::BindOnce([](const int& value) {}));
//
// Listeners are called on the current SequencedTaskRunner, and are guaranteed
// to be called asynchronously. AsyncResult and Resolver objects are internally
// reference counted and can be passed between sequences; the internal data
// structures are updated on the sequence that created the Resolver.
template <typename T>
class AsyncResult {
 public:
  using CompleteType = T;
  using CompleteCallback = base::OnceCallback<void(const T&)>;

  void Then(CompleteCallback on_complete) {
    Listener listener = {.on_complete = std::move(on_complete),
                         .task_runner = base::SequencedTaskRunnerHandle::Get()};

    task_runner_->PostTask(FROM_HERE, base::BindOnce(AddListenerInTask, store_,
                                                     std::move(listener)));
  }

  class Resolver {
   public:
    Resolver() {}
    void Complete(T&& value) { result_.Complete(std::move(value)); }
    AsyncResult result() const { return result_; }

   private:
    AsyncResult result_;
  };

 private:
  AsyncResult()
      : store_(new Store()),
        task_runner_(base::SequencedTaskRunnerHandle::Get()) {}

  enum class State { kPending, kComplete };

  struct Listener {
    CompleteCallback on_complete;
    scoped_refptr<base::SequencedTaskRunner> task_runner;
  };

  struct Store {
    Store() {}
    State state = State::kPending;
    absl::optional<T> value;
    std::list<Listener> listeners;
  };

  void Complete(T&& value) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(SetCompleteInTask, store_, std::move(value)));
  }

  static void AddListenerInTask(std::shared_ptr<Store> store,
                                Listener listener) {
    switch (store->state) {
      case State::kComplete:
        listener.task_runner->PostTask(
            FROM_HERE, base::BindOnce(RunCompleteCallback, store,
                                      std::move(listener.on_complete)));
        break;
      case State::kPending:
        store->listeners.emplace_back(std::move(listener));
        break;
    }
  }

  static void SetCompleteInTask(std::shared_ptr<Store> store, T value) {
    if (store->state != State::kPending)
      return;

    store->state = State::kComplete;
    store->value = std::move(value);

    for (auto& listener : store->listeners) {
      listener.task_runner->PostTask(
          FROM_HERE, base::BindOnce(RunCompleteCallback, store,
                                    std::move(listener.on_complete)));
    }

    store->listeners.clear();
  }

  static void RunCompleteCallback(std::shared_ptr<Store> store,
                                  CompleteCallback on_complete) {
    DCHECK(store->value);
    std::move(on_complete).Run(*store->value);
  }

  std::shared_ptr<Store> store_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ASYNC_RESULT_H_
