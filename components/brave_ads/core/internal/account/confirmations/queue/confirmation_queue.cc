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
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/redeem_confirmation/redeem_confirmation_factory.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"

namespace brave_ads {

namespace {
constexpr base::TimeDelta kProcessQueueItemAfter = base::Seconds(15);
}  // namespace

ConfirmationQueue::ConfirmationQueue() {
  AdsClientHelper::AddObserver(this);
}

ConfirmationQueue::~ConfirmationQueue() {
  AdsClientHelper::RemoveObserver(this);
  delegate_ = nullptr;
}

void ConfirmationQueue::Add(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  AddConfirmationQueueItem(confirmation);

  if (delegate_) {
    delegate_->OnDidAddConfirmationToQueue(confirmation);
  }

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
      FROM_HERE, kProcessQueueItemAfter,
      base::BindOnce(&ConfirmationQueue::ProcessQueueItem,
                     base::Unretained(this), confirmation));

  if (delegate_) {
    delegate_->OnWillProcessConfirmationQueue(confirmation, process_at);
  }
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

  RemoveConfirmationQueueItem(confirmation);

  if (delegate_) {
    delegate_->OnDidProcessConfirmationQueue(confirmation);
  }

  ResetTimerBackoffDelay();

  ProcessNextQueueItem();
}

void ConfirmationQueue::FailedToProcessQueueItem(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  CHECK(IsValid(confirmation));

  if (!should_retry) {
    RemoveConfirmationQueueItem(confirmation);
  }

  if (delegate_) {
    delegate_->OnFailedToProcessConfirmationQueue(confirmation);
  }

  ProcessNextQueueItem();
}

void ConfirmationQueue::ProcessNextQueueItem() {
  const absl::optional<ConfirmationInfo> confirmation =
      MaybeGetNextConfirmationQueueItem();
  if (!confirmation) {
    if (delegate_) {
      delegate_->OnDidExhaustConfirmationQueue();
    }

    return;
  }

  ProcessQueueItemAfterDelay(*confirmation);
}

void ConfirmationQueue::ResetTimerBackoffDelay() {
  timer_.Stop();
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
