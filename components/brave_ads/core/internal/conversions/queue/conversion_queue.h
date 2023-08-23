/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_CONVERSION_QUEUE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_CONVERSION_QUEUE_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue_delegate.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

class ConversionQueue final : public AdsClientNotifierObserver {
 public:
  ConversionQueue();

  ConversionQueue(const ConversionQueue&) = delete;
  ConversionQueue& operator=(const ConversionQueue&) = delete;

  ConversionQueue(ConversionQueue&&) noexcept = delete;
  ConversionQueue& operator=(ConversionQueue&&) noexcept = delete;

  ~ConversionQueue() override;

  void SetDelegate(ConversionQueueDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  // Add conversions to a queue that are processed in ascending order.
  void Add(const ConversionInfo& conversion);

 private:
  void AddCallback(const ConversionQueueItemInfo& conversion_queue_item,
                   bool success);

  bool ShouldProcessQueueItem(
      const ConversionQueueItemInfo& conversion_queue_item);
  bool ShouldProcessBeforeScheduledQueueItem(
      const ConversionQueueItemInfo& conversion_queue_item);
  void ProcessQueueItemAfterDelay(const ConversionQueueItemInfo& queue_item);
  void ProcessQueueItem(const ConversionQueueItemInfo& conversion_queue_item);

  void MarkQueueItemAsProcessed(
      const ConversionQueueItemInfo& conversion_queue_item);
  void MarkQueueItemAsProcessedCallback(
      const ConversionQueueItemInfo& conversion_queue_item,
      bool success);

  void SuccessfullyProcessedQueueItem(
      const ConversionQueueItemInfo& conversion_queue_item);
  void FailedToProcessQueueItem(
      const ConversionQueueItemInfo& conversion_queue_item);

  void ProcessNextQueueItem();
  void ProcessNextQueueItemCallback(
      bool success,
      const ConversionQueueItemList& conversion_queue_items);

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;

  raw_ptr<ConversionQueueDelegate> delegate_ = nullptr;

  Timer timer_;

  base::WeakPtrFactory<ConversionQueue> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_CONVERSION_QUEUE_H_
