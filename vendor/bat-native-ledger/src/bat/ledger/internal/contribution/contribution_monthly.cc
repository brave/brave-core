/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "bat/ledger/internal/contribution/contribution_monthly.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace contribution {

ContributionMonthly::ContributionMonthly(LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

ContributionMonthly::~ContributionMonthly() = default;

void ContributionMonthly::Process(ledger::LegacyResultCallback callback) {
  auto get_callback = std::bind(&ContributionMonthly::PrepareTipList,
      this,
      _1,
      callback);

  ledger_->contribution()->GetRecurringTips(get_callback);
}

void ContributionMonthly::PrepareTipList(
    std::vector<mojom::PublisherInfoPtr> list,
    ledger::LegacyResultCallback callback) {
  std::vector<mojom::PublisherInfoPtr> verified_list;
  GetVerifiedTipList(list, &verified_list);

  mojom::ContributionQueuePtr queue;
  mojom::ContributionQueuePublisherPtr publisher;
  for (const auto &item : verified_list) {
    std::vector<mojom::ContributionQueuePublisherPtr> queue_list;
    publisher = mojom::ContributionQueuePublisher::New();
    publisher->publisher_key = item->id;
    publisher->amount_percent = 100.0;
    queue_list.push_back(std::move(publisher));

    queue = mojom::ContributionQueue::New();
    queue->id = base::GenerateGUID();
    queue->type = mojom::RewardsType::RECURRING_TIP;
    queue->amount = item->weight;
    queue->partial = false;
    queue->publishers = std::move(queue_list);

    ledger_->database()->SaveContributionQueue(std::move(queue),
                                               [](const mojom::Result _) {});
  }

  // TODO(https://github.com/brave/brave-browser/issues/8804):
  // we should change this logic and do batch insert with callback
  ledger_->contribution()->CheckContributionQueue();
  callback(mojom::Result::LEDGER_OK);
}

void ContributionMonthly::GetVerifiedTipList(
    const std::vector<mojom::PublisherInfoPtr>& list,
    std::vector<mojom::PublisherInfoPtr>* verified_list) {
  DCHECK(verified_list);
  std::vector<mojom::PendingContributionPtr> non_verified;

  for (const auto& publisher : list) {
    if (!publisher || publisher->id.empty() || publisher->weight == 0.0) {
      continue;
    }

    if (publisher->status != mojom::PublisherStatus::NOT_VERIFIED) {
      verified_list->push_back(publisher->Clone());
      continue;
    }

    auto contribution = mojom::PendingContribution::New();
    contribution->amount = publisher->weight;
    contribution->publisher_key = publisher->id;
    contribution->viewing_id = "";
    contribution->type = mojom::RewardsType::RECURRING_TIP;

    non_verified.push_back(std::move(contribution));
  }

  if (non_verified.empty()) {
    return;
  }

  auto save_callback = std::bind(
      &ContributionMonthly::OnSavePendingContribution,
      this,
      _1);
  ledger_->database()->SavePendingContribution(
      std::move(non_verified),
      save_callback);
}

void ContributionMonthly::OnSavePendingContribution(
    const mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Problem saving pending");
  }

  ledger_->ledger_client()->PendingContributionSaved(result);
}

}  // namespace contribution
}  // namespace ledger
