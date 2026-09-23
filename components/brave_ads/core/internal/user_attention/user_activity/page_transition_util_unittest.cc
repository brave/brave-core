/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/page_transition_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPageTransitionUtilTest, ToUserActivityClickedLinkEventType) {
  // Act & Assert
  EXPECT_EQ(UserActivityEventType::kClickedLink,
            ToUserActivityEventType(ui::PageTransition::PAGE_TRANSITION_LINK));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivityTypedUrlEventType) {
  // Act & Assert
  EXPECT_EQ(UserActivityEventType::kTypedUrl,
            ToUserActivityEventType(ui::PageTransition::PAGE_TRANSITION_TYPED));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivityClickedBookmarkEventType) {
  // Act & Assert
  EXPECT_EQ(UserActivityEventType::kClickedBookmark,
            ToUserActivityEventType(
                ui::PageTransition::PAGE_TRANSITION_AUTO_BOOKMARK));
}

TEST(BraveAdsPageTransitionUtilTest,
     ToUserActivityTypedAndSelectedNonUrlEventType) {
  // Act & Assert
  EXPECT_EQ(
      UserActivityEventType::kTypedAndSelectedNonUrl,
      ToUserActivityEventType(ui::PageTransition::PAGE_TRANSITION_GENERATED));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivitySubmittedFormEventType) {
  // Act & Assert
  EXPECT_EQ(
      UserActivityEventType::kSubmittedForm,
      ToUserActivityEventType(ui::PageTransition::PAGE_TRANSITION_FORM_SUBMIT));
}

TEST(BraveAdsPageTransitionUtilTest,
     ToUserActivityClickedReloadButtonEventType) {
  // Act & Assert
  EXPECT_EQ(
      UserActivityEventType::kClickedReloadButton,
      ToUserActivityEventType(ui::PageTransition::PAGE_TRANSITION_RELOAD));
}

TEST(BraveAdsPageTransitionUtilTest,
     ToUserActivityTypedKeywordOtherThanDefaultSearchProviderEventType) {
  // Act & Assert
  EXPECT_EQ(
      UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider,
      ToUserActivityEventType(ui::PageTransition::PAGE_TRANSITION_KEYWORD));
}

TEST(BraveAdsPageTransitionUtilTest, ToUserActivityGeneratedKeywordEventType) {
  // Act & Assert
  EXPECT_EQ(UserActivityEventType::kGeneratedKeyword,
            ToUserActivityEventType(
                ui::PageTransition::PAGE_TRANSITION_KEYWORD_GENERATED));
}

}  // namespace brave_ads
