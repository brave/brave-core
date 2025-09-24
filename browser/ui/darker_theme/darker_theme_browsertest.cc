// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/browser/ui/darker_theme/features.h"
#include "brave/browser/ui/darker_theme/pref_names.h"
#include "brave/browser/ui/views/frame/brave_browser_widget.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "ui/color/color_provider_key.h"

using DarkerThemeWithFlagDisabledBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(DarkerThemeWithFlagDisabledBrowserTest,
                       PreferenceNotRegistered) {
  // By default, the darker theme feature flag should be off.
  EXPECT_FALSE(
      base::FeatureList::IsEnabled(darker_theme::features::kBraveDarkerTheme));

  // When the flag is off, the pref should not be registered.
  EXPECT_FALSE(browser()->profile()->GetPrefs()->FindPreference(
      darker_theme::prefs::kBraveDarkerMode));
}

class DarkerThemeBrowserTest : public InProcessBrowserTest {
 public:
  DarkerThemeBrowserTest()
      : scoped_feature_list_(darker_theme::features::kBraveDarkerTheme) {}
  ~DarkerThemeBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(DarkerThemeBrowserTest, EnableDarkerMode) {
  // By default, the darker theme should be off.
  ASSERT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(
      darker_theme::prefs::kBraveDarkerMode));

  auto* browser_view = static_cast<BrowserView*>(browser()->window());
  auto* browser_widget =
      static_cast<BraveBrowserWidget*>(browser_view->browser_widget());
  auto* theme_service =
      ThemeServiceFactory::GetForProfile(browser()->profile());
  ASSERT_TRUE(theme_service);
  theme_service->SetBrowserColorScheme(ThemeService::BrowserColorScheme::kDark);

  auto color_provider_key = browser_widget->GetColorProviderKey();
  ASSERT_FALSE(color_provider_key.scheme_variant.has_value());
  ASSERT_EQ(color_provider_key.color_mode,
            ui::ColorProviderKey::ColorMode::kDark);

  auto* prefs = browser()->profile()->GetPrefs();
  // Enable the darker theme.
  prefs->SetBoolean(darker_theme::prefs::kBraveDarkerMode, true);
  color_provider_key = browser_widget->GetColorProviderKey();
  EXPECT_TRUE(color_provider_key.scheme_variant.has_value());
  EXPECT_EQ(*color_provider_key.scheme_variant,
            ui::ColorProviderKey::SchemeVariant::kDarker);

  auto* color_provider = browser_widget->GetColorProvider();
  ASSERT_TRUE(color_provider);
  EXPECT_EQ(color_provider->GetColor(kColorForTest), kDarkerColorForTest);

  // Disable the darker theme.
  prefs->SetBoolean(darker_theme::prefs::kBraveDarkerMode, false);
  color_provider_key = browser_widget->GetColorProviderKey();
  EXPECT_FALSE(color_provider_key.scheme_variant.has_value());

  color_provider = browser_widget->GetColorProvider();
  EXPECT_EQ(color_provider->GetColor(kColorForTest), kDarkColorForTest);
}

class DarkerThemeFeatureToggleOffBrowserTest : public InProcessBrowserTest {
 public:
  DarkerThemeFeatureToggleOffBrowserTest() {
    if (GetTestPreCount()) {
      // If this is a PRE_ test, we want to enable the feature flag.
      scoped_feature_list_.InitAndEnableFeature(
          darker_theme::features::kBraveDarkerTheme);
    } else {
      // When this is a main test that runs after the PRE_ test, disable the
      // feature flag.
      scoped_feature_list_.InitAndDisableFeature(
          darker_theme::features::kBraveDarkerTheme);
    }
  }
  ~DarkerThemeFeatureToggleOffBrowserTest() override = default;

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(DarkerThemeFeatureToggleOffBrowserTest,
                       PRE_FeatureDisabledByUsers) {
  ASSERT_TRUE(
      base::FeatureList::IsEnabled(darker_theme::features::kBraveDarkerTheme))
      << "Feature flag should be disabled from PRE_FeatureDisabledByUsers.";

  // In this PRE_ test, we turns on Darker theme preference and then turns off
  // the feature flag.
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(darker_theme::prefs::kBraveDarkerMode, true);
}

IN_PROC_BROWSER_TEST_F(DarkerThemeFeatureToggleOffBrowserTest,
                       FeatureDisabledByUsers) {
  ASSERT_FALSE(
      base::FeatureList::IsEnabled(darker_theme::features::kBraveDarkerTheme))
      << "Feature flag should be disabled from PRE_FeatureDisabledByUsers.";

  // After the feature flag is turned off, the preference should be removed.
  auto* prefs = browser()->profile()->GetPrefs();
  EXPECT_FALSE(prefs->FindPreference(darker_theme::prefs::kBraveDarkerMode));

  // Even if the preference was previously set to true, the darker theme
  // should not be applied when the feature flag is off.
  auto* browser_view = static_cast<BrowserView*>(browser()->window());
  auto* browser_widget =
      static_cast<BraveBrowserWidget*>(browser_view->browser_widget());
  auto* theme_service =
      ThemeServiceFactory::GetForProfile(browser()->profile());
  ASSERT_TRUE(theme_service);
  theme_service->SetBrowserColorScheme(ThemeService::BrowserColorScheme::kDark);

  auto color_provider_key = browser_widget->GetColorProviderKey();
  ASSERT_EQ(color_provider_key.color_mode,
            ui::ColorProviderKey::ColorMode::kDark);
  EXPECT_FALSE(color_provider_key.scheme_variant.has_value())
      << "Scheme variant shouldn't be kDarker when the feature flag is off.";
}
