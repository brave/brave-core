/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/contribution/contribution_tip.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_contribution {

ContributionTip::ContributionTip(bat_ledger::LedgerImpl* ledger,
    Contribution* contribution) :
    ledger_(ledger),
    contribution_(contribution) {
  DCHECK(ledger_ && contribution_);
}

ContributionTip::~ContributionTip() = default;

void ContributionTip::Process(
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  if (publisher_key.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Failed to do tip due to missing publisher key";
    callback(ledger::Result::NOT_FOUND);
    return;
  }

  const auto server_callback = std::bind(&ContributionTip::ServerPublisher,
      this,
      _1,
      publisher_key,
      amount,
      callback);

  ledger_->GetServerPublisherInfo(publisher_key, server_callback);
}

void ContributionTip::ServerPublisher(
    ledger::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  auto status = ledger::PublisherStatus::NOT_VERIFIED;
  if (server_info) {
    status = server_info->status;
  }

  // Save to the pending list if not verified
  if (status == ledger::PublisherStatus::NOT_VERIFIED) {
    auto save_callback = std::bind(&ContributionTip::OnSavePending,
        this,
        _1,
        callback);

    SavePending(
        publisher_key,
        amount,
        save_callback);
    return;
  }

  ledger::ContributionQueuePublisherList queue_list;
  auto publisher = ledger::ContributionQueuePublisher::New();
  publisher->publisher_key = publisher_key;
  publisher->amount_percent = 100.0;
  queue_list.push_back(std::move(publisher));

  auto queue = ledger::ContributionQueue::New();
  queue->type = ledger::RewardsType::ONE_TIME_TIP;
  queue->amount = amount;
  queue->partial = false;
  queue->publishers = std::move(queue_list);

  contribution_->Start(std::move(queue));
  callback(ledger::Result::LEDGER_OK);
}

void ContributionTip::SavePending(
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  auto contribution = ledger::PendingContribution::New();
  contribution->publisher_key = publisher_key;
  contribution->amount = amount;
  contribution->type = ledger::RewardsType::ONE_TIME_TIP;

  ledger::PendingContributionList list;
  list.push_back(std::move(contribution));

  ledger_->SavePendingContribution(std::move(list), callback);
}

void ContributionTip::OnSavePending(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)<< "Pending tip save failed";
  }

  ledger_->PendingContributionSaved(result);
  callback(result);
}

}  // namespace braveledger_contribution
