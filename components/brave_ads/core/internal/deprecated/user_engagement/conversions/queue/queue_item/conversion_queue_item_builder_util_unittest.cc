/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_builder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionQueueItemBuilderUtilTest : public UnitTestBase {};

TEST_F(BraveAdsConversionQueueItemBuilderUtilTest, ProcessConversionAt) {
  // Act & Assert
  EXPECT_FALSE(ProcessConversionAt().is_null());
}

}  // namespace brave_ads
