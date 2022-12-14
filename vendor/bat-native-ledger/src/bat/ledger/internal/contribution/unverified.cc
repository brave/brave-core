/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/numerics/safe_conversions.h"
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
  if (processing_start_time_) {
    BLOG(1, "Pending tips already processing");
    return;
  }
  BLOG(1, "Pending tips processing starting");
  processing_start_time_ = base::Time::Now();
  ledger_->database()->GetUnverifiedPublishersForPendingContributions(
      std::bind(&Unverified::FetchInfoForUnverifiedPublishers, this, _1));
}

void Unverified::FetchInfoForUnverifiedPublishers(
    std::vector<std::string>&& publisher_keys) {
  if (!publisher_keys.empty()) {
    const auto publisher_key = std::move(publisher_keys.back());
    ledger_->publisher()->FetchServerPublisherInfo(
        publisher_key, [this, publisher_keys = std::move(publisher_keys)](
                           mojom::ServerPublisherInfoPtr) mutable {
          publisher_keys.pop_back();
          FetchInfoForUnverifiedPublishers(std::move(publisher_keys));
        });
  } else {
    ProcessNext();
  }
}

void Unverified::ProcessNext() {
  DCHECK(processing_start_time_);
  ledger_->wallet()->FetchBalance(base::BindOnce(
      &Unverified::OnContributeUnverifiedBalance, base::Unretained(this)));
}

void Unverified::OnContributeUnverifiedBalance(mojom::Result result,
                                               mojom::BalancePtr properties) {
  if (result != mojom::Result::LEDGER_OK || !properties) {
    BLOG(0, "Balance is null");
    return ProcessingCompleted();
  }

  ledger_->database()->GetPendingContributions(
      std::bind(&Unverified::OnContributeUnverifiedPublishers,
                this,
                properties->total,
                _1));
}

void Unverified::OnContributeUnverifiedPublishers(
    double balance,
    const std::vector<mojom::PendingContributionInfoPtr>& list) {
  if (list.empty()) {
    BLOG(1, "List is empty");
    return ProcessingCompleted();
  }

  if (balance == 0) {
    BLOG(0, "Not enough funds");
    ledger_->ledger_client()->OnContributeUnverifiedPublishers(
        mojom::Result::PENDING_NOT_ENOUGH_FUNDS, "", "");
    return ProcessingCompleted();
  }

  // NOTE: `PendingContribution::added_date` is stored as a uint64_t of ms from
  // Unix epoch, with the subsecond interval truncated. For correct comparison
  // with that value, convert the processing start time to uin64_t and truncate.
  DCHECK(processing_start_time_);
  uint64_t processing_cutoff =
      base::ClampFloor<uint64_t>(processing_start_time_->ToDoubleT());

  const auto now = util::GetCurrentTimeStamp();

  mojom::PendingContributionInfoPtr current;

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

    // If the pending entry was added after we started processing, then skip it.
    if (item->added_date >= processing_cutoff) {
      continue;
    }

    // If the publisher is still not verified,
    // leave the contribution in the pending table.
    if (!ledger_->publisher()->IsVerified(item->status)) {
      continue;
    }

    if (!current) {
      current = item->Clone();
    }
  }

  if (!current) {
    BLOG(1, "Nothing to process");
    return ProcessingCompleted();
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
        mojom::Result::PENDING_NOT_ENOUGH_FUNDS, "", "");
    return ProcessingCompleted();
  }

  std::vector<mojom::ContributionQueuePublisherPtr> queue_list;
  auto publisher = mojom::ContributionQueuePublisher::New();
  publisher->publisher_key = current->publisher_key;
  publisher->amount_percent = 100.0;
  queue_list.push_back(std::move(publisher));

  auto queue = mojom::ContributionQueue::New();
  queue->id = base::GenerateGUID();
  queue->type = mojom::RewardsType::ONE_TIME_TIP;
  queue->amount = current->amount;
  queue->partial = false;
  queue->publishers = std::move(queue_list);

  auto save_callback = std::bind(&Unverified::QueueSaved,
      this,
      _1,
      current->id);

  ledger_->database()->SaveContributionQueue(std::move(queue), save_callback);
}

void Unverified::QueueSaved(const mojom::Result result,
                            const uint64_t pending_contribution_id) {
  if (result == mojom::Result::LEDGER_OK) {
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
                              ? base::Seconds(2)
                              : util::GetRandomizedDelay(base::Seconds(45));

  BLOG(1, "Unverified contribution timer set for " << delay);

  unverified_publishers_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&Unverified::ProcessNext, base::Unretained(this)));
}

void Unverified::WasPublisherProcessed(const mojom::Result result,
                                       const std::string& publisher_key,
                                       const std::string& name) {
  if (result == mojom::Result::LEDGER_ERROR) {
    BLOG(0, "Couldn't get processed data");
    return;
  }

  if (result == mojom::Result::LEDGER_OK) {
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

void Unverified::ProcessedPublisherSaved(const mojom::Result result,
                                         const std::string& publisher_key,
                                         const std::string& name) {
  ledger_->ledger_client()->OnContributeUnverifiedPublishers(
      mojom::Result::VERIFIED_PUBLISHER, publisher_key, name);
}

void Unverified::OnRemovePendingContribution(mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Problem removing pending contribution");
    return ProcessingCompleted();
  }

  ledger_->ledger_client()->OnContributeUnverifiedPublishers(
      mojom::Result::PENDING_PUBLISHER_REMOVED, "", "");
}

void Unverified::ProcessingCompleted() {
  BLOG(1, "Pending tips processing completed");
  processing_start_time_ = {};
}

}  // namespace contribution
}  // namespace ledger
