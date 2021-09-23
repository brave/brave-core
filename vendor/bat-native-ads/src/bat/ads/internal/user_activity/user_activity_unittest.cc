/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityTest : public UnitTestBase {
 protected:
  BatAdsUserActivityTest() = default;

  ~BatAdsUserActivityTest() override = default;
};

TEST_F(BatAdsUserActivityTest, HasInstance) {
  // Arrange

  // Act

  // Assert
  const bool has_instance = UserActivity::HasInstance();
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsUserActivityTest, RecordLaunchedBrowserEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kInitializedAds;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordBrowserDidBecomeActiveEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidBecomeActive;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordBrowserDidEnterBackgroundEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserDidEnterBackground;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest,
       RecordClickedBackOrForwardNavigationButtonsEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedBackOrForwardNavigationButtons;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordClickedBookmarkEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedBookmark;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordClickedHomePageButtonEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedHomePageButton;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordClickedLinkEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClickedLink;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordClickedReloadButtonEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kClickedReloadButton;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordClosedTabEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClosedTab;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordFocusedOnExistingTabEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kFocusedOnExistingTab;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordGeneratedKeywordEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kGeneratedKeyword;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordNewNavigationEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kNewNavigation;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordOpenedLinkFromExternalApplicationEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kOpenedLinkFromExternalApplication;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordOpenedNewTabEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kOpenedNewTab;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordPlayedMediaEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kPlayedMedia;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordStoppedPlayingMediaEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kStoppedPlayingMedia;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordSubmittedFormEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kSubmittedForm;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordTabUpdatedEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kTabUpdated;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordTypedAndSelectedNonUrlEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTypedAndSelectedNonUrl;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest,
       RecordTypedKeywordOtherThanDefaultSearchProviderEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordTypedUrlEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kTypedUrl;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, RecordUsedAddressBarEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kUsedAddressBar;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = event_type;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, GetHistoryForTimeWindow) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kInitializedAds);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserDidBecomeActive);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserDidEnterBackground);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kClickedBackOrForwardNavigationButtons);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedBookmark);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kClickedHomePageButton);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedReloadButton);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kGeneratedKeyword);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kNewNavigation);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedLinkFromExternalApplication);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kStoppedPlayingMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kSubmittedForm);

  AdvanceClock(base::TimeDelta::FromHours(1));

  const base::Time time = Now();

  UserActivity::Get()->RecordEvent(UserActivityEventType::kTabUpdated);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kTypedAndSelectedNonUrl);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kUsedAddressBar);

  AdvanceClock(base::TimeDelta::FromHours(1));

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

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

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsUserActivityTest, MaximumHistoryEntries) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClosedTab;
  for (int i = 0; i < kMaximumHistoryEntries; i++) {
    UserActivity::Get()->RecordEvent(event_type);
  }

  // Act
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Assert
  UserActivityEventList expected_events;
  UserActivityEventInfo event;

  for (int i = 0; i < kMaximumHistoryEntries - 1; i++) {
    event.type = event_type;
    event.created_at = Now();
    expected_events.push_back(event);
  }

  event.type = UserActivityEventType::kOpenedNewTab;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

}  // namespace ads
