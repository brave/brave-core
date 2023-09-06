/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_attention/user_activity/page_transition_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPageTransitionUtilTest, IsNewNavigation) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsNewNavigation(kPageTransitionTyped));
}

TEST(BraveAdsPageTransitionUtilTest,
     DidUseBackOrFowardButtonToTriggerNavigation) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      DidUseBackOrFowardButtonToTriggerNavigation(kPageTransitionForwardBack));
}

TEST(BraveAdsPageTransitionUtilTest, DidUseAddressBarToTriggerNavigation) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      DidUseAddressBarToTriggerNavigation(kPageTransitionFromAddressBar));
}

TEST(BraveAdsPageTransitionUtilTest, DidNavigateToHomePage) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DidNavigateToHomePage(kPageTransitionHomePage));
}

TEST(BraveAdsPageTransitionUtilTest, DidTransitionFromExternalApplication) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DidTransitionFromExternalApplication(kPageTransitionFromAPI));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivityClickedLinkEventType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(UserActivityEventType::kClickedLink,
            ToUserActivityEventType(kPageTransitionLink));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivityTypedUrlEventType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(UserActivityEventType::kTypedUrl,
            ToUserActivityEventType(kPageTransitionTyped));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivityClickedBookmarkEventType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(UserActivityEventType::kClickedBookmark,
            ToUserActivityEventType(kPageTransitionAutoBookmark));
}

TEST(BraveAdsPageTransitionUtilTest,
     ToUserActivityTypedAndSelectedNonUrlEventType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(UserActivityEventType::kTypedAndSelectedNonUrl,
            ToUserActivityEventType(kPageTransitionGenerated));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivitySubmittedFormEventType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(UserActivityEventType::kSubmittedForm,
            ToUserActivityEventType(kPageTransitionFormSubmit));
}

TEST(BraveAdsPageTransitionUtilTest,
     ToUserActivityClickedReloadButtonEventType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(UserActivityEventType::kClickedReloadButton,
            ToUserActivityEventType(kPageTransitionReload));
}

TEST(BraveAdsPageTransitionUtilTest,
     ToUserActivityTypedKeywordOtherThanDefaultSearchProviderEventType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider,
            ToUserActivityEventType(kPageTransitionKeyword));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivityGeneratedKeywordEventType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(UserActivityEventType::kGeneratedKeyword,
            ToUserActivityEventType(kPageTransitionKeywordGenerated));
}

}  // namespace brave_ads
