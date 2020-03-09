/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UTIL_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UTIL_H_

#include <map>
#include <string>

#include "base/values.h"
#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/internal/properties/reconcile_direction_properties.h"

namespace braveledger_contribution {

ledger::ReconcileDirections
FromContributionQueuePublishersToReconcileDirections(
    ledger::ContributionQueuePublisherList list);

ledger::ReportType GetReportTypeFromRewardsType(const ledger::RewardsType type);

bool GenerateSuggestion(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& suggestion_encoded,
    base::Value* result);

bool GenerateSuggestionMock(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& suggestion_encoded,
    base::Value* result);

double GetTotalFromRecurringVerified(
    const ledger::PublisherInfoList& publisher_list);

}  // namespace braveledger_contribution

#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UTIL_H_
