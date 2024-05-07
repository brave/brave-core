/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/contribution/contribution_ac.h"

#include <utility>

#include "base/uuid.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::contribution {

ContributionAC::ContributionAC(RewardsEngine& engine) : engine_(engine) {}

ContributionAC::~ContributionAC() = default;

void ContributionAC::Process(const uint64_t reconcile_stamp) {
  if (!engine_->contribution()->GetAutoContributeEnabled()) {
    engine_->Log(FROM_HERE) << "Auto contribution is off";
    return;
  }

  engine_->Log(FROM_HERE) << "Starting auto contribution";

  int min_visits = engine_->Get<Prefs>().GetInteger(prefs::kMinVisits);

  auto filter = engine_->publisher()->CreateActivityFilter(
      "", mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED, true,
      reconcile_stamp, false, min_visits);

  engine_->database()->GetActivityInfoList(
      0, 0, std::move(filter),
      base::BindOnce(&ContributionAC::PreparePublisherList,
                     weak_factory_.GetWeakPtr()));
}

void ContributionAC::PreparePublisherList(
    std::vector<mojom::PublisherInfoPtr> list) {
  std::vector<mojom::PublisherInfoPtr> normalized_list;

  engine_->publisher()->NormalizeContributeWinners(&normalized_list, &list, 0);

  if (normalized_list.empty()) {
    engine_->Log(FROM_HERE) << "AC list is empty";
    return;
  }

  std::vector<mojom::ContributionQueuePublisherPtr> queue_list;
  mojom::ContributionQueuePublisherPtr publisher;
  for (const auto& item : normalized_list) {
    if (!item || item->percent == 0) {
      continue;
    }

    publisher = mojom::ContributionQueuePublisher::New();
    publisher->publisher_key = item->id;
    publisher->amount_percent = item->weight;
    queue_list.push_back(std::move(publisher));
  }

  if (queue_list.empty()) {
    engine_->Log(FROM_HERE) << "AC queue list is empty";
    return;
  }

  auto queue = mojom::ContributionQueue::New();
  queue->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  queue->type = mojom::RewardsType::AUTO_CONTRIBUTE;
  queue->amount = engine_->contribution()->GetAutoContributeAmount();
  queue->partial = true;
  queue->publishers = std::move(queue_list);

  engine_->database()->SaveEventLog(log::kACAddedToQueue,
                                    std::to_string(queue->amount));

  engine_->database()->SaveContributionQueue(
      std::move(queue),
      base::BindOnce(&ContributionAC::QueueSaved, weak_factory_.GetWeakPtr()));
}

void ContributionAC::QueueSaved(const mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Queue was not saved";
    return;
  }

  engine_->contribution()->CheckContributionQueue();
}

}  // namespace brave_rewards::internal::contribution
