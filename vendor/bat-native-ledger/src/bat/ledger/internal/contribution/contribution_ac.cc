/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "bat/ledger/internal/contribution/contribution_ac.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_contribution {

ContributionAC::ContributionAC(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

ContributionAC::~ContributionAC() = default;

void ContributionAC::Process(const uint64_t reconcile_stamp) {
  if (!ledger_->state()->GetRewardsMainEnabled() ||
      !ledger_->state()->GetAutoContributeEnabled()) {
    BLOG(1, "Auto contribution is off");
    return;
  }

  BLOG(1, "Starting auto contribution");

  auto filter = ledger_->publisher()->CreateActivityFilter(
      "",
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      reconcile_stamp,
      false,
      ledger_->state()->GetPublisherMinVisits());

  auto get_callback = std::bind(&ContributionAC::PreparePublisherList,
      this,
      _1);

  ledger_->database()->GetActivityInfoList(
      0,
      0,
      std::move(filter),
      get_callback);
}

void ContributionAC::PreparePublisherList(ledger::PublisherInfoList list) {
  ledger::PublisherInfoList normalized_list;

  ledger_->publisher()->NormalizeContributeWinners(&normalized_list, &list, 0);

  if (normalized_list.empty()) {
    BLOG(1, "AC list is empty");
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
    BLOG(1, "AC queue list is empty");
    return;
  }

  auto queue = ledger::ContributionQueue::New();
  queue->id = base::GenerateGUID();
  queue->type = ledger::RewardsType::AUTO_CONTRIBUTE;
  queue->amount = ledger_->state()->GetAutoContributionAmount();
  queue->partial = true;
  queue->publishers = std::move(queue_list);

  ledger_->database()->SaveEventLog(
      ledger::log::kACAddedToQueue,
      std::to_string(queue->amount));

  auto save_callback = std::bind(&ContributionAC::QueueSaved,
      this,
      _1);

  ledger_->database()->SaveContributionQueue(std::move(queue), save_callback);
}

void ContributionAC::QueueSaved(const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Queue was not saved");
    return;
  }

  ledger_->contribution()->CheckContributionQueue();
}

}  // namespace braveledger_contribution
