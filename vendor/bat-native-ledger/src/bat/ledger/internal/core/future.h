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

namespace {
template <typename T>
struct identity {
  using type = T;
};

template <typename T>
using identity_t = typename identity<T>::type;

template <typename, template <typename...> typename>
struct is_specialization_of : std::false_type {};

template <template <typename...> typename TT, typename... Ts>
struct is_specialization_of<TT<Ts...>, TT> : std::true_type {};

template <typename T, template <typename...> typename TT>
inline constexpr bool is_specialization_of_v =
    is_specialization_of<T, TT>::value;
}  // namespace

namespace ledger {

template <typename... Ts>
class FutureState : public base::RefCountedThreadSafe<FutureState<Ts...>> {
 public:
  using CompleteCallback = base::OnceCallback<void(Ts...)>;

  FutureState() : task_runner_(base::SequencedTaskRunnerHandle::Get()) {}

  template <typename... Vs>
  void Set(Vs&&... values) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(&FutureState::SetValuesInTask, this,
                                          std::forward<Vs>(values)...));
  }

  void SetListener(CompleteCallback on_complete) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &FutureState::SetListenerInTask, this,
            Listener{.on_complete = std::move(on_complete),
                     .task_runner = base::SequencedTaskRunnerHandle::Get()}));
  }

 private:
  enum class Status { kPending, kComplete, kEmpty };

  struct Listener {
    CompleteCallback on_complete;
    scoped_refptr<base::SequencedTaskRunner> task_runner;
  };

  void SetListenerInTask(Listener listener) {
    switch (status_) {
      case Status::kComplete:
        status_ = Status::kEmpty;
        DCHECK(values_);
        listener.task_runner->PostTask(
            FROM_HERE, base::BindOnce(RunCompleteCallback, std::move(*values_),
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

  void SetValuesInTask(Ts... values) {
    if (status_ != Status::kPending) {
      return;
    }

    values_.emplace(std::move(values)...);
    status_ = Status::kComplete;

    if (listener_) {
      status_ = Status::kEmpty;
      Listener listener = std::move(*listener_);
      listener.task_runner->PostTask(
          FROM_HERE, base::BindOnce(RunCompleteCallback, std::move(*values_),
                                    std::move(listener.on_complete)));
    }
  }

  static void RunCompleteCallback(std::tuple<Ts...> values,
                                  CompleteCallback on_complete) {
    std::apply(
        [on_complete = std::move(on_complete)](auto&&... values) mutable {
          std::move(on_complete).Run(std::forward<decltype(values)>(values)...);
        },
        std::move(values));
  }

  Status status_ = Status::kPending;
  absl::optional<std::tuple<Ts...>> values_;
  absl::optional<Listener> listener_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

template <typename...>
class Future;

template <typename... Ts>
class Promise {
 public:
  Promise() : state_(new FutureState<Ts...>), future_(Future<Ts...>(state_)) {}

  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  // Gets the associated future for this promise. This function may only be
  // called once; additional calls will result in a crash.
  Future<Ts...> GetFuture() {
    CHECK(future_);
    return *std::exchange(future_, absl::nullopt);
  }

  // Sets the completed values of the associated future.
  template <typename... Vs>
  void Set(Vs&&... values) {
    state_->Set(std::forward<Vs>(values)...);
  }

 private:
  scoped_refptr<FutureState<Ts...>> state_;
  absl::optional<Future<Ts...>> future_;
};

template <typename... Ts>
struct Unwrap : std::tuple<Ts...> {
  using std::tuple<Ts...>::tuple;
};

template <typename... Ts>
Unwrap(Ts...) -> Unwrap<Ts...>;

template <typename... Ts>
decltype(auto) deduce_tuple(std::tuple<Ts...>& t) {
  return t;
}

template <typename... Ts>
decltype(auto) deduce_tuple(std::tuple<Ts...>&& t) {
  return std::move(t);
}

// Represents the result of an asynchronous operation.
//
// Example:
//   Promise<int> promise;
//   promise.SetValue(10);
//   Future<int> future = promise.GetFuture();
//   future.Then(base::BindOnce([](int value) {}));
template <typename... Ts>
class Future {
  // Calculates the Promise type for transforming `Then`.
  template <typename R>
  struct promise_impl {
    using type = Promise<R>;
  };

  template <typename... Rs>
  struct promise_impl<Unwrap<Rs...>> {
    using type = Promise<Rs...>;
  };

  template <typename... Rs>
  struct promise_impl<Future<Rs...>> {
    using type = Promise<Rs...>;
  };

  template <typename R>
  using promise_t = typename promise_impl<R>::type;

  using tuple = std::tuple<Ts...>;

 public:
  using ValueType = typename std::conditional_t<sizeof...(Ts) == 1,
                                                std::tuple_element<0, tuple>,
                                                identity<tuple>>::type;

  // We need the above identity trick, since otherwise if Ts is empty, we would
  // end up instantiating std::tuple_element_t<0, tuple> for an empty tuple
  // (which results in a compilation error).
  // In other words, this doesn't work:
  // using ValueType = std::
  //    conditional_t<sizeof...(Ts) == 1, std::tuple_element_t<0, tuple>,
  //    tuple>;

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  // converting `Then`:
  // Forwards the call to the appropriate `Then(base::OnceCallback<>)` overload.
  template <typename R, typename... As>
  auto Then(base::RepeatingCallback<R(As...)> then) {
    return Then(base::OnceCallback<R(As...)>(std::move(then)));
  }

  // non-transforming `Then`:
  // Attaches a callback that will be executed when the future values are
  // available. The callback will be executed on the caller's task runner.
  // Returns an empty future to enable specifying continuations.
  auto Then(base::OnceCallback<void(Ts...)> then) {
    Promise<> promise;
    auto future = promise.GetFuture();
    state_->SetListener(base::BindOnce(
        [](base::OnceCallback<void(Ts...)> then, Promise<> promise, Ts... ts) {
          std::move(then).Run(std::move(ts)...);
          promise.Set();
        },
        std::move(then), std::move(promise)));
    state_.reset();
    return future;
  }

  // transforming `Then`:
  // Attaches a transforming callback that will be executed when the future
  // values are available. Returns a future for the transformed values.
  // `Future<>`s returned by |then| are flattened.
  template <typename R>
  auto Then(base::OnceCallback<R(Ts...)> then) {
    promise_t<R> promise;
    auto future = promise.GetFuture();
    Then(base::BindOnce(Transform<decltype(then)>, std::move(then),
                        std::move(promise)));
    return future;
  }

  // discarding `Then`:
  // Attaches a callback that will be executed when the future values are
  // available - future values are discarded.
  // Disabled for `Future<>` (where `Ts` is empty), which handles these
  // callbacks via the non-transforming and transforming overloads.
  template <typename R,
            std::size_t size = sizeof...(Ts),
            std::enable_if_t<size != 0>* = nullptr>
  auto Then(base::OnceCallback<R()> then) {
    return Then(base::BindOnce([](base::OnceCallback<R()> then,
                                  Ts...) { return std::move(then).Run(); },
                               std::move(then)));
  }

 private:
  friend class Promise<Ts...>;

  explicit Future(scoped_refptr<FutureState<Ts...>> state)
      : state_(std::move(state)) {
    DCHECK(state_);
  }

  template <typename>
  struct SetValues;

  template <typename... Vs>
  struct SetValues<Promise<Vs...>> {
    void operator()(Promise<Vs...> promise, Vs... values) const {
      promise.Set(std::move(values)...);
    }
  };

  template <typename Then, typename R = typename Then::ResultType>
  static void Transform(Then then, promise_t<R> promise, Ts... values) {
    if constexpr (is_specialization_of_v<R, Unwrap>) {
      std::apply(
          [&](auto&&... values) {
            promise.Set(std::forward<decltype(values)>(values)...);
          },
          deduce_tuple(std::move(then).Run(std::move(values)...)));
    } else if constexpr (is_specialization_of_v<R, Future>) {
      std::move(then)
          .Run(std::move(values)...)
          .Then(base::BindOnce(SetValues<decltype(promise)>(),
                               std::move(promise)));
    } else {
      promise.Set(std::move(then).Run(std::move(values)...));
    }
  }

  scoped_refptr<FutureState<Ts...>> state_;
};

template <typename... Ts, typename Then>
auto operator|(Future<Ts...>& future, Then then) {
  if constexpr (is_specialization_of_v<Then, base::OnceCallback>) {
    return future.Then(std::move(then));
  } else {
    return future.Then(base::BindOnce(std::move(then)));
  }
}

template <typename... Ts, typename Then>
auto operator|(Future<Ts...>&& future, Then then) {
  return future | std::move(then);
}

template <typename... Ts, typename Then>
auto operator|(Promise<Ts...>& promise, Then then) {
  return promise.GetFuture() | std::move(then);
}

// Returns a completed future that wraps the provided values.
template <typename... Vs>
auto MakeFuture(Vs&&... values) {
  Promise<std::decay_t<Vs>...> promise;
  promise.Set(std::forward<Vs>(values)...);
  return promise.GetFuture();
}

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_FUTURE_H_
