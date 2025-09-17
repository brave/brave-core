/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include <memory>

#include "brave/browser/extensions/brave_theme_event_router.h"
#include "chrome/browser/profiles/profile.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/darker_theme/features.h"
#include "brave/browser/ui/darker_theme/pref_names.h"
#endif  // defined(TOOLKIT_VIEWS)

BraveThemeService::BraveThemeService(Profile* profile,
                                     const ThemeHelper& theme_helper)
    : ThemeService(profile, theme_helper) {
  brave_theme_event_router_ =
      std::make_unique<extensions::BraveThemeEventRouter>(profile);

#if defined(TOOLKIT_VIEWS)
  if (base::FeatureList::IsEnabled(darker_theme::features::kBraveDarkerTheme)) {
    darker_theme_enabled_.Init(
        darker_theme::prefs::kBraveDarkerMode, profile->GetPrefs(),
        base::BindRepeating(&BraveThemeService::OnDarkerThemePrefChanged,
                            base::Unretained(this)));
  }
#endif  // defined(TOOLKIT_VIEWS)
}

BraveThemeService::~BraveThemeService() = default;

// We replace the baseline theme with the grayscale theme - the default theme is
// blue ish while ours is gray.
bool BraveThemeService::GetIsGrayscale() const {
  return ThemeService::GetIsGrayscale() || GetIsBaseline();
}

void BraveThemeService::SetBraveThemeEventRouterForTesting(
    extensions::BraveThemeEventRouter* mock_router) {
  brave_theme_event_router_.reset(mock_router);
}

#if defined(TOOLKIT_VIEWS)
void BraveThemeService::OnDarkerThemePrefChanged() {
  NotifyThemeChanged();
}
#endif  // defined(TOOLKIT_VIEWS)
