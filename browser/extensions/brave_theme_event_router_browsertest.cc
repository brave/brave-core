/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_theme_event_router.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/native_theme/native_theme_dark_aura.h"

using BraveThemeEventRouterBrowserTest = InProcessBrowserTest;

namespace {

void SetBraveThemeType(Profile* profile, BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}

}  // namespace

namespace extensions {

class MockBraveThemeEventRouter : public BraveThemeEventRouter {
 public:
  using BraveThemeEventRouter::BraveThemeEventRouter;
  ~MockBraveThemeEventRouter() override {}

  ui::NativeTheme* current_native_theme_for_testing() const {
    return current_native_theme_for_testing_;
  }

  MOCK_METHOD0(Notify, void());
};

}  // namespace extensions


IN_PROC_BROWSER_TEST_F(BraveThemeEventRouterBrowserTest,
                       ThemeChangeTest) {
  Profile* profile = browser()->profile();
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_DARK);

  extensions::MockBraveThemeEventRouter* mock_router =
      new extensions::MockBraveThemeEventRouter(profile);
  BraveThemeService* service = static_cast<BraveThemeService*>(
      ThemeServiceFactory::GetForProfile(browser()->profile()));
  service->SetBraveThemeEventRouterForTesting(mock_router);

  EXPECT_CALL(*mock_router, Notify()).Times(1);
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_LIGHT);
  EXPECT_EQ(
      ui::NativeTheme::GetInstanceForNativeUi(),
      mock_router->current_native_theme_for_testing());

  EXPECT_CALL(*mock_router, Notify()).Times(1);
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_DARK);
  EXPECT_EQ(
      ui::NativeThemeDarkAura::instance(),
      mock_router->current_native_theme_for_testing());

  EXPECT_CALL(*mock_router, Notify()).Times(0);
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_DARK);
}
