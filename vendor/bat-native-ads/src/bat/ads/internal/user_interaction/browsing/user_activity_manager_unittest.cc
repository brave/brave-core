/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/browsing/user_activity_manager.h"

#include "bat/ads/internal/base/containers/container_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"
#include "bat/ads/internal/user_interaction/browsing/user_activity_constants.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityManagerTest : public UnitTestBase {
 protected:
  BatAdsUserActivityManagerTest() = default;

  ~BatAdsUserActivityManagerTest() override = default;
};

TEST_F(BatAdsUserActivityManagerTest, HasInstance) {
  // Arrange

  // Act

  // Assert
  const bool has_instance = UserActivityManager::HasInstance();
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsUserActivityManagerTest, RecordInitializedAdsEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kInitializedAds;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordBrowserDidEnterForegroundEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidEnterForeground;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordBrowserDidEnterBackgroundEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidEnterBackground;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest,
       RecordClickedBackOrForwardNavigationButtonsEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedBackOrForwardNavigationButtons;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClickedBookmarkEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedBookmark;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClickedHomePageButtonEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedHomePageButton;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClickedLinkEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClickedLink;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClickedReloadButtonEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedReloadButton;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordClosedTabEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClosedTab;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordFocusedOnExistingTabEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTabChangedFocus;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordGeneratedKeywordEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kGeneratedKeyword;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordNewNavigationEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kNewNavigation;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest,
       RecordOpenedLinkFromExternalApplicationEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kOpenedLinkFromExternalApplication;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordOpenedNewTabEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kOpenedNewTab;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordPlayedMediaEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTabStartedPlayingMedia;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordStoppedPlayingMediaEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTabStoppedPlayingMedia;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordSubmittedFormEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kSubmittedForm;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordTabUpdatedEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kTabUpdated;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordTypedAndSelectedNonUrlEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTypedAndSelectedNonUrl;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest,
       RecordTypedKeywordOtherThanDefaultSearchProviderEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordTypedUrlEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kTypedUrl;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordUsedAddressBarEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kUsedAddressBar;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordBrowserDidBecomeActiveEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidBecomeActive;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, RecordBrowserDidResignActiveEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidResignActive;

  // Act
  UserActivityManager::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, GetHistoryForTimeWindow) {
  // Arrange
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kInitializedAds);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kBrowserDidBecomeActive);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kBrowserDidEnterBackground);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kClickedBackOrForwardNavigationButtons);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kClickedBookmark);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kClickedHomePageButton);
  UserActivityManager::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kClickedReloadButton);
  UserActivityManager::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kTabChangedFocus);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kGeneratedKeyword);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kNewNavigation);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kOpenedLinkFromExternalApplication);
  UserActivityManager::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kTabStoppedPlayingMedia);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kSubmittedForm);

  AdvanceClockBy(base::Hours(1));

  const base::Time time = Now();

  UserActivityManager::Get()->RecordEvent(UserActivityEventType::kTabUpdated);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kTypedAndSelectedNonUrl);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider);
  UserActivityManager::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivityManager::Get()->RecordEvent(
      UserActivityEventType::kUsedAddressBar);

  AdvanceClockBy(base::Hours(1));

  // Act
  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

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

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

TEST_F(BatAdsUserActivityManagerTest, MaximumHistoryItems) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClosedTab;
  for (int i = 0; i < kMaximumHistoryItems; i++) {
    UserActivityManager::Get()->RecordEvent(event_type);
  }

  // Act
  UserActivityManager::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  const UserActivityEventList events =
      UserActivityManager::Get()->GetHistoryForTimeWindow(base::Hours(1));

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

  EXPECT_TRUE(IsEqualContainers(expected_events, events));
}

}  // namespace ads
