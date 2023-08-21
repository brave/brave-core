/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/p2a_value_util.h"

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::p2a {

namespace {
constexpr char kEventsAsJson[] = R"(["event_1","event_2"])";
}  // namespace

TEST(BraveAdsP2AValueUtilTest, EventsToValue) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonList(kEventsAsJson),
            EventsToValue({"event_1", "event_2"}));
}

TEST(BraveAdsP2AValueUtilTest, EmptyEventsToValue) {
  // Arrange

  // Act
  const base::Value::List events = EventsToValue({});

  // Assert
  EXPECT_TRUE(events.empty());
}

}  // namespace brave_ads::p2a
