/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_TOKEN_VENDOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_TOKEN_VENDOR_H_

#include <string>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

class ContributionTokenVendor : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "contribution-token-vendor";

  std::string StartPurchase(double amount);

  Future<bool> ResumePurchase(const std::string& job_id);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_TOKEN_VENDOR_H_
