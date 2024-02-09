/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/contribution/contribution_tip.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/common/callback_helpers.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal::contribution {

ContributionTip::ContributionTip(RewardsEngineImpl& engine) : engine_(engine) {}

ContributionTip::~ContributionTip() = default;

void ContributionTip::Process(const std::string& publisher_id,
                              double amount,
                              ProcessCallback callback) {
  if (publisher_id.empty()) {
    engine_->LogError(FROM_HERE)
        << "Failed to do tip due to missing publisher key";
    std::move(callback).Run(std::nullopt);
    return;
  }

  engine_->publisher()->GetServerPublisherInfo(
      publisher_id,
      ToLegacyCallback(base::BindOnce(&ContributionTip::OnPublisherDataRead,
                                      base::Unretained(this), publisher_id,
                                      amount, std::move(callback))));
}

void ContributionTip::OnPublisherDataRead(
    const std::string& publisher_id,
    double amount,
    ProcessCallback callback,
    mojom::ServerPublisherInfoPtr server_info) {
  if (!server_info || server_info->address.empty()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  std::string queue_id = base::Uuid::GenerateRandomV4().AsLowercaseString();

  std::vector<mojom::ContributionQueuePublisherPtr> queue_list;
  auto publisher = mojom::ContributionQueuePublisher::New();
  publisher->publisher_key = publisher_id;
  publisher->amount_percent = 100.0;
  queue_list.push_back(std::move(publisher));

  auto queue = mojom::ContributionQueue::New();
  queue->id = queue_id;
  queue->type = mojom::RewardsType::ONE_TIME_TIP;
  queue->amount = amount;
  queue->partial = false;
  queue->publishers = std::move(queue_list);

  engine_->database()->SaveContributionQueue(
      std::move(queue),
      ToLegacyCallback(
          base::BindOnce(&ContributionTip::OnQueueSaved, base::Unretained(this),
                         std::move(queue_id), std::move(callback))));
}

void ContributionTip::OnQueueSaved(const std::string& queue_id,
                                   ProcessCallback callback,
                                   mojom::Result result) {
  if (result == mojom::Result::OK) {
    engine_->contribution()->ProcessContributionQueue();
    std::move(callback).Run(queue_id);
  } else {
    engine_->LogError(FROM_HERE) << "Queue was not saved";
    std::move(callback).Run(std::nullopt);
  }
}

}  // namespace brave_rewards::internal::contribution
