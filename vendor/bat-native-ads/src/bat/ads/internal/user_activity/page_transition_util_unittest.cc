/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/page_transition_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsPageTransitionTest, IsNewNavigation) {
  // Arrange

  // Act
  const bool did_transition_page = IsNewNavigation(kPageTransitionTyped);

  // Assert
  EXPECT_TRUE(did_transition_page);
}

TEST(BatAdsPageTransitionTest, DidUseBackOrFowardButtonToTriggerNavigation) {
  // Arrange

  // Act
  const bool did_transition_page =
      DidUseBackOrFowardButtonToTriggerNavigation(kPageTransitionForwardBack);

  // Assert
  EXPECT_TRUE(did_transition_page);
}

TEST(BatAdsPageTransitionTest, DidUseAddressBarToTriggerNavigation) {
  // Arrange

  // Act
  const bool did_transition_page =
      DidUseAddressBarToTriggerNavigation(kPageTransitionFromAddressBar);

  // Assert
  EXPECT_TRUE(did_transition_page);
}

TEST(BatAdsPageTransitionTest, DidNavigateToHomePage) {
  // Arrange

  // Act
  const bool did_transition_page =
      DidNavigateToHomePage(kPageTransitionHomePage);

  // Assert
  EXPECT_TRUE(did_transition_page);
}

TEST(BatAdsPageTransitionTest, DidTransitionFromExternalApplication) {
  // Arrange

  // Act
  const bool did_transition_page =
      DidTransitionFromExternalApplication(kPageTransitionFromAPI);

  // Assert
  EXPECT_TRUE(did_transition_page);
}

TEST(BatAdsPageTransitionTest, ToUserActivityClickedLinkEventType) {
  // Arrange

  // Act
  absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(kPageTransitionLink);

  // Assert
  EXPECT_EQ(UserActivityEventType::kClickedLink, event_type);
}

TEST(BatAdsPageTransitionTest, ToUserActivityTypedUrlEventType) {
  // Arrange

  // Act
  absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(kPageTransitionTyped);

  // Assert
  EXPECT_EQ(UserActivityEventType::kTypedUrl, event_type);
}

TEST(BatAdsPageTransitionTest, ToUserActivityClickedBookmarkEventType) {
  // Arrange

  // Act
  absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(kPageTransitionAutoBookmark);

  // Assert
  EXPECT_EQ(UserActivityEventType::kClickedBookmark, event_type);
}

TEST(BatAdsPageTransitionTest, ToUserActivityTypedAndSelectedNonUrlEventType) {
  // Arrange

  // Act
  absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(kPageTransitionGenerated);

  // Assert
  EXPECT_EQ(UserActivityEventType::kTypedAndSelectedNonUrl, event_type);
}

TEST(BatAdsPageTransitionTest, ToUserActivitySubmittedFormEventType) {
  // Arrange

  // Act
  absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(kPageTransitionFormSubmit);

  // Assert
  EXPECT_EQ(UserActivityEventType::kSubmittedForm, event_type);
}

TEST(BatAdsPageTransitionTest, ToUserActivityClickedReloadButtonEventType) {
  // Arrange

  // Act
  absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(kPageTransitionReload);

  // Assert
  EXPECT_EQ(UserActivityEventType::kClickedReloadButton, event_type);
}

TEST(BatAdsPageTransitionTest,
     ToUserActivityTypedKeywordOtherThanDefaultSearchProviderEventType) {
  // Arrange

  // Act
  absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(kPageTransitionKeyword);

  // Assert
  EXPECT_EQ(UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider,
            event_type);
}

TEST(BatAdsPageTransitionTest, ToUserActivityGeneratedKeywordEventType) {
  // Arrange

  // Act
  absl::optional<UserActivityEventType> event_type =
      ToUserActivityEventType(kPageTransitionKeywordGenerated);

  // Assert
  EXPECT_EQ(UserActivityEventType::kGeneratedKeyword, event_type);
}

}  // namespace ads
