// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/color/features.h"
#include "brave/browser/ui/color/pref_names.h"
#include "brave/browser/ui/views/frame/brave_browser_frame.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "ui/color/color_provider_key.h"

class DarkerThemeBrowserTest : public InProcessBrowserTest {
 public:
  DarkerThemeBrowserTest()
      : scoped_feature_list_(color::features::kBraveDarkerTheme) {}
  ~DarkerThemeBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(DarkerThemeBrowserTest, EnableDarkerMode) {
  // By default, the darker theme should be off.
  ASSERT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(
      color::prefs::kBraveDarkerMode));

  auto* browser_view = static_cast<BrowserView*>(browser()->window());
  auto* browser_frame = static_cast<BraveBrowserFrame*>(browser_view->frame());
  auto* theme_service =
      ThemeServiceFactory::GetForProfile(browser()->profile());
  ASSERT_TRUE(theme_service);
  theme_service->SetBrowserColorScheme(ThemeService::BrowserColorScheme::kDark);

  auto color_provider_key = browser_frame->GetColorProviderKey();
  ASSERT_FALSE(color_provider_key.scheme_variant.has_value());
  ASSERT_EQ(color_provider_key.color_mode,
            ui::ColorProviderKey::ColorMode::kDark);

  auto* prefs = browser()->profile()->GetPrefs();
  // Enable the darker theme.
  prefs->SetBoolean(color::prefs::kBraveDarkerMode, true);
  color_provider_key = browser_frame->GetColorProviderKey();
  EXPECT_TRUE(color_provider_key.scheme_variant.has_value());
  EXPECT_EQ(*color_provider_key.scheme_variant,
            ui::ColorProviderKey::SchemeVariant::kDarker);

  // Disable the darker theme.
  prefs->SetBoolean(color::prefs::kBraveDarkerMode, false);
  color_provider_key = browser_frame->GetColorProviderKey();
  EXPECT_FALSE(color_provider_key.scheme_variant.has_value());
}
