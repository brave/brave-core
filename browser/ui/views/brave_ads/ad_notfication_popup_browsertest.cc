/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/run_loop.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/brave_ads/ad_notification.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_popup.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/test/views/snapshot/widget_snapshot_checker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
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
  WidgetSnapshotChecker widget_checker;
  TestPopupInstanceFactory test_factory;
  auto check_ads_popup_snapshot = [&widget_checker, &test_factory,
                                   this](const std::string& id) {
    AdNotification notification(id, u"test", u"test", {});
    AdNotificationPopup::Show(browser()->profile(), notification,
                              &test_factory);
    MockAdNotificationPopup* mock_popup = test_factory.get_popup();
    ASSERT_TRUE(mock_popup);

    ASSERT_NO_FATAL_FAILURE(
        widget_checker.CaptureAndCheckSnapshot(mock_popup->GetWidget()));
  };

  // Check appearance in light theme.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  std::string notification_id = "id_light";
  check_ads_popup_snapshot(notification_id);
  AdNotificationPopup::CloseWidget(notification_id);

  // Check appearance in light theme.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  notification_id = "id_dark";
  check_ads_popup_snapshot(notification_id);

  // Check that OnThemeChanged() is called on browser theme change.
  base::RunLoop run_loop;
  EXPECT_CALL(*test_factory.get_popup(), OnThemeChangedMock())
      .WillOnce([&run_loop]() { run_loop.Quit(); });
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  run_loop.Run();

  AdNotificationPopup::CloseWidget(notification_id);
}

}  // namespace brave_ads
