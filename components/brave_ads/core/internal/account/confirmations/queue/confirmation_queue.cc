/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_feature.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_factory.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"

namespace brave_ads {

ConfirmationQueue::ConfirmationQueue() {
  AddAdsClientNotifierObserver(this);
}

ConfirmationQueue::~ConfirmationQueue() {
  RemoveAdsClientNotifierObserver(this);
  delegate_ = nullptr;
}

void ConfirmationQueue::Add(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  AddConfirmationQueueItem(confirmation);

  NotifyDidAddConfirmationToQueue(confirmation);

  if (ShouldProcessQueueItem()) {
    ProcessQueueItemAfterDelay(confirmation);
  }
}

///////////////////////////////////////////////////////////////////////////////

bool ConfirmationQueue::ShouldProcessQueueItem() {
  return !timer_.IsRunning();
}

void ConfirmationQueue::ProcessQueueItemAfterDelay(
    const ConfirmationInfo& confirmation) {
  const base::Time process_at = timer_.StartWithPrivacy(
      FROM_HERE, kProcessConfirmationAfter.Get(),
      base::BindOnce(&ConfirmationQueue::ProcessQueueItem,
                     base::Unretained(this), confirmation));

  NotifyWillProcessConfirmationQueue(confirmation, process_at);
}

void ConfirmationQueue::ProcessQueueItem(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  RebuildConfirmationQueueItem(
      confirmation, base::BindOnce(&ConfirmationQueue::ProcessQueueItemCallback,
                                   base::AsWeakPtr(this)));
}

void ConfirmationQueue::ProcessQueueItemCallback(
    const ConfirmationInfo& confirmation) {
  RedeemConfirmationFactory::BuildAndRedeemConfirmation(base::AsWeakPtr(this),
                                                        confirmation);
}

void ConfirmationQueue::SuccessfullyProcessedQueueItem(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  ResetTimerBackoffDelay();

  RemoveConfirmationQueueItem(confirmation);

  NotifyDidProcessConfirmationQueue(confirmation);

  ProcessNextQueueItem();
}

void ConfirmationQueue::FailedToProcessQueueItem(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  CHECK(IsValid(confirmation));

  if (!should_retry) {
    RemoveConfirmationQueueItem(confirmation);
  }

  NotifyFailedToProcessConfirmationQueue(confirmation);

  ProcessNextQueueItem();
}

void ConfirmationQueue::ProcessNextQueueItem() {
  const absl::optional<ConfirmationInfo> confirmation =
      MaybeGetNextConfirmationQueueItem();
  if (!confirmation) {
    return NotifyDidExhaustConfirmationQueue();
  }

  ProcessQueueItemAfterDelay(*confirmation);
}

void ConfirmationQueue::ResetTimerBackoffDelay() {
  timer_.Stop();
}

void ConfirmationQueue::NotifyDidAddConfirmationToQueue(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnDidAddConfirmationToQueue(confirmation);
  }
}

void ConfirmationQueue::NotifyWillProcessConfirmationQueue(
    const ConfirmationInfo& confirmation,
    const base::Time process_at) const {
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

void ConfirmationQueue::NotifyDidExhaustConfirmationQueue() const {
  if (delegate_) {
    delegate_->OnDidExhaustConfirmationQueue();
  }
}

void ConfirmationQueue::OnNotifyDidInitializeAds() {
  ProcessNextQueueItem();
}

void ConfirmationQueue::OnDidRedeemConfirmation(
    const ConfirmationInfo& confirmation) {
  SuccessfullyProcessedQueueItem(confirmation);
}

void ConfirmationQueue::OnFailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  FailedToProcessQueueItem(confirmation, should_retry);
}

}  // namespace brave_ads
