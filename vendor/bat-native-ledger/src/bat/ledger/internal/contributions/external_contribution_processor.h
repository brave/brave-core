/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_EXTERNAL_CONTRIBUTION_PROCESSOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_EXTERNAL_CONTRIBUTION_PROCESSOR_H_

#include "bat/ledger/internal/contributions/contribution_data.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

class ExternalContributionProcessor : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "external-contribution-processor";

  enum class Result {
    kSuccess,
    kBalanceUnavailable,
    kInsufficientFunds,
    kNoPublisherAddress,
    kTransferError
  };

  Future<Result> ProcessContribution(const Contribution& contribution);
};

using ExternalContributionResult = ExternalContributionProcessor::Result;

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_EXTERNAL_CONTRIBUTION_PROCESSOR_H_
