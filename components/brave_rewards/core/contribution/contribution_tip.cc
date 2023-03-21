/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/contribution/contribution_tip.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/guid.h"
#include "brave/components/brave_rewards/core/common/legacy_callback_helpers.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"

namespace ledger::contribution {

ContributionTip::ContributionTip(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

ContributionTip::~ContributionTip() = default;

void ContributionTip::Process(const std::string& publisher_id,
                              double amount,
                              ProcessCallback callback) {
  if (publisher_id.empty()) {
    BLOG(0, "Failed to do tip due to missing publisher key");
    std::move(callback).Run(absl::nullopt);
    return;
  }

  ledger_->publisher()->GetServerPublisherInfo(
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
  auto status = mojom::PublisherStatus::NOT_VERIFIED;
  if (server_info) {
    status = server_info->status;
  }

  // If the publisher is not registered or is not verified, add an entry to the
  // pending contribution table.
  if (status == mojom::PublisherStatus::NOT_VERIFIED) {
    BLOG(1, "Saving pending publisher " << publisher_id);

    auto pending = mojom::PendingContribution::New();
    pending->publisher_key = publisher_id;
    pending->amount = amount;
    pending->type = mojom::RewardsType::ONE_TIME_TIP;

    std::vector<mojom::PendingContributionPtr> list;
    list.push_back(std::move(pending));

    ledger_->database()->SavePendingContribution(
        std::move(list),
        ToLegacyCallback(base::BindOnce(&ContributionTip::OnPendingTipSaved,
                                        base::Unretained(this))));

    std::move(callback).Run(absl::nullopt);
    return;
  }

  std::string queue_id = base::GenerateGUID();

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

  ledger_->database()->SaveContributionQueue(
      std::move(queue),
      ToLegacyCallback(
          base::BindOnce(&ContributionTip::OnQueueSaved, base::Unretained(this),
                         std::move(queue_id), std::move(callback))));
}

void ContributionTip::OnQueueSaved(const std::string& queue_id,
                                   ProcessCallback callback,
                                   mojom::Result result) {
  if (result == mojom::Result::LEDGER_OK) {
    ledger_->contribution()->ProcessContributionQueue();
    std::move(callback).Run(queue_id);
  } else {
    BLOG(0, "Queue was not saved");
    std::move(callback).Run(absl::nullopt);
  }
}

void ContributionTip::OnPendingTipSaved(mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Pending tip save failed");
  } else {
    ledger_->client()->PendingContributionSaved(result);
  }
}

}  // namespace ledger::contribution
