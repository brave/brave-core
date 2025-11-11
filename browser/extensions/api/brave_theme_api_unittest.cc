/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"

#include "chrome/browser/extensions/extension_api_unittest.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

using BraveThemeAPITest = extensions::ExtensionApiUnittest;
using extensions::api::BraveThemeSetBraveThemeTypeFunction;

TEST_F(BraveThemeAPITest, SetBraveThemeTypeTest) {
  auto* theme_service = ThemeServiceFactory::GetForProfile(profile());
  CHECK(theme_service);

  scoped_refptr<BraveThemeSetBraveThemeTypeFunction> set_dark_function(
      new BraveThemeSetBraveThemeTypeFunction());
  RunFunction(set_dark_function.get(), R"(["Dark"])");
  EXPECT_EQ(ThemeService::BrowserColorScheme::kDark,
            theme_service->GetBrowserColorScheme());

  scoped_refptr<BraveThemeSetBraveThemeTypeFunction> set_light_function(
      new BraveThemeSetBraveThemeTypeFunction());
  RunFunction(set_light_function.get(), R"(["Light"])");
  EXPECT_EQ(ThemeService::BrowserColorScheme::kLight,
            theme_service->GetBrowserColorScheme());

  scoped_refptr<BraveThemeSetBraveThemeTypeFunction> set_system_function(
      new BraveThemeSetBraveThemeTypeFunction());
  RunFunction(set_system_function.get(), R"(["System"])");
  EXPECT_EQ(ThemeService::BrowserColorScheme::kSystem,
            theme_service->GetBrowserColorScheme());
}
