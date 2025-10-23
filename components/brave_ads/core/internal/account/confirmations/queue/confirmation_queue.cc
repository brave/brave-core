/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue.h"

#include <utility>

#include "base/check.h"
#include "base/debug/crash_logging.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_factory.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

ConfirmationQueue::ConfirmationQueue() {
  GetAdsClient().AddObserver(this);
}

ConfirmationQueue::~ConfirmationQueue() {
  GetAdsClient().RemoveObserver(this);
  delegate_ = nullptr;
}

void ConfirmationQueue::Add(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  const ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(confirmation,
                                 /*process_at=*/base::Time::Now());
  CHECK(confirmation_queue_item.IsValid());

  database_table_.Save(
      {confirmation_queue_item},
      base::BindOnce(&ConfirmationQueue::AddCallback,
                     weak_factory_.GetWeakPtr(), confirmation_queue_item));
}

///////////////////////////////////////////////////////////////////////////////

void ConfirmationQueue::AddCallback(
    const ConfirmationQueueItemInfo& confirmation_queue_item,
    bool success) {
  if (!success) {
    SCOPED_CRASH_KEY_NUMBER(
        "Issue50267", "ad_type",
        static_cast<int>(confirmation_queue_item.confirmation.ad_type));
    SCOPED_CRASH_KEY_NUMBER(
        "Issue50267", "confirmation_type",
        static_cast<int>(confirmation_queue_item.confirmation.type));
    SCOPED_CRASH_KEY_STRING64(
        "Issue50267", "creative_instance_id",
        confirmation_queue_item.confirmation.creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "Failed to add confirmation to queue");
    DUMP_WILL_BE_NOTREACHED();
    return NotifyFailedToAddConfirmationToQueue(
        confirmation_queue_item.confirmation);
  }

  NotifyDidAddConfirmationToQueue(confirmation_queue_item.confirmation);

  if (ShouldProcessQueueItem(confirmation_queue_item)) {
    ProcessQueueItemAfterDelay(confirmation_queue_item);
  }
}

bool ConfirmationQueue::ShouldProcessQueueItem(
    const ConfirmationQueueItemInfo& confirmation_queue_item) {
  return !is_processing_ &&
         (!timer_.IsRunning() ||
          ShouldProcessBeforeScheduledQueueItem(confirmation_queue_item));
}

bool ConfirmationQueue::ShouldProcessBeforeScheduledQueueItem(
    const ConfirmationQueueItemInfo& confirmation_queue_item) {
  CHECK(timer_.IsRunning());

  const base::Time process_at =
      base::Time::Now() + CalculateDelayBeforeProcessingConfirmationQueueItem(
                              confirmation_queue_item);

  return process_at < timer_.desired_run_time();
}

void ConfirmationQueue::ProcessQueueItemAfterDelay(
    const ConfirmationQueueItemInfo& confirmation_queue_item) {
  const base::Time process_at = timer_.Start(
      FROM_HERE,
      CalculateDelayBeforeProcessingConfirmationQueueItem(
          confirmation_queue_item),
      base::BindOnce(&ConfirmationQueue::ProcessQueueItem,
                     weak_factory_.GetWeakPtr(), confirmation_queue_item));

  NotifyWillProcessConfirmationQueue(confirmation_queue_item.confirmation,
                                     process_at);
}

void ConfirmationQueue::ProcessQueueItem(
    const ConfirmationQueueItemInfo& confirmation_queue_item) {
  CHECK(confirmation_queue_item.IsValid());

  is_processing_ = true;

  RedeemConfirmationFactory::BuildAndRedeemConfirmation(
      weak_factory_.GetWeakPtr(),
      RebuildConfirmationDynamicUserData(confirmation_queue_item.confirmation));
}

void ConfirmationQueue::SuccessfullyProcessedQueueItem(
    const ConfirmationInfo& confirmation) {
  database_table_.Delete(
      confirmation.transaction_id,
      base::BindOnce(&ConfirmationQueue::SuccessfullyProcessedQueueItemCallback,
                     weak_factory_.GetWeakPtr(), confirmation));
}

void ConfirmationQueue::SuccessfullyProcessedQueueItemCallback(
    const ConfirmationInfo& confirmation,
    bool success) {
  is_processing_ = false;

  NotifyDidProcessConfirmationQueue(confirmation);

  if (!success) {
    return BLOG(0, "Failed to delete confirmation queue item");
  }

  ProcessNextQueueItem();
}

