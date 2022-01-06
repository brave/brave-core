/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_STORE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_STORE_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "bat/ledger/internal/contributions/contribution_data.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"
#include "bat/ledger/internal/external_wallet/external_wallet_data.h"

namespace ledger {

class ContributionStore : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "contribution-store";

  Future<bool> SaveCompletedContribution(const Contribution& contribution);

  Future<bool> SaveCompletedAutoContribute(
      const base::flat_map<std::string, double>& publisher_amounts,
      ContributionSource source);

  Future<bool> SavePendingContribution(const std::string& publisher_id,
                                       double amount);

  Future<bool> SavePendingContribution(const std::string& publisher_id,
                                       double amount,
                                       base::Time created_at);

  Future<std::vector<PendingContribution>> GetPendingContributions();

  Future<bool> DeletePendingContribution(int64_t id);

  Future<bool> ClearPendingContributions();

  Future<std::vector<PublisherActivity>> GetPublisherActivity();

  Future<bool> ResetPublisherActivity();

  Future<std::vector<RecurringContribution>> GetRecurringContributions();

  Future<base::Time> GetLastScheduledContributionTime();

  Future<bool> UpdateLastScheduledContributionTime();
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_STORE_H_
