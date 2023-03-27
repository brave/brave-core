/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/containers/cxx20_erase.h"
#include "base/functional/bind.h"
#include "base/guid.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/contribution/contribution_monthly.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::core {
namespace contribution {

ContributionMonthly::ContributionMonthly(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

ContributionMonthly::~ContributionMonthly() = default;

void ContributionMonthly::Process(absl::optional<base::Time> cutoff_time,
                                  LegacyResultCallback callback) {
  ledger_->contribution()->GetRecurringTips(
      [this, cutoff_time,
       callback](std::vector<mojom::PublisherInfoPtr> publishers) {
        AdvanceContributionDates(cutoff_time, callback, std::move(publishers));
      });
}

void ContributionMonthly::AdvanceContributionDates(
    absl::optional<base::Time> cutoff_time,
    LegacyResultCallback callback,
    std::vector<mojom::PublisherInfoPtr> publishers) {
  // Remove any contributions whose next contribution date is in the future.
  base::EraseIf(publishers,
                [cutoff_time](const mojom::PublisherInfoPtr& publisher) {
                  if (!publisher || publisher->id.empty()) {
                    return true;
                  }
                  base::Time next_contribution = base::Time::FromDoubleT(
                      static_cast<double>(publisher->reconcile_stamp));
                  return cutoff_time && next_contribution > cutoff_time;
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
    LegacyResultCallback callback,
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
}  // namespace brave_rewards::core
