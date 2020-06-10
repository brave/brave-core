/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_theme_event_router.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"

using BraveThemeEventRouterBrowserTest = InProcessBrowserTest;

namespace extensions {

class MockBraveThemeEventRouter : public BraveThemeEventRouter {
 public:
  using BraveThemeEventRouter::BraveThemeEventRouter;
  ~MockBraveThemeEventRouter() override {}

  MOCK_METHOD0(Notify, void());
};

}  // namespace extensions

IN_PROC_BROWSER_TEST_F(BraveThemeEventRouterBrowserTest,
                       ThemeChangeTest) {
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);

  extensions::MockBraveThemeEventRouter* mock_router =
      new extensions::MockBraveThemeEventRouter(browser()->profile());
  BraveThemeService* service = static_cast<BraveThemeService*>(
      ThemeServiceFactory::GetForProfile(browser()->profile()));
  service->SetBraveThemeEventRouterForTesting(mock_router);

  EXPECT_CALL(*mock_router, Notify()).Times(1);
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

  EXPECT_CALL(*mock_router, Notify()).Times(1);
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);

  EXPECT_CALL(*mock_router, Notify()).Times(0);
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
}
