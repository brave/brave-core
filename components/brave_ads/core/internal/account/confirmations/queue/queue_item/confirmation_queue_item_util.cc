/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"

#include <optional>
#include <string>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util_constants.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"

namespace brave_ads {

namespace {

std::optional<base::TimeDelta>
    g_scoped_delay_before_processing_confirmation_queue_item_for_testing;

base::TimeDelta DelayBeforeProcessingQueueItem(
    const ConfirmationQueueItemInfo& confirmation_queue_item,
    base::Time time) {
  CHECK(confirmation_queue_item.process_at);
  return *confirmation_queue_item.process_at - time;
}

bool ShouldHaveProcessedQueueItemInThePast(
    const ConfirmationQueueItemInfo& confirmation_queue_item,
    base::Time time) {
  return DelayBeforeProcessingQueueItem(confirmation_queue_item, time)
      .is_negative();
}

bool ShouldProcessQueueItem(
    const ConfirmationQueueItemInfo& confirmation_queue_item,
    base::Time time) {
  return time >= confirmation_queue_item.process_at;
}

}  // namespace

base::TimeDelta CalculateDelayBeforeProcessingConfirmationQueueItem(
    const ConfirmationQueueItemInfo& confirmation_queue_item) {
  if (g_scoped_delay_before_processing_confirmation_queue_item_for_testing) {
    CHECK_IS_TEST();

    return *g_scoped_delay_before_processing_confirmation_queue_item_for_testing;
  }

  const base::Time now = base::Time::Now();

  if (ShouldHaveProcessedQueueItemInThePast(confirmation_queue_item, now) ||
      ShouldProcessQueueItem(confirmation_queue_item, now)) {
    return kMinimumDelayBeforeProcessingConfirmationQueueItem;
  }

  const base::TimeDelta delay =
      DelayBeforeProcessingQueueItem(confirmation_queue_item, now);
  if (delay < kMinimumDelayBeforeProcessingConfirmationQueueItem) {
    return kMinimumDelayBeforeProcessingConfirmationQueueItem;
  }

  return delay;
}

ScopedDelayBeforeProcessingConfirmationQueueItemForTesting::
    ScopedDelayBeforeProcessingConfirmationQueueItemForTesting(
        base::TimeDelta delay) {
  CHECK_IS_TEST();

  g_scoped_delay_before_processing_confirmation_queue_item_for_testing = delay;
}

ScopedDelayBeforeProcessingConfirmationQueueItemForTesting::
    ~ScopedDelayBeforeProcessingConfirmationQueueItemForTesting() {
  g_scoped_delay_before_processing_confirmation_queue_item_for_testing =
      std::nullopt;
}

ConfirmationInfo RebuildConfirmationWithoutDynamicUserData(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  ConfirmationInfo mutable_confirmation(confirmation);

  mutable_confirmation.user_data.dynamic.clear();

  if (mutable_confirmation.reward) {
    const std::optional<std::string> reward_credential_base64url =
        BuildRewardCredential(mutable_confirmation);
    CHECK(reward_credential_base64url);

    mutable_confirmation.reward->credential_base64url =
        *reward_credential_base64url;
  }

  return mutable_confirmation;
}

ConfirmationInfo RebuildConfirmationDynamicUserData(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  ConfirmationInfo mutable_confirmation(confirmation);

  mutable_confirmation.user_data.dynamic = BuildDynamicUserData();

  if (mutable_confirmation.reward) {
    const std::optional<std::string> reward_credential_base64url =
        BuildRewardCredential(mutable_confirmation);
    CHECK(reward_credential_base64url);

    mutable_confirmation.reward->credential_base64url =
        *reward_credential_base64url;
  }

  return mutable_confirmation;
}

}  // namespace brave_ads
