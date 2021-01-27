/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/contribution/unverified.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace contribution {

Unverified::Unverified(LedgerImpl* ledger) :
    ledger_(ledger) {
}

Unverified::~Unverified() = default;

void Unverified::Contribute() {
  ledger_->wallet()->FetchBalance(
      std::bind(&Unverified::OnContributeUnverifiedBalance,
                this,
                _1,
                _2));
}

void Unverified::OnContributeUnverifiedBalance(
    type::Result result,
    type::BalancePtr properties) {
  if (result != type::Result::LEDGER_OK || !properties) {
    BLOG(0, "Balance is null");
    return;
  }

  ledger_->database()->GetPendingContributions(
      std::bind(&Unverified::OnContributeUnverifiedPublishers,
                this,
                properties->total,
                _1));
}

void Unverified::OnContributeUnverifiedPublishers(
    double balance,
    const type::PendingContributionInfoList& list) {
  if (list.empty()) {
    BLOG(1, "List is empty");
    return;
  }

  if (balance == 0) {
    BLOG(0, "Not enough funds");
    ledger_->ledger_client()->OnContributeUnverifiedPublishers(
        type::Result::PENDING_NOT_ENOUGH_FUNDS,
        "",
        "");
    return;
  }

  const auto now = util::GetCurrentTimeStamp();

  type::PendingContributionInfoPtr current;

  for (const auto& item : list) {
    // remove pending contribution if it's over expiration date
    if (now > item->expiration_date) {
      ledger_->database()->RemovePendingContribution(
          item->id,
          std::bind(&Unverified::OnRemovePendingContribution,
                    this,
                    _1));
      continue;
    }

    // verified status didn't change
    if (item->status != type::PublisherStatus::UPHOLD_VERIFIED &&
        item->status != type::PublisherStatus::BITFLYER_VERIFIED) {
      continue;
    }

    if (!current) {
      current = item->Clone();
    }
  }

  if (!current) {
    BLOG(1, "Nothing to process");
    return;
  }

  auto get_callback = std::bind(&Unverified::WasPublisherProcessed,
      this,
      _1,
      current->publisher_key,
      current->name);

  ledger_->database()->WasPublisherProcessed(
      current->publisher_key,
      get_callback);

  if (balance < current->amount) {
    BLOG(0, "Not enough funds");
    ledger_->ledger_client()->OnContributeUnverifiedPublishers(
        type::Result::PENDING_NOT_ENOUGH_FUNDS,
        "",
        "");
    return;
  }

  type::ContributionQueuePublisherList queue_list;
  auto publisher = type::ContributionQueuePublisher::New();
  publisher->publisher_key = current->publisher_key;
  publisher->amount_percent = 100.0;
  queue_list.push_back(std::move(publisher));

  auto queue = type::ContributionQueue::New();
  queue->id = base::GenerateGUID();
  queue->type = type::RewardsType::ONE_TIME_TIP;
  queue->amount = current->amount;
  queue->partial = false;
  queue->publishers = std::move(queue_list);

  auto save_callback = std::bind(&Unverified::QueueSaved,
      this,
      _1,
      current->id);

  ledger_->database()->SaveContributionQueue(std::move(queue), save_callback);
}

void Unverified::QueueSaved(
    const type::Result result,
    const uint64_t pending_contribution_id) {
  if (result == type::Result::LEDGER_OK) {
    ledger_->database()->RemovePendingContribution(
      pending_contribution_id,
      std::bind(&Unverified::OnRemovePendingContribution,
                this,
                _1));

    ledger_->contribution()->ProcessContributionQueue();
  } else {
    BLOG(1, "Queue was not saved");
  }

  base::TimeDelta delay = ledger::is_testing
      ? base::TimeDelta::FromSeconds(2)
      : util::GetRandomizedDelay(
          base::TimeDelta::FromSeconds(45));

  BLOG(1, "Unverified contribution timer set for " << delay);

  unverified_publishers_timer_.Start(FROM_HERE, delay,
      base::BindOnce(&Unverified::Contribute, base::Unretained(this)));
}

void Unverified::WasPublisherProcessed(
    const type::Result result,
    const std::string& publisher_key,
    const std::string& name) {
  if (result == type::Result::LEDGER_ERROR) {
    BLOG(0, "Couldn't get processed data");
    return;
  }

  if (result == type::Result::LEDGER_OK) {
    BLOG(1, "Publisher already processed");
    // Nothing to do here as publisher was already processed
    return;
  }

  auto save_callback = std::bind(&Unverified::ProcessedPublisherSaved,
      this,
      _1,
      publisher_key,
      name);
  ledger_->database()->SaveProcessedPublisherList(
      {publisher_key},
      save_callback);
}

void Unverified::ProcessedPublisherSaved(
    const type::Result result,
    const std::string& publisher_key,
    const std::string& name) {
  ledger_->ledger_client()->OnContributeUnverifiedPublishers(
      type::Result::VERIFIED_PUBLISHER,
      publisher_key,
      name);
}

void Unverified::OnRemovePendingContribution(
    type::Result result) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Problem removing pending contribution");
    return;
  }

  ledger_->ledger_client()->OnContributeUnverifiedPublishers(
      type::Result::PENDING_PUBLISHER_REMOVED,
      "",
      "");
}

}  // namespace contribution
}  // namespace ledger
