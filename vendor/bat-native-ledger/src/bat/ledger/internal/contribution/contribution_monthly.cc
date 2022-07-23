/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "bat/ledger/internal/contribution/contribution_monthly.h"
#include "bat/ledger/internal/contribution/contribution_monthly_util.h"
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
    type::PublisherInfoList list,
    ledger::LegacyResultCallback callback) {
  type::PublisherInfoList verified_list;
  GetVerifiedTipList(list, &verified_list);

  type::ContributionQueuePtr queue;
  type::ContributionQueuePublisherPtr publisher;
  for (const auto &item : verified_list) {
    type::ContributionQueuePublisherList queue_list;
    publisher = type::ContributionQueuePublisher::New();
    publisher->publisher_key = item->id;
    publisher->amount_percent = 100.0;
    queue_list.push_back(std::move(publisher));

    queue = type::ContributionQueue::New();
    queue->id = base::GenerateGUID();
    queue->type = type::RewardsType::RECURRING_TIP;
    queue->amount = item->weight;
    queue->partial = false;
    queue->publishers = std::move(queue_list);

    ledger_->database()->SaveContributionQueue(
        std::move(queue),
        [](const type::Result _){});
  }

  // TODO(https://github.com/brave/brave-browser/issues/8804):
  // we should change this logic and do batch insert with callback
  ledger_->contribution()->CheckContributionQueue();
  callback(type::Result::LEDGER_OK);
}

void ContributionMonthly::GetVerifiedTipList(
    const type::PublisherInfoList& list,
    type::PublisherInfoList* verified_list) {
  DCHECK(verified_list);
  type::PendingContributionList non_verified;

  for (const auto& publisher : list) {
    if (!publisher || publisher->id.empty() || publisher->weight == 0.0) {
      continue;
    }

    if (publisher->status != type::PublisherStatus::NOT_VERIFIED) {
      verified_list->push_back(publisher->Clone());
      continue;
    }

    auto contribution = type::PendingContribution::New();
    contribution->amount = publisher->weight;
    contribution->publisher_key = publisher->id;
    contribution->viewing_id = "";
    contribution->type = type::RewardsType::RECURRING_TIP;

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
    const type::Result result) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Problem saving pending");
  }

  ledger_->ledger_client()->PendingContributionSaved(result);
}

void ContributionMonthly::HasSufficientBalance(
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  auto fetch_callback =
      base::BindOnce(&ContributionMonthly::OnSufficientBalanceWallet,
                     base::Unretained(this), std::move(callback));

  ledger_->wallet()->FetchBalance(std::move(fetch_callback));
}

void ContributionMonthly::OnSufficientBalanceWallet(
    ledger::HasSufficientBalanceToReconcileCallback callback,
    const type::Result result,
    type::BalancePtr info) {
  if (result != type::Result::LEDGER_OK || !info) {
    BLOG(0, "Problem getting balance");
    return;
  }

  auto tips_callback = std::bind(&ContributionMonthly::OnHasSufficientBalance,
      this,
      _1,
      info->total,
      callback);

  ledger_->contribution()->GetRecurringTips(tips_callback);
}

void ContributionMonthly::OnHasSufficientBalance(
    const type::PublisherInfoList& publisher_list,
    const double balance,
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  if (publisher_list.empty()) {
    BLOG(1, "Publisher list is empty");
    callback(true);
    return;
  }

  const auto total = GetTotalFromVerifiedTips(publisher_list);
  callback(balance >= total);
}

}  // namespace contribution
}  // namespace ledger
