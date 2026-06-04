/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/side_panel_utils.h"

#include <memory>

#include "brave/common/pref_names.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/layout/layout_provider.h"

namespace brave {

class SidePanelUtilsTest : public ChromeViewsTestBase {
 public:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    ChromeViewsTestBase::SetUp();
  }

  void TearDown() override {
    profile_.reset();
    ChromeViewsTestBase::TearDown();
  }

 protected:
  PrefService* prefs() { return profile_->GetPrefs(); }

  int ContentRadius() const {
    return views::LayoutProvider::Get()->GetDistanceMetric(
        ChromeDistanceMetric::DISTANCE_SIDE_PANEL_CONTENT_RADIUS);
  }

  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(SidePanelUtilsTest, ShouldShowSidePanelHeaderOnlyForBraveEntries) {
  EXPECT_TRUE(ShouldShowSidePanelHeader(SidePanelEntryId::kReadingList));
  EXPECT_TRUE(ShouldShowSidePanelHeader(SidePanelEntryId::kBookmarks));
  EXPECT_FALSE(ShouldShowSidePanelHeader(SidePanelEntryId::kCustomizeChrome));
}

TEST_F(SidePanelUtilsTest, RoundedCornersEmptyWhenPrefDisabled) {
  prefs()->SetBoolean(kWebViewRoundedCorners, false);
  EXPECT_EQ(gfx::RoundedCornersF(),
            GetPanelContentsRoundedCorners(prefs(), false));
  EXPECT_EQ(gfx::RoundedCornersF(),
            GetPanelContentsRoundedCorners(prefs(), true));
}

TEST_F(SidePanelUtilsTest, RoundedCornersAllCornersWhenNoHeader) {
  prefs()->SetBoolean(kWebViewRoundedCorners, true);
  EXPECT_EQ(gfx::RoundedCornersF(ContentRadius()),
            GetPanelContentsRoundedCorners(prefs(), false));
}

TEST_F(SidePanelUtilsTest, RoundedCornersFlatTopWhenHasHeader) {
  prefs()->SetBoolean(kWebViewRoundedCorners, true);
  const int r = ContentRadius();
  EXPECT_EQ(gfx::RoundedCornersF(0, 0, r, r),
            GetPanelContentsRoundedCorners(prefs(), true));
}

}  // namespace brave
