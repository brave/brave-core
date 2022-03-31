/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/run_loop.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/brave_ads/ad_notification.h"
#include "brave/browser/ui/brave_ads/ad_notification_popup_handler.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_popup.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_popup_collection.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/test/views/snapshot/widget_snapshot_checker.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

// npm run test -- brave_browser_tests --filter=AdNotificationPopupBrowserTest.*

namespace brave_ads {

class AdNotificationPopupBrowserTest : public InProcessBrowserTest {
 public:
  AdNotificationPopupBrowserTest() {
    feature_list_.InitAndEnableFeature(
        brave_ads::features::kCustomAdNotifications);
  }

  void SetUp() override {
    EnablePixelOutput();
    AdNotificationPopup::SetDisableFadeInAnimationForTesting(true);
    InProcessBrowserTest::SetUp();
  }

  void TearDown() override {
    AdNotificationPopup::SetDisableFadeInAnimationForTesting(false);
    InProcessBrowserTest::TearDown();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    brave::RegisterPathProvider();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(AdNotificationPopupBrowserTest, CheckThemeChanged) {
  // Check appearance in light theme.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  std::string notification_id = "notification_id";
  AdNotification notification(notification_id, u"test", u"test", {});
  gfx::NativeWindow browser_native_window =
      browser()->window()->GetNativeWindow();
  EXPECT_TRUE(browser_native_window);
  gfx::NativeView browser_native_view =
      platform_util::GetViewForWindow(browser_native_window);
  EXPECT_TRUE(browser_native_view);

  AdNotificationPopupHandler::Show(browser()->profile(), notification,
                                   browser_native_window, browser_native_view);
  AdNotificationPopup* popup =
      AdNotificationPopupCollection::Get(notification_id);
  ASSERT_TRUE(popup);

  WidgetSnapshotChecker widget_checker;
  EXPECT_NO_FATAL_FAILURE(
      widget_checker.CaptureAndCheckSnapshot(popup->GetWidget()));

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  // Check appearance in dark theme.
  EXPECT_NO_FATAL_FAILURE(
      widget_checker.CaptureAndCheckSnapshot(popup->GetWidget()));

  AdNotificationPopupHandler::Close(notification_id, false);
}

}  // namespace brave_ads
