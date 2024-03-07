/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueItemBuilderUtilTest : public UnitTestBase {};

TEST_F(BraveAdsConfirmationQueueItemBuilderUtilTest, ProcessConfirmationAt) {
  // Act & Assert
  EXPECT_FALSE(ProcessConfirmationAt().is_null());
}

}  // namespace brave_ads
