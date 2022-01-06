/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_DELAY_GENERATOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_DELAY_GENERATOR_H_

#include "base/time/time.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

class DelayGenerator : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "delay-generator";

  Future<base::TimeDelta> Delay(base::Location location, base::TimeDelta delay);

  Future<base::TimeDelta> RandomDelay(base::Location location,
                                      base::TimeDelta delay);
};

class BackoffDelay {
 public:
  BackoffDelay(base::TimeDelta min, base::TimeDelta max);

  base::TimeDelta GetNextDelay();

  void Reset();

  int count() const { return backoff_count_; }

 private:
  base::TimeDelta min_;
  base::TimeDelta max_;
  int backoff_count_ = 0;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_DELAY_GENERATOR_H_
