/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_scoring.h"

#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/internal/user_activity/user_activity_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsUserActivityUtilTest, ToUserActivityTriggers) {
  // Arrange

  // Act
  const UserActivityTriggers triggers =
      ToUserActivityTriggers("05=.3;0C1305=1.0;0C13=0.5");

  // Assert
  UserActivityTriggers expected_triggers;
  UserActivityTriggerInfo trigger;
  trigger.event_sequence = "05";
  trigger.score = 0.3;
  expected_triggers.push_back(trigger);
  trigger.event_sequence = "0C1305";
  trigger.score = 1.0;
  expected_triggers.push_back(trigger);
  trigger.event_sequence = "0C13";
  trigger.score = 0.5;
  expected_triggers.push_back(trigger);

  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityUtilTest, ToUserActivityTriggersForInvalidTrigger) {
  // Arrange

  // Act
  const UserActivityTriggers triggers = ToUserActivityTriggers("INVALID");

  // Assert
  const UserActivityTriggers expected_triggers = {};
  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityUtilTest, ToUserActivityTriggersForMalformedTrigger) {
  // Arrange

  // Act
  const UserActivityTriggers triggers =
      ToUserActivityTriggers("05=.3;0C1305=;=0.5;C1305=1.0");

  // Assert
  UserActivityTriggers expected_triggers;
  UserActivityTriggerInfo trigger;
  trigger.event_sequence = "05";
  trigger.score = 0.3;
  expected_triggers.push_back(trigger);

  EXPECT_EQ(expected_triggers, triggers);
}

TEST(BatAdsUserActivityUtilTest, ToUserActivityTriggersForEmptyTrigger) {
  // Arrange

  // Act
  const UserActivityTriggers triggers = ToUserActivityTriggers("");

  // Assert
  const UserActivityTriggers expected_triggers = {};
  EXPECT_EQ(expected_triggers, triggers);
}

}  // namespace ads
