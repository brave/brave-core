/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/contribution/contribution_monthly.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace contribution {

ContributionMonthly::ContributionMonthly(RewardsEngineImpl& engine)
    : engine_(engine) {}

ContributionMonthly::~ContributionMonthly() = default;

void ContributionMonthly::Process(std::optional<base::Time> cutoff_time,
                                  ResultCallback callback) {
  engine_->contribution()->GetRecurringTips(base::BindOnce(
      &ContributionMonthly::AdvanceContributionDates,
      weak_factory_.GetWeakPtr(), std::move(cutoff_time), std::move(callback)));
}

void ContributionMonthly::AdvanceContributionDates(
    std::optional<base::Time> cutoff_time,
    ResultCallback callback,
    std::vector<mojom::PublisherInfoPtr> publishers) {
  // Remove any contributions whose next contribution date is in the future.
  std::erase_if(publishers,
                [cutoff_time](const mojom::PublisherInfoPtr& publisher) {
                  if (!publisher || publisher->id.empty()) {
                    return true;
                  }
                  base::Time next_contribution =
                      base::Time::FromSecondsSinceUnixEpoch(
                          static_cast<double>(publisher->reconcile_stamp));
                  return cutoff_time && next_contribution > cutoff_time;
                });

  std::vector<std::string> publisher_ids;
  for (const auto& publisher_info : publishers) {
    publisher_ids.push_back(publisher_info->id);
  }

  // Advance the next contribution dates before attempting to add contributions.
  engine_->database()->AdvanceMonthlyContributionDates(
      publisher_ids,
      base::BindOnce(&ContributionMonthly::OnNextContributionDateAdvanced,
                     weak_factory_.GetWeakPtr(), std::move(publishers),
                     std::move(callback)));
}

void ContributionMonthly::OnNextContributionDateAdvanced(
    std::vector<mojom::PublisherInfoPtr> publishers,
    ResultCallback callback,
    bool success) {
  if (!success) {
    engine_->LogError(FROM_HERE)
        << "Unable to advance monthly contribution dates";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  // Remove entries for zero contribution amounts or unverified creators. Note
  // that in previous versions, pending contributions would be created if the
  // creator was unverified.
  std::erase_if(publishers, [](const mojom::PublisherInfoPtr& publisher) {
    DCHECK(publisher);
    return publisher->weight <= 0 ||
           publisher->status == mojom::PublisherStatus::NOT_VERIFIED;
  });

  engine_->Log(FROM_HERE) << "Sending " << publishers.size()
                          << " monthly contributions";

  for (const auto& item : publishers) {
    auto publisher = mojom::ContributionQueuePublisher::New();
    publisher->publisher_key = item->id;
    publisher->amount_percent = 100.0;

    auto queue = mojom::ContributionQueue::New();
    queue->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
    queue->type = mojom::RewardsType::RECURRING_TIP;
    queue->amount = item->weight;
    queue->partial = false;
    queue->publishers.push_back(std::move(publisher));

    engine_->database()->SaveContributionQueue(std::move(queue),
                                               base::DoNothing());
  }

  engine_->contribution()->CheckContributionQueue();
  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace contribution
}  // namespace brave_rewards::internal
