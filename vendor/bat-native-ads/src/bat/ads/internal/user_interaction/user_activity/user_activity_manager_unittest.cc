/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_activity/user_activity_manager.h"

#include "base/ranges/algorithm.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_constants.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityManagerTest : public UnitTestBase {};

TEST_F(BatAdsUserActivityManagerTest, HasInstance) {
  // Arrange

  // Act
  const bool has_instance = UserActivityManager::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsUserActivityManagerTest, RecordInitializedAdsEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kInitializedAds;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordBrowserDidEnterForegroundEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidEnterForeground;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordBrowserDidEnterBackgroundEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidEnterBackground;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest,
       RecordClickedBackOrForwardNavigationButtonsEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedBackOrForwardNavigationButtons;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClickedBookmarkEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedBookmark;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClickedHomePageButtonEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedHomePageButton;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClickedLinkEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClickedLink;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClickedReloadButtonEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedReloadButton;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClosedTabEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClosedTab;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordFocusedOnExistingTabEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTabChangedFocus;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordGeneratedKeywordEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kGeneratedKeyword;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordNewNavigationEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kNewNavigation;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest,
       RecordOpenedLinkFromExternalApplicationEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kOpenedLinkFromExternalApplication;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordOpenedNewTabEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kOpenedNewTab;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordPlayedMediaEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTabStartedPlayingMedia;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordStoppedPlayingMediaEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTabStoppedPlayingMedia;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordSubmittedFormEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kSubmittedForm;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordTabUpdatedEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kTabUpdated;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordTypedAndSelectedNonUrlEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTypedAndSelectedNonUrl;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest,
       RecordTypedKeywordOtherThanDefaultSearchProviderEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordTypedUrlEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kTypedUrl;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordUsedAddressBarEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kUsedAddressBar;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordBrowserDidBecomeActiveEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidBecomeActive;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordBrowserDidResignActiveEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidResignActive;

  // Act
  UserActivityManager::GetInstance()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, GetHistoryForTimeWindow) {
  // Arrange
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kInitializedAds);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kBrowserDidBecomeActive);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kBrowserDidEnterBackground);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedBackOrForwardNavigationButtons);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedBookmark);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedHomePageButton);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedReloadButton);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClosedTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabChangedFocus);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kGeneratedKeyword);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kNewNavigation);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedLinkFromExternalApplication);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabStoppedPlayingMedia);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kSubmittedForm);

  AdvanceClockBy(base::Hours(1));

  const base::Time time = Now();

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabUpdated);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedAndSelectedNonUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kUsedAddressBar);

  AdvanceClockBy(base::Hours(1));

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kTabUpdated;
  event.created_at = time;
  expected_events.push_back(event);
  event.type = UserActivityEventType::kTypedAndSelectedNonUrl;
  event.created_at = time;
  expected_events.push_back(event);
  event.type =
      UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider;
  event.created_at = time;
  expected_events.push_back(event);
  event.type = UserActivityEventType::kTypedUrl;
  event.created_at = time;
  expected_events.push_back(event);
  event.type = UserActivityEventType::kUsedAddressBar;
  event.created_at = time;
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, MaximumHistoryItems) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClosedTab;
  for (int i = 0; i < kMaximumHistoryItems; i++) {
    UserActivityManager::GetInstance()->RecordEvent(event_type);
  }

  // Act
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;

  for (int i = 0; i < kMaximumHistoryItems - 1; i++) {
    event.type = event_type;
    event.created_at = Now();
    expected_events.push_back(event);
  }

  event.type = UserActivityEventType::kOpenedNewTab;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(base::ranges::equal(expected_events, events));
}

}  // namespace ads
