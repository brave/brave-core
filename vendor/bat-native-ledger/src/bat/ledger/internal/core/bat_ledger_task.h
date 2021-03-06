/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_TASK_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_TASK_H_

#include <utility>

#include "bat/ledger/internal/core/async_result.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"

namespace ledger {

// Convenience class for defining task components that can be started by calling
// |BATLedgerContext::StartTask|. Subclasses must implement a |Start| method
// that begins the asynchronous operation.
//
// Example:
//   class MyTask : public BATLedgerTask<int> {
//    public:
//     expicit MyTask(BATLedgerContext* context)
//         : BATLedgerTask<int>(context) {}
//
//     void Start() {
//       Complete(42);
//     }
//   };
template <typename T>
class BATLedgerTask : public BATLedgerContext::Component {
 public:
  using Result = AsyncResult<T>;

  // Returns the AsyncResult for the task.
  Result result() const { return resolver_.result(); }

 protected:
  explicit BATLedgerTask(BATLedgerContext* context) : Component(context) {}

  // Completes the task with the specified value.
  void Complete(T&& value) { resolver_.Complete(std::move(value)); }

 private:
  typename Result::Resolver resolver_;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_TASK_H_
