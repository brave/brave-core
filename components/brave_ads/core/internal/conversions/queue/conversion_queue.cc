/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_builder_util.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_util.h"

namespace brave_ads {

ConversionQueue::ConversionQueue() {
  AddAdsClientNotifierObserver(this);
}

ConversionQueue::~ConversionQueue() {
  RemoveAdsClientNotifierObserver(this);
  delegate_ = nullptr;
}

void ConversionQueue::Add(const ConversionInfo& conversion) {
  const ConversionQueueItemInfo conversion_queue_item =
      BuildConversionQueueItem(conversion, ProcessConversionAt());
  CHECK(conversion_queue_item.IsValid());

  const database::table::ConversionQueue database_table;
  database_table.Save(
      {conversion_queue_item},
      base::BindOnce(&ConversionQueue::AddCallback, weak_factory_.GetWeakPtr(),
                     conversion_queue_item));
}

///////////////////////////////////////////////////////////////////////////////

void ConversionQueue::AddCallback(
    const ConversionQueueItemInfo& conversion_queue_item,
    const bool success) {
  if (!success) {
    return NotifyFailedToAddConversionToQueue(conversion_queue_item.conversion);
  }

  NotifyDidAddConversionToQueue(conversion_queue_item.conversion);

  if (ShouldProcessQueueItem(conversion_queue_item)) {
    ProcessQueueItemAfterDelay(conversion_queue_item);
  }
}

bool ConversionQueue::ShouldProcessQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  return !timer_.IsRunning() ||
         ShouldProcessBeforeScheduledQueueItem(conversion_queue_item);
}

bool ConversionQueue::ShouldProcessBeforeScheduledQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  CHECK(timer_.IsRunning());

  const base::Time process_at =
      base::Time::Now() +
      CalculateDelayBeforeProcessingConversionQueueItem(conversion_queue_item);

  return process_at < timer_.desired_run_time();
}

void ConversionQueue::ProcessQueueItemAfterDelay(
    const ConversionQueueItemInfo& conversion_queue_item) {
  const base::Time process_at = timer_.Start(
      FROM_HERE,
      CalculateDelayBeforeProcessingConversionQueueItem(conversion_queue_item),
      base::BindOnce(&ConversionQueue::ProcessQueueItem, base::Unretained(this),
                     conversion_queue_item));

  NotifyWillProcessConversionQueue(conversion_queue_item.conversion,
                                   process_at);
}

void ConversionQueue::ProcessQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  CHECK(conversion_queue_item.IsValid());

  MarkQueueItemAsProcessed(conversion_queue_item);
}

void ConversionQueue::MarkQueueItemAsProcessed(
    const ConversionQueueItemInfo& conversion_queue_item) {
  const database::table::ConversionQueue database_table;
  database_table.Update(
      conversion_queue_item,
      base::BindOnce(&ConversionQueue::MarkQueueItemAsProcessedCallback,
                     weak_factory_.GetWeakPtr(), conversion_queue_item));
}

void ConversionQueue::MarkQueueItemAsProcessedCallback(
    const ConversionQueueItemInfo& conversion_queue_item,
    const bool success) {
  if (!success) {
    BLOG(0, "Failed to mark conversion queue item as processed");
    return FailedToProcessQueueItem(conversion_queue_item);
  }

  SuccessfullyProcessedQueueItem(conversion_queue_item);
}

void ConversionQueue::SuccessfullyProcessedQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  NotifyDidProcessConversionQueue(conversion_queue_item.conversion);

  ProcessNextQueueItem();
}

void ConversionQueue::FailedToProcessQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  NotifyFailedToProcessConversionQueue(conversion_queue_item.conversion);

  ProcessNextQueueItem();
}

void ConversionQueue::ProcessNextQueueItem() {
  const database::table::ConversionQueue database_table;
  database_table.GetUnprocessed(
      base::BindOnce(&ConversionQueue::ProcessNextQueueItemCallback,
                     weak_factory_.GetWeakPtr()));
}

void ConversionQueue::ProcessNextQueueItemCallback(
    const bool success,
    const ConversionQueueItemList& conversion_queue_items) {
  if (!success) {
    return NotifyFailedToProcessNextConversionInQueue();
  }

  if (conversion_queue_items.empty()) {
    return NotifyDidExhaustConversionQueue();
  }

  const ConversionQueueItemInfo& conversion_queue_item =
      conversion_queue_items.front();

  ProcessQueueItemAfterDelay(conversion_queue_item);
}

void ConversionQueue::OnNotifyDidInitializeAds() {
  ProcessNextQueueItem();
}

void ConversionQueue::NotifyFailedToAddConversionToQueue(
    const ConversionInfo& conversion) const {
  if (delegate_) {
    delegate_->OnFailedToAddConversionToQueue(conversion);
  }
}

void ConversionQueue::NotifyDidAddConversionToQueue(
    const ConversionInfo& conversion) const {
  if (delegate_) {
    delegate_->OnDidAddConversionToQueue(conversion);
  }
}

void ConversionQueue::NotifyWillProcessConversionQueue(
    const ConversionInfo& conversion,
    const base::Time process_at) const {
  if (delegate_) {
    delegate_->OnWillProcessConversionQueue(conversion, process_at);
  }
}

void ConversionQueue::NotifyDidProcessConversionQueue(
    const ConversionInfo& conversion) const {
  if (delegate_) {
    delegate_->OnDidProcessConversionQueue(conversion);
  }
}

void ConversionQueue::NotifyFailedToProcessConversionQueue(
    const ConversionInfo& conversion) const {
  if (delegate_) {
    delegate_->OnFailedToProcessConversionQueue(conversion);
  }
}

void ConversionQueue::NotifyFailedToProcessNextConversionInQueue() const {
  if (delegate_) {
    delegate_->OnFailedToProcessNextConversionInQueue();
  }
}

void ConversionQueue::NotifyDidExhaustConversionQueue() const {
  if (delegate_) {
    delegate_->OnDidExhaustConversionQueue();
  }
}

}  // namespace brave_ads
