/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/side_panel_utils.h"

#include <memory>

#include "brave/common/pref_names.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rounded_corners_f.h"

using testing::Return;

namespace brave {

class SidePanelUtilsTest : public ChromeViewsTestBase {
 public:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    ChromeViewsTestBase::SetUp();
    ON_CALL(mock_interface_, GetProfile())
        .WillByDefault(Return(profile_.get()));
  }

  void TearDown() override {
    profile_.reset();
    ChromeViewsTestBase::TearDown();
  }

 protected:
  PrefService* prefs() { return profile_->GetPrefs(); }

  std::unique_ptr<TestingProfile> profile_;
  testing::NiceMock<MockBrowserWindowInterface> mock_interface_;
};

TEST_F(SidePanelUtilsTest, ShouldShowSidePanelHeaderOnlyForBraveEntries) {
  EXPECT_TRUE(ShouldShowSidePanelHeader(SidePanelEntryId::kReadingList));
  EXPECT_TRUE(ShouldShowSidePanelHeader(SidePanelEntryId::kBookmarks));
  EXPECT_FALSE(ShouldShowSidePanelHeader(SidePanelEntryId::kCustomizeChrome));
}

// BrowserView::GetBrowserViewForBrowser() returns null in unit-test
// environments (no real native window). GetPanelContentsRoundedCorners() must
// return empty corners in that case, matching the null-safety guard for the
// startup path.
TEST_F(SidePanelUtilsTest, RoundedCornersEmptyWhenBrowserViewIsNull) {
  prefs()->SetBoolean(kWebViewRoundedCorners, true);
  EXPECT_EQ(gfx::RoundedCornersF(),
            GetPanelContentsRoundedCorners(&mock_interface_, false));
  EXPECT_EQ(gfx::RoundedCornersF(),
            GetPanelContentsRoundedCorners(&mock_interface_, true));
}

TEST_F(SidePanelUtilsTest, RoundedCornersEmptyWhenPrefDisabled) {
  prefs()->SetBoolean(kWebViewRoundedCorners, false);
  EXPECT_EQ(gfx::RoundedCornersF(),
            GetPanelContentsRoundedCorners(&mock_interface_, false));
  EXPECT_EQ(gfx::RoundedCornersF(),
            GetPanelContentsRoundedCorners(&mock_interface_, true));
}

}  // namespace brave
