/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_BAP_REPORTER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_BAP_REPORTER_H_

#include <functional>
#include <vector>

#include "base/timer/timer.h"
#include "bat/ledger/internal/promotion/bap_reporter_endpoint.h"
#include "bat/ledger/ledger.h"

namespace ledger {

class LedgerImpl;

namespace promotion {

class BAPReporter {
 public:
  explicit BAPReporter(LedgerImpl* ledger);

  BAPReporter(const BAPReporter&) = delete;
  BAPReporter& operator=(const BAPReporter&) = delete;

  ~BAPReporter();

  void ReportBAPAmount();

 private:
  void OnUnblindedTokens(std::vector<type::UnblindedTokenPtr> tokens);
  void OnEndpointResponse(bool success);
  void ScheduleRetryAfterZeroBalance();
  void ScheduleRetryAfterFailure();

  LedgerImpl* ledger_;
  bool running_ = false;
  int retry_count_ = 0;
  base::OneShotTimer timer_;
  BAPReporterEndpoint endpoint_;
};

}  // namespace promotion
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_BAP_REPORTER_H_
