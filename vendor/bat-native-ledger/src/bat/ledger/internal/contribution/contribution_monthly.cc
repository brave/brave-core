/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/containers/cxx20_erase.h"
#include "base/functional/bind.h"
#include "base/guid.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/contribution/contribution_monthly.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace contribution {

ContributionMonthly::ContributionMonthly(LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

ContributionMonthly::~ContributionMonthly() = default;

void ContributionMonthly::Process(base::Time cutoff_time,
                                  ledger::LegacyResultCallback callback) {
  ledger_->contribution()->GetRecurringTips(
      [this, cutoff_time,
       callback](std::vector<mojom::PublisherInfoPtr> publishers) {
        AdvanceContributionDates(cutoff_time, callback, std::move(publishers));
      });
}

void ContributionMonthly::AdvanceContributionDates(
    base::Time cutoff_time,
    ledger::LegacyResultCallback callback,
    std::vector<mojom::PublisherInfoPtr> publishers) {
  // A null cutoff time indicates that all monthly contributions should be sent,
  // regardless of their next contribution date. This should only be used in
  // tests.
  if (cutoff_time.is_null()) {
    cutoff_time = base::Time::Max();
  }

  // Remove any contributions whose next contribution date is in the future.
  uint64_t now = static_cast<uint64_t>(cutoff_time.ToDoubleT());
  base::EraseIf(publishers, [now](const mojom::PublisherInfoPtr& publisher) {
    return !publisher || publisher->id.empty() ||
           publisher->reconcile_stamp > now;
  });

  std::vector<std::string> publisher_ids;
  for (const auto& publisher_info : publishers) {
    publisher_ids.push_back(publisher_info->id);
  }

  // Advance the next contribution dates before attempting to add contributions.
  ledger_->database()->AdvanceMonthlyContributionDates(
      publisher_ids,
      base::BindOnce(&ContributionMonthly::OnNextContributionDateAdvanced,
                     base::Unretained(this), std::move(publishers), callback));
}

void ContributionMonthly::OnNextContributionDateAdvanced(
    std::vector<mojom::PublisherInfoPtr> publishers,
    ledger::LegacyResultCallback callback,
    bool success) {
  if (!success) {
    BLOG(0, "Unable to advance monthly contribution dates.");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  // Remove entries for zero contribution amounts or unverified creators. Note
  // that in previous versions, pending contributions would be created if the
  // creator was unverified.
  base::EraseIf(publishers, [](const mojom::PublisherInfoPtr& publisher) {
    DCHECK(publisher);
    return publisher->weight <= 0 ||
           publisher->status == mojom::PublisherStatus::NOT_VERIFIED;
  });

  for (const auto& item : publishers) {
    auto publisher = mojom::ContributionQueuePublisher::New();
    publisher->publisher_key = item->id;
    publisher->amount_percent = 100.0;

    auto queue = mojom::ContributionQueue::New();
    queue->id = base::GenerateGUID();
    queue->type = mojom::RewardsType::RECURRING_TIP;
    queue->amount = item->weight;
    queue->partial = false;
    queue->publishers.push_back(std::move(publisher));

    ledger_->database()->SaveContributionQueue(std::move(queue),
                                               [](mojom::Result) {});
  }

  ledger_->contribution()->CheckContributionQueue();
  callback(mojom::Result::LEDGER_OK);
}

}  // namespace contribution
}  // namespace ledger
