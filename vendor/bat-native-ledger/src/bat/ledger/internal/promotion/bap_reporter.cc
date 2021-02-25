/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/promotion/bap_reporter.h"

#include "base/time/time.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/option_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace promotion {

namespace {

constexpr int64_t kRetryDelay = 24 * base::Time::kSecondsPerHour;
constexpr int64_t kRetryAfterFailureDelay = 10 * base::Time::kSecondsPerMinute;
constexpr int64_t kMaxRetryAfterFailureDelay = 4 * base::Time::kSecondsPerHour;

}  // namespace

BAPReporter::BAPReporter(LedgerImpl* ledger)
    : ledger_(ledger), endpoint_(ledger) {
  DCHECK(ledger);
}

BAPReporter::~BAPReporter() = default;

void BAPReporter::ReportBAPAmount() {
  if (running_)
    return;

  timer_.Stop();

  bool should_report = ledger_->ledger_client()->GetBooleanOption(
      option::kShouldReportBAPAmount);

  // Only run this reporter if the user is in a BAP region and we haven't
  // successfully reported yet.
  if (!should_report || ledger_->state()->GetBAPReported())
    return;

  running_ = true;

  // First, get the user's unspent BAP tokens.
  ledger_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {type::CredsBatchType::PROMOTION},
      std::bind(&BAPReporter::OnUnblindedTokens, this, _1));
}

void BAPReporter::OnUnblindedTokens(
    std::vector<type::UnblindedTokenPtr> tokens) {
  double amount = 0;
  for (const auto& token : tokens)
    amount += token->value;

  // If the user has no BAP, then schedule a retry and exit.
  if (amount <= 0) {
    ScheduleRetryAfterZeroBalance();
    return;
  }

  // Send the amount to the server.
  endpoint_.Request(amount,
                    std::bind(&BAPReporter::OnEndpointResponse, this, _1));
}

void BAPReporter::OnEndpointResponse(bool success) {
  // If the server reported an error, assume a temporary problem and try again
  // later.
  if (!success) {
    ScheduleRetryAfterFailure();
    return;
  }

  BLOG(1, "BAP successsfully reported");

  // Set a flag to indicate that we don't need to report again.
  ledger_->state()->SetBAPReported(true);
  running_ = false;
  retry_count_ = 0;
}

void BAPReporter::ScheduleRetryAfterZeroBalance() {
  running_ = false;

  base::TimeDelta delay = base::TimeDelta::FromSeconds(kRetryDelay);

  BLOG(1, "User has zero balance - rescheduling BAP reporting in " << delay);
  timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&BAPReporter::ReportBAPAmount, base::Unretained(this)));
}

void BAPReporter::ScheduleRetryAfterFailure() {
  running_ = false;

  base::TimeDelta delay = util::GetRandomizedDelayWithBackoff(
      base::TimeDelta::FromSeconds(kRetryAfterFailureDelay),
      base::TimeDelta::FromSeconds(kMaxRetryAfterFailureDelay), retry_count_++);

  BLOG(1, "BAP reporting failed - rescheduling in " << delay);
  timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&BAPReporter::ReportBAPAmount, base::Unretained(this)));
}

}  // namespace promotion
}  // namespace ledger
