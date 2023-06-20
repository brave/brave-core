/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/uuid.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/contribution/contribution_ac.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/state/state.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace brave_rewards::internal::contribution {

void ContributionAC::Process(const uint64_t reconcile_stamp) {
  if (!ledger().state()->GetAutoContributeEnabled()) {
    BLOG(1, "Auto contribution is off");
    return;
  }

  BLOG(1, "Starting auto contribution");

  auto filter = ledger().publisher()->CreateActivityFilter(
      "", mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED, true,
      reconcile_stamp, false, ledger().state()->GetPublisherMinVisits());

  auto get_callback =
      std::bind(&ContributionAC::PreparePublisherList, this, _1);

  ledger().database()->GetActivityInfoList(0, 0, std::move(filter),
                                           get_callback);
}

void ContributionAC::PreparePublisherList(
    std::vector<mojom::PublisherInfoPtr> list) {
  std::vector<mojom::PublisherInfoPtr> normalized_list;

  ledger().publisher()->NormalizeContributeWinners(&normalized_list, &list, 0);

  if (normalized_list.empty()) {
    BLOG(1, "AC list is empty");
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
    BLOG(1, "AC queue list is empty");
    return;
  }

  auto queue = mojom::ContributionQueue::New();
  queue->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  queue->type = mojom::RewardsType::AUTO_CONTRIBUTE;
  queue->amount = ledger().state()->GetAutoContributionAmount();
  queue->partial = true;
  queue->publishers = std::move(queue_list);

  ledger().database()->SaveEventLog(log::kACAddedToQueue,
                                    std::to_string(queue->amount));

  auto save_callback = std::bind(&ContributionAC::QueueSaved, this, _1);

  ledger().database()->SaveContributionQueue(std::move(queue), save_callback);
}

void ContributionAC::QueueSaved(const mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Queue was not saved");
    return;
  }

  ledger().contribution()->CheckContributionQueue();
}

}  // namespace brave_rewards::internal::contribution
