/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

struct ConfirmationInfo;

class ConfirmationQueue final : public AdsClientNotifierObserver,
                                public RedeemConfirmationDelegate {
 public:
  ConfirmationQueue();

  ConfirmationQueue(const ConfirmationQueue&) = delete;
  ConfirmationQueue& operator=(const ConfirmationQueue&) = delete;

  ConfirmationQueue(ConfirmationQueue&&) noexcept = delete;
  ConfirmationQueue& operator=(ConfirmationQueue&&) noexcept = delete;

  ~ConfirmationQueue() override;

  void SetDelegate(ConfirmationQueueDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void Add(const ConfirmationInfo& confirmation);

 private:
  bool ShouldProcessQueueItem();
  void ProcessQueueItemAfterDelay(const ConfirmationInfo& confirmation);
  void ProcessQueueItem(const ConfirmationInfo& confirmation);
  void ProcessQueueItemCallback(const ConfirmationInfo& confirmation);

  void SuccessfullyProcessedQueueItem(const ConfirmationInfo& confirmation);
  void FailedToProcessQueueItem(const ConfirmationInfo& confirmation,
                                bool should_retry);

  void ProcessNextQueueItem();

  void ResetTimerBackoffDelay();

  void NotifyDidAddConfirmationToQueue(
      const ConfirmationInfo& confirmation) const;
  void NotifyWillProcessConfirmationQueue(const ConfirmationInfo& confirmation,
                                          base::Time process_at) const;
  void NotifyDidProcessConfirmationQueue(
      const ConfirmationInfo& confirmation) const;
  void NotifyFailedToProcessConfirmationQueue(
      const ConfirmationInfo& confirmation) const;
  void NotifyDidExhaustConfirmationQueue() const;

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;

  // RedeemConfirmationDelegate:
  void OnDidRedeemConfirmation(const ConfirmationInfo& confirmation) override;
  void OnFailedToRedeemConfirmation(const ConfirmationInfo& confirmation,
                                    bool should_retry) override;

  raw_ptr<ConfirmationQueueDelegate> delegate_ = nullptr;

  BackoffTimer timer_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_H_
