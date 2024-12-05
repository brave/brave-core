/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace brave_ads {

struct ConfirmationInfo;

class ConfirmationQueue final : public AdsClientNotifierObserver,
                                public RedeemConfirmationDelegate {
 public:
  ConfirmationQueue();

  ConfirmationQueue(const ConfirmationQueue&) = delete;
  ConfirmationQueue& operator=(const ConfirmationQueue&) = delete;

  ~ConfirmationQueue() override;

  void SetDelegate(ConfirmationQueueDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  // Add confirmations to a queue that are processed in cronological order.
  void Add(const ConfirmationInfo& confirmation);

 private:
  void AddCallback(const ConfirmationQueueItemInfo& confirmation_queue_item,
                   bool success);

  bool ShouldProcessQueueItem(
      const ConfirmationQueueItemInfo& confirmation_queue_item);
  bool ShouldProcessBeforeScheduledQueueItem(
      const ConfirmationQueueItemInfo& confirmation_queue_item);
  void ProcessQueueItemAfterDelay(
      const ConfirmationQueueItemInfo& confirmation_queue_item);
  void ProcessQueueItem(
      const ConfirmationQueueItemInfo& confirmation_queue_item);

  void SuccessfullyProcessedQueueItem(const ConfirmationInfo& confirmation);
  void SuccessfullyProcessedQueueItemCallback(
      const ConfirmationInfo& confirmation,
      bool success);
  void FailedToProcessQueueItem(const ConfirmationInfo& confirmation,
                                bool should_retry);
  void FailedToProcessQueueItemCallback(const ConfirmationInfo& confirmation,
                                        bool should_retry,
                                        bool success);

  void ProcessNextQueueItem();
  void ProcessNextQueueItemCallback(
      bool success,
      const ConfirmationQueueItemList& confirmation_queue_items);

  void NotifyFailedToAddConfirmationToQueue(
      const ConfirmationInfo& confirmation) const;
  void NotifyDidAddConfirmationToQueue(
      const ConfirmationInfo& confirmation) const;
  void NotifyWillProcessConfirmationQueue(const ConfirmationInfo& confirmation,
                                          base::Time process_at) const;
  void NotifyDidProcessConfirmationQueue(
      const ConfirmationInfo& confirmation) const;
  void NotifyFailedToProcessConfirmationQueue(
      const ConfirmationInfo& confirmation) const;
  void NotifyFailedToProcessNextConfirmationInQueue() const;
  void NotifyDidExhaustConfirmationQueue() const;

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;

  // RedeemConfirmationDelegate:
  void OnDidRedeemConfirmation(const ConfirmationInfo& confirmation) override;
  void OnFailedToRedeemConfirmation(const ConfirmationInfo& confirmation,
                                    bool should_retry) override;

  raw_ptr<ConfirmationQueueDelegate> delegate_ = nullptr;  // Not owned.

  database::table::ConfirmationQueue database_table_;

  Timer timer_;

  bool is_processing_ = false;

  base::WeakPtrFactory<ConfirmationQueue> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_H_
