/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/notification_ad_popup.h"

#include <tuple>

#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "brave/browser/ui/brave_ads/notification_ad_popup_handler.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup_collection.h"
#include "brave/components/brave_ads/browser/ad_units/notification_ad/custom_notification_ad_feature.h"
#include "brave/test/views/snapshot/widget_snapshot_checker.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/gfx/native_widget_types.h"

// npm run test -- brave_browser_tests --filter=NotificationAdPopupBrowserTest.*

namespace brave_ads {

namespace {

std::string TestParamToString(
    const ::testing::TestParamInfo<
        std::tuple<std::string, std::u16string, std::u16string>>& test_param) {
  const auto& [test_name, title, body] = test_param.param;
  return test_name;
}

}  // namespace

class NotificationAdPopupBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<
          std::tuple<std::string, std::u16string, std::u16string>> {
 public:
  NotificationAdPopupBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(kCustomNotificationAdFeature);
  }

  void SetUp() override {
    EnablePixelOutput();
    NotificationAdPopup::SetDisableFadeInAnimationForTesting(true);
    InProcessBrowserTest::SetUp();
  }

  void TearDown() override {
    NotificationAdPopup::SetDisableFadeInAnimationForTesting(false);
    InProcessBrowserTest::TearDown();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(NotificationAdPopupBrowserTest, CheckThemeChanged) {
  // Check appearance in light theme.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

  const std::string notification_id = "notification_id";
  const auto& [_, notification_title, notification_body] = GetParam();

  const NotificationAd ad(notification_id, notification_title,
                          notification_body, {});

  gfx::NativeWindow browser_native_window =
      browser()->window()->GetNativeWindow();
  EXPECT_TRUE(browser_native_window);

  gfx::NativeView browser_native_view =
      platform_util::GetViewForWindow(browser_native_window);
  EXPECT_TRUE(browser_native_view);

  NotificationAdPopupHandler::Show(*browser()->profile(), ad,
                                   browser_native_window, browser_native_view);

  NotificationAdPopup* popup =
      NotificationAdPopupCollection::Get(notification_id);
  ASSERT_TRUE(popup);

  WidgetSnapshotChecker widget_snapshot_checker;
  EXPECT_NO_FATAL_FAILURE(
      widget_snapshot_checker.CaptureAndCheckSnapshot(popup->GetWidget()));

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  // Check appearance in dark theme.
  EXPECT_NO_FATAL_FAILURE(
      widget_snapshot_checker.CaptureAndCheckSnapshot(popup->GetWidget()));

  NotificationAdPopupHandler::Close(notification_id, false);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    NotificationAdPopupBrowserTest,
    testing::Values(
        std::make_tuple("WithEmoji",
                        u"🔥 Lorem ipsum dolor ac amet elit 🔥",
                        u"🔥 Cras justo odio, dapibus ac facilisis "
                        u"in, egestas eget quam. 🔥"),
        std::make_tuple(
            "WithoutEmoji",
            u"Lorem ipsum dolor ac amet elit",
            u"Cras justo odio, dapibus ac facilisis in, egestas eget quam.")),
    &TestParamToString);

}  // namespace brave_ads
