/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_header_controller.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/test/mock_callback.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_key.h"
#include "chrome/browser/ui/side_panel/side_panel_native_view.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"

using ::testing::NiceMock;
using ::testing::Return;

class BraveSidePanelHeaderControllerTest : public ChromeViewsTestBase {
 public:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    ChromeViewsTestBase::SetUp();

    mock_browser_window_ =
        std::make_unique<NiceMock<MockBrowserWindowInterface>>();
    ON_CALL(*mock_browser_window_, GetProfile())
        .WillByDefault(Return(profile_.get()));

    entry_ = std::make_unique<SidePanelEntry>(
        SidePanelEntryKey(SidePanelEntry::Id::kReadingList),
        base::BindRepeating([](SidePanelEntryScope&) {
          return SidePanelNativeView(std::make_unique<views::View>());
        }),
        /*default_content_width_callback=*/base::NullCallback());

    controller_ = std::make_unique<BraveSidePanelHeaderController>(
        *mock_browser_window_, entry_.get());
  }

  void TearDown() override {
    controller_.reset();
    entry_.reset();
    mock_browser_window_.reset();
    profile_.reset();
    ChromeViewsTestBase::TearDown();
  }

 protected:
  PrefService* prefs() { return profile_->GetPrefs(); }

  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<NiceMock<MockBrowserWindowInterface>> mock_browser_window_;
  std::unique_ptr<SidePanelEntry> entry_;
  std::unique_ptr<BraveSidePanelHeaderController> controller_;
};

TEST_F(BraveSidePanelHeaderControllerTest, GetTopRadiusIsZeroWhenPrefDisabled) {
  prefs()->SetBoolean(kWebViewRoundedCorners, false);
  EXPECT_EQ(0, controller_->GetTopRadius());
}

TEST_F(BraveSidePanelHeaderControllerTest,
       GetTopRadiusMatchesLayoutMetricWhenPrefEnabled) {
  prefs()->SetBoolean(kWebViewRoundedCorners, true);
  const int expected = views::LayoutProvider::Get()->GetCornerRadiusMetric(
      views::ShapeContextTokensOverride::kRoundedCornersBorderRadius);
  EXPECT_EQ(expected, controller_->GetTopRadius());
}

TEST_F(BraveSidePanelHeaderControllerTest,
       PrefChangeRunsRegisteredUpdateCallback) {
  base::MockRepeatingClosure mock_callback;
  controller_->SetUpdateHeaderCallback(mock_callback.Get());

  EXPECT_CALL(mock_callback, Run()).Times(1);
  prefs()->SetBoolean(kWebViewRoundedCorners, true);
  testing::Mock::VerifyAndClearExpectations(&mock_callback);

  EXPECT_CALL(mock_callback, Run()).Times(1);
  prefs()->SetBoolean(kWebViewRoundedCorners, false);
}

TEST_F(BraveSidePanelHeaderControllerTest,
       PrefChangeWithoutRegisteredCallbackDoesNotCrash) {
  // No callback registered. Flipping the pref must not crash.
  prefs()->SetBoolean(kWebViewRoundedCorners, true);
  prefs()->SetBoolean(kWebViewRoundedCorners, false);
}
