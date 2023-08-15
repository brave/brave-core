/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/p2a.h"

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

using ::testing::Eq;

namespace brave_ads::p2a {

namespace {
constexpr char kEventsAsJson[] = R"(["event_1","event_2"])";
}  // namespace

class BraveAdsP2ATest : public UnitTestBase {};

TEST_F(BraveAdsP2ATest, RecordEvent) {
  // Arrange
  const base::Value::List events = base::test::ParseJsonList(kEventsAsJson);

  // Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents(Eq(std::ref(events))));

  // Act
  RecordEvent({"event_1", "event_2"});
}

}  // namespace brave_ads::p2a
