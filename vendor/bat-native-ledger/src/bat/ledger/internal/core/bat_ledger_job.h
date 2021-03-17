/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_JOB_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_JOB_H_

#include <utility>

#include "base/memory/weak_ptr.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

// Convenience class for defining job classes that can be started by calling
// |BATLedgerContext::StartJob|. Subclasses must implement a |Start| method
// that begins the asynchronous operation.
//
// Example:
//   class MyJob : public BATLedgerJob<int> {
//    public:
//     void Start() {
//       Complete(42);
//     }
//   };
template <typename T>
class BATLedgerJob : public BATLedgerContext::Object,
                     public base::SupportsWeakPtr<BATLedgerJob<T>> {
 public:
  // Returns the Future for the job.
  Future<T> GetFuture() { return std::move(future_pair_.future); }

 protected:
  // Completes the job with the specified value.
  virtual void Complete(typename Future<T>::CompleteType value) {
    future_pair_.resolver.Complete(std::move(value));
  }

  // Completes the job with the specified future value.
  void CompleteWith(Future<T> future) {
    future.Then(
        base::BindOnce(&BATLedgerJob::OnInnerCompleted, base::AsWeakPtr(this)));
  }

  // Returns a |OnceCallback| that wraps the specified member function. The
  // resulting callback is bound with a WeakPtr for the receiver. It is not
  // bound with any additional arguments.
  template <typename Derived, typename... Args>
  inline static auto ContinueWith(Derived* self, void (Derived::*fn)(Args...)) {
    return base::BindOnce(fn, base::AsWeakPtr(self));
  }

  // Returns a lambda that wraps the specified member function. The resulting
  // lambda will be a no-op if the job has been destroyed. This helper should
  // only be used for older APIs that require |std::function| callbacks.
  template <typename Derived, typename... Args>
  inline static auto CreateLambdaCallback(Derived* self,
                                          void (Derived::*fn)(Args...)) {
    return
        [weak_self = base::AsWeakPtr(self), fn = std::move(fn)](Args... args) {
          if (weak_self) {
            (weak_self.get()->*fn)(std::forward<Args>(args)...);
          }
        };
  }

 private:
  void OnInnerCompleted(typename Future<T>::CompleteType value) {
    Complete(value);
  }

  FuturePair<T> future_pair_;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_JOB_H_
