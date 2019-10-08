/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/unverified.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_contribution {

Unverified::Unverified(bat_ledger::LedgerImpl* ledger,
    Contribution* contribution) :
    ledger_(ledger),
    contribution_(contribution),
    unverified_publishers_timer_id_(0u) {
}

Unverified::~Unverified() {
}

void Unverified::Contribute() {
  ledger_->FetchBalance(
      std::bind(&Unverified::OnContributeUnverifiedBalance,
                this,
                _1,
                _2));
}

void Unverified::OnContributeUnverifiedBalance(
    ledger::Result result,
    ledger::BalancePtr properties) {
  if (result != ledger::Result::LEDGER_OK || !properties) {
    return;
  }

  ledger_->GetPendingContributions(
      std::bind(&Unverified::OnContributeUnverifiedPublishers,
                this,
                properties->total,
                _1));
}

void Unverified::OnContributeUnverifiedPublishers(
    double balance,
    const ledger::PendingContributionInfoList& list) {
  if (list.empty()) {
    return;
  }

  if (balance == 0) {
    ledger_->OnContributeUnverifiedPublishers(
        ledger::Result::PENDING_NOT_ENOUGH_FUNDS);
    return;
  }

  base::Time now = base::Time::Now();
  double now_seconds = now.ToDoubleT();

  ledger::PendingContributionInfoPtr current;

  for (const auto& item : list) {
    // remove pending contribution if it's over expiration date
    if (now_seconds > item->expiration_date) {
      ledger_->RemovePendingContribution(
          item->publisher_key,
          item->viewing_id,
          item->added_date,
          std::bind(&Unverified::OnRemovePendingContribution,
                    this,
                    _1));
      continue;
    }

    // verified status didn't change
    if (item->status != ledger::PublisherStatus::VERIFIED) {
      continue;
    }

    if (!current) {
      current = item->Clone();
    }
  }

  if (!current) {
    return;
  }

  if (!ledger_->WasPublisherAlreadyProcessed(current->publisher_key)) {
    ledger_->OnContributeUnverifiedPublishers(
        ledger::Result::VERIFIED_PUBLISHER,
        current->publisher_key,
        current->name);
    ledger_->SavePublisherProcessed(current->publisher_key);
  }

  // Trigger contribution
  if (balance >= current->amount) {
    auto direction = braveledger_bat_helper::RECONCILE_DIRECTION(
        current->publisher_key,
        current->amount,
        "BAT");

    auto direction_list = std::vector
        <braveledger_bat_helper::RECONCILE_DIRECTION> { direction };
    contribution_->InitReconcile(
        ledger::RewardsType::ONE_TIME_TIP,
        {},
        direction_list);

    ledger_->RemovePendingContribution(
        current->publisher_key,
        current->viewing_id,
        current->added_date,
        std::bind(&Unverified::OnRemovePendingContribution,
                  this,
                  _1));

    if (ledger::is_testing) {
      contribution_->SetTimer(&unverified_publishers_timer_id_, 1);
    } else {
      contribution_->SetTimer(&unverified_publishers_timer_id_);
    }
  } else {
    ledger_->OnContributeUnverifiedPublishers(
        ledger::Result::PENDING_NOT_ENOUGH_FUNDS);
  }
}

void Unverified::OnRemovePendingContribution(
    ledger::Result result) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->OnContributeUnverifiedPublishers(
        ledger::Result::PENDING_PUBLISHER_REMOVED);
  }
}

void Unverified::OnTimer(uint32_t timer_id) {
  if (timer_id == unverified_publishers_timer_id_) {
    unverified_publishers_timer_id_ = 0;
    Contribute();
    return;
  }
}

}  // namespace braveledger_contribution
