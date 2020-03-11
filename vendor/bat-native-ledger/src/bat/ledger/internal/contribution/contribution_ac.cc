/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/contribution/contribution_ac.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_contribution {

ContributionAC::ContributionAC(bat_ledger::LedgerImpl* ledger,
    Contribution* contribution) :
    ledger_(ledger),
    contribution_(contribution) {
  DCHECK(ledger_ && contribution_);
}

ContributionAC::~ContributionAC() = default;

void ContributionAC::Process() {
  if (!ledger_->GetRewardsMainEnabled() || !ledger_->GetAutoContribute()) {
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO) << "Staring auto contribution";

  auto filter = ledger_->CreateActivityFilter(
      "",
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      ledger_->GetReconcileStamp(),
      false,
      ledger_->GetPublisherMinVisits());

  auto get_callback = std::bind(&ContributionAC::PreparePublisherList,
      this,
      _1);

  ledger_->GetActivityInfoList(0, 0, std::move(filter), get_callback);
  contribution_->ResetReconcileStamp();
}

void ContributionAC::PreparePublisherList(ledger::PublisherInfoList list) {
  ledger::PublisherInfoList normalized_list;

  ledger_->NormalizeContributeWinners(&normalized_list, &list, 0);

  if (normalized_list.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_INFO) << "AC list is empty";
    return;
  }

  ledger::ContributionQueuePublisherList queue_list;
  ledger::ContributionQueuePublisherPtr publisher;
  for (const auto &item : normalized_list) {
    if (!item || item->percent == 0) {
      continue;
    }

    publisher = ledger::ContributionQueuePublisher::New();
    publisher->publisher_key = item->id;
    publisher->amount_percent =  item->weight;
    queue_list.push_back(std::move(publisher));
  }

  if (queue_list.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_INFO) << "AC queue list is empty";
    return;
  }

  auto queue = ledger::ContributionQueue::New();
  queue->type = ledger::RewardsType::AUTO_CONTRIBUTE;
  queue->amount = ledger_->GetContributionAmount();
  queue->partial = true;
  queue->publishers = std::move(queue_list);

  auto save_callback = std::bind(&ContributionAC::QueueSaved,
      this,
      _1);

  ledger_->SaveContributionQueue(std::move(queue), save_callback);
}

void ContributionAC::QueueSaved(const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_INFO) << "Queue was not saved";
    return;
  }

  contribution_->CheckContributionQueue();
}

}  // namespace braveledger_contribution
