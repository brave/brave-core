/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/run_loop.h"
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
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/views/widget/widget.h"

// npm run test -- brave_browser_tests --filter=AdNotificationPopupBrowserTest.*

namespace brave_ads {

namespace {

class MockAdNotificationPopup : public AdNotificationPopup {
 public:
  MockAdNotificationPopup(Profile* profile,
                          const AdNotification& ad_notification)
      : AdNotificationPopup(profile, ad_notification) {}

  void OnThemeChanged() override {
    AdNotificationPopup::OnThemeChanged();
    OnThemeChangedMock();
  }

  MOCK_METHOD0(OnThemeChangedMock, void());
};

class TestPopupInstanceFactory
    : public AdNotificationPopup::PopupInstanceFactory {
 public:
  ~TestPopupInstanceFactory() override = default;

  AdNotificationPopup* CreateInstance(
      Profile* profile,
      const AdNotification& ad_notification) override {
    popup_ = new MockAdNotificationPopup(profile, ad_notification);
    return popup_;
  }

  MockAdNotificationPopup* get_popup() { return popup_; }

 private:
  MockAdNotificationPopup* popup_ = nullptr;
};

}  // namespace

class AdNotificationPopupBrowserTest : public InProcessBrowserTest {
 public:
  AdNotificationPopupBrowserTest() {
    feature_list_.InitAndEnableFeatureWithParameters(
        brave_ads::features::kAdNotifications,
        {{"should_show_custom_notifications", "true"}});
  }

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(AdNotificationPopupBrowserTest,
                       CheckDynamicThemeChange) {
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

  TestPopupInstanceFactory test_factory;
  AdNotification notification("id", u"test", u"test", {});
  AdNotificationPopup::Show(browser()->profile(), notification, &test_factory);

  MockAdNotificationPopup* mock_popup = test_factory.get_popup();
  ASSERT_TRUE(mock_popup);

  base::RunLoop run_loop;
  EXPECT_CALL(*mock_popup, OnThemeChangedMock()).WillOnce([&run_loop]() {
    run_loop.Quit();
  });

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  run_loop.Run();
  AdNotificationPopup::CloseWidget(notification.id());
}

class AdNotificationSnapshotBrowserTest
    : public AdNotificationPopupBrowserTest {
 public:
  AdNotificationSnapshotBrowserTest() {}

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
};  // namespace brave_ads

// Snapshots are not properly taken on MacOS for now.
#if defined(OS_MAC)
#define MAYBE_ShowPopup DISABLED_ShowPopup
#else
#define MAYBE_ShowPopup ShowPopup
#endif  // defined(OS_MAC)
IN_PROC_BROWSER_TEST_F(AdNotificationSnapshotBrowserTest, MAYBE_ShowPopup) {
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
