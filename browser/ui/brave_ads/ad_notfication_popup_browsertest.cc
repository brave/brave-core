/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/brave_ads/ad_notification.h"
#include "brave/browser/ui/brave_ads/ad_notification_popup.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/features.h"
#include "brave/test/snapshot/widget_snapshot_checker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/views/widget/widget.h"

// npm run test -- brave_browser_tests --filter=AdNotificationPopupBrowserTest.*

namespace brave_ads {

class AdNotificationPopupBrowserTest : public InProcessBrowserTest {
 public:
  AdNotificationPopupBrowserTest() {
    feature_list_.InitAndEnableFeatureWithParameters(
        brave_ads::features::kAdNotifications,
        {{"should_show_custom_notifications", "true"}});
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

  base::test::ScopedFeatureList feature_list_;
};  // namespace brave_ads

// Snapshots are not properly taken on MacOS for now.
#if defined(OS_MAC)
#define MAYBE_ShowPopup DISABLED_ShowPopup
#else
#define MAYBE_ShowPopup ShowPopup
#endif  // defined(OS_MAC)
IN_PROC_BROWSER_TEST_F(AdNotificationPopupBrowserTest, MAYBE_ShowPopup) {
  WidgetSnapshotChecker widget_checker;
  auto check_ads_popup = [&widget_checker, this](base::StringPiece id) {
    AdNotification notification(id.data(), u"test", u"test", {});
    AdNotificationPopup::Show(browser()->profile(), notification);

    ASSERT_NO_FATAL_FAILURE(widget_checker.CaptureAndCheckSnapshot(
        AdNotificationPopup::GetWidgetForTesting(notification.id())));
    AdNotificationPopup::CloseWidget(notification.id());
  };

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  check_ads_popup("id_light");

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  check_ads_popup("id_dark");
}

}  // namespace brave_ads
