/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "brave/browser/ui/brave_ads/notification_ad_popup_handler.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup_collection.h"
#include "brave/components/brave_ads/browser/ad_units/notification_ad/custom_notification_ad_feature.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/test/views/snapshot/widget_snapshot_checker.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"  // IWYU pragma: keep
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/gfx/native_widget_types.h"

// npm run test -- brave_browser_tests --filter=NotificationAdPopupBrowserTest.*

namespace brave_ads {

class NotificationAdPopupBrowserTest : public InProcessBrowserTest {
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

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    brave::RegisterPathProvider();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(NotificationAdPopupBrowserTest, CheckThemeChanged) {
  // Check appearance in light theme.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

  const std::string notification_id = "notification_id";

  const NotificationAd ad(
      notification_id, u"Lorem ipsum dolor ac amet elit",
      u"Cras justo odio, dapibus ac facilisis in, egestas eget quam.", {});

  gfx::NativeWindow browser_native_window =
      browser()->window()->GetNativeWindow();
  EXPECT_TRUE(browser_native_window);

  gfx::NativeView browser_native_view =
      platform_util::GetViewForWindow(browser_native_window);
  EXPECT_TRUE(browser_native_view);

  NotificationAdPopupHandler::Show(browser()->profile(), ad,
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

}  // namespace brave_ads
