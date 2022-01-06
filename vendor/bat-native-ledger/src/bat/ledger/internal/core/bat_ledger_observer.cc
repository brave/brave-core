/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/bat_ledger_observer.h"

#include <utility>

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

namespace {

// Returns a completed ContributionInfo record for submission to the client via
// "OnReconcileComplete". Clients currently only use a small number of these
// fields, and so we can use arbitrary data for most of them. In the future, we
// will replace the "OnReconcileComplete" observation method and this shim will
// no longer be necessary.
mojom::ContributionInfoPtr MakeContributionInfo() {
  auto contribution_info = mojom::ContributionInfo::New();
  contribution_info->contribution_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  contribution_info->type = mojom::RewardsType::ONE_TIME_TIP;
  contribution_info->step = mojom::ContributionStep::STEP_COMPLETED;
  contribution_info->created_at =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  contribution_info->processor = mojom::ContributionProcessor::BRAVE_TOKENS;
  return contribution_info;
}

}  // namespace

void BATLedgerObserver::OnAvailableBalanceUpdated() {
  // Currently, clients observe updates to the balance by listening for
  // "OnReconcileComplete" or "UnblindedTokensReady". In the future we will have
  // a single observation method that clients can use to respond to any change
  // in the available balance.
}

void BATLedgerObserver::OnContributionCompleted(double amount) {
  auto contribution_info = MakeContributionInfo();
  contribution_info->amount = amount;
  context().GetLedgerClient()->OnReconcileComplete(
      mojom::Result::LEDGER_OK, std::move(contribution_info));
}

void BATLedgerObserver::OnAutoContributeCompleted(double total_amount) {
  auto contribution_info = MakeContributionInfo();
  contribution_info->type = mojom::RewardsType::AUTO_CONTRIBUTE;
  contribution_info->amount = total_amount;
  context().GetLedgerClient()->OnReconcileComplete(
      mojom::Result::LEDGER_OK, std::move(contribution_info));
}

}  // namespace ledger
