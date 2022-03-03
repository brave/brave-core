/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_tabs_util.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/user_activity/user_activity.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityTabsUtilTest : public UnitTestBase {
 protected:
  BatAdsUserActivityTabsUtilTest() = default;

  ~BatAdsUserActivityTabsUtilTest() override = default;
};

TEST_F(BatAdsUserActivityTabsUtilTest, NoTabsOpened) {
  // // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int number_of_tabs_opened = GetNumberOfTabsOpened(events);

  // // Assert
  EXPECT_EQ(0, number_of_tabs_opened);
}

TEST_F(BatAdsUserActivityTabsUtilTest, TabsOpened) {
  // // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  AdvanceClock(base::Minutes(30));

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int number_of_tabs_opened = GetNumberOfTabsOpened(events);

  // // Assert
  EXPECT_EQ(2, number_of_tabs_opened);
}

}  // namespace ads