void ConfirmationQueue::FailedToProcessQueueItem(
    const ConfirmationInfo& confirmation,
    bool should_retry) {
  auto callback =
      base::BindOnce(&ConfirmationQueue::FailedToProcessQueueItemCallback,
                     weak_factory_.GetWeakPtr(), confirmation, should_retry);

  if (should_retry) {
    return database_table_.Retry(confirmation.transaction_id,
                                 std::move(callback));
  }

  database_table_.Delete(confirmation.transaction_id, std::move(callback));
}

void ConfirmationQueue::FailedToProcessQueueItemCallback(
    const ConfirmationInfo& confirmation,
    bool should_retry,
    bool success) {
  is_processing_ = false;

  NotifyFailedToProcessConfirmationQueue(confirmation);

  if (!success) {
    if (should_retry) {
      SCOPED_CRASH_KEY_NUMBER("Issue50267", "ad_type",
                              static_cast<int>(confirmation.ad_type));
      SCOPED_CRASH_KEY_NUMBER("Issue50267", "confirmation_type",
                              static_cast<int>(confirmation.type));
      SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                                confirmation.creative_instance_id);
      SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                                "Failed to retry confirmation queue item");
      DUMP_WILL_BE_NOTREACHED();

      return BLOG(0, "Failed to retry confirmation queue item");
    }

    SCOPED_CRASH_KEY_NUMBER("Issue50267", "ad_type",
                            static_cast<int>(confirmation.ad_type));
    SCOPED_CRASH_KEY_NUMBER("Issue50267", "confirmation_type",
                            static_cast<int>(confirmation.type));
    SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                              confirmation.creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "Failed to delete confirmation queue item");
    return BLOG(0, "Failed to delete confirmation queue item");
  }

  ProcessNextQueueItem();
}

void ConfirmationQueue::ProcessNextQueueItem() {
  database_table_.GetNext(
      base::BindOnce(&ConfirmationQueue::ProcessNextQueueItemCallback,
                     weak_factory_.GetWeakPtr()));
}

void ConfirmationQueue::ProcessNextQueueItemCallback(
    bool success,
    const ConfirmationQueueItemList& confirmation_queue_items) {
  if (!success) {
    return NotifyFailedToProcessNextConfirmationInQueue();
  }

  if (confirmation_queue_items.empty()) {
    return NotifyDidExhaustConfirmationQueue();
  }

  const ConfirmationQueueItemInfo& confirmation_queue_item =
      confirmation_queue_items.front();
  ProcessQueueItemAfterDelay(confirmation_queue_item);
}

void ConfirmationQueue::OnNotifyDidInitializeAds() {
  ProcessNextQueueItem();
}

void ConfirmationQueue::NotifyFailedToAddConfirmationToQueue(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnFailedToAddConfirmationToQueue(confirmation);
  }
}

void ConfirmationQueue::NotifyDidAddConfirmationToQueue(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnDidAddConfirmationToQueue(confirmation);
  }
}

void ConfirmationQueue::NotifyWillProcessConfirmationQueue(
    const ConfirmationInfo& confirmation,
    base::Time process_at) const {
  if (delegate_) {
    delegate_->OnWillProcessConfirmationQueue(confirmation, process_at);
  }
}

void ConfirmationQueue::NotifyDidProcessConfirmationQueue(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnDidProcessConfirmationQueue(confirmation);
  }
}

void ConfirmationQueue::NotifyFailedToProcessConfirmationQueue(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnFailedToProcessConfirmationQueue(confirmation);
  }
}

void ConfirmationQueue::NotifyFailedToProcessNextConfirmationInQueue() const {
  if (delegate_) {
    delegate_->OnFailedToProcessNextConfirmationInQueue();
  }
}

void ConfirmationQueue::NotifyDidExhaustConfirmationQueue() const {
  if (delegate_) {
    delegate_->OnDidExhaustConfirmationQueue();
  }
}

void ConfirmationQueue::OnDidRedeemConfirmation(
    const ConfirmationInfo& confirmation) {
  SuccessfullyProcessedQueueItem(confirmation);
}

void ConfirmationQueue::OnFailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    bool should_retry) {
  FailedToProcessQueueItem(confirmation, should_retry);
}

}  // namespace brave_ads
