/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads::test {

namespace {

void RecordAdEvent(const AdEventInfo& ad_event) {
  RecordAdEvent(ad_event, base::BindOnce([](const bool success) {
                  ASSERT_TRUE(success);
                }));
}

}  // namespace

void RecordAdEvent(const AdInfo& ad,
                   const mojom::ConfirmationType mojom_confirmation_type) {
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom_confirmation_type, /*created_at=*/Now());
  RecordAdEvent(ad_event);
}

void RecordAdEvents(
    const AdInfo& ad,
    const std::vector<mojom::ConfirmationType>& mojom_confirmation_types) {
  for (const auto& mojom_confirmation_type : mojom_confirmation_types) {
    RecordAdEvent(ad, mojom_confirmation_type);
  }
}

void RecordAdEvents(const AdInfo& ad,
                    mojom::ConfirmationType mojom_confirmation_type,
                    int count) {
  CHECK_GT(count, 0);

  for (int i = 0; i < count; ++i) {
    RecordAdEvent(ad, mojom_confirmation_type);
  }
}

}  // namespace brave_ads::test
