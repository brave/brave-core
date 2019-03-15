/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"
#include "brave/browser/extensions/brave_theme_event_router.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"

using BraveThemeAPIBrowserTest = InProcessBrowserTest;

namespace {
class MockBraveThemeEventRouter : public extensions::BraveThemeEventRouter {
 public:
  MockBraveThemeEventRouter() {}
  ~MockBraveThemeEventRouter() override {}

  MOCK_METHOD1(OnBraveThemeTypeChanged, void(Profile*));
};

void SetBraveThemeType(Profile* profile, BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}
}  // namespace

IN_PROC_BROWSER_TEST_F(BraveThemeAPIBrowserTest, BraveThemeEventRouterTest) {
  Profile* profile = browser()->profile();
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_DARK);

  MockBraveThemeEventRouter* mock_router_ = new MockBraveThemeEventRouter;
  EXPECT_CALL(*mock_router_, OnBraveThemeTypeChanged(profile)).Times(1);

  BraveThemeService* service = static_cast<BraveThemeService*>(
      ThemeServiceFactory::GetForProfile(browser()->profile()));
  service->SetBraveThemeEventRouterForTesting(mock_router_);
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_LIGHT);
}
