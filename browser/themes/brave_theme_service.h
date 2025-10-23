/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_

#include "chrome/browser/themes/theme_service.h"
#include "components/prefs/pref_member.h"

class Profile;

class BraveThemeService : public ThemeService {
 public:
  explicit BraveThemeService(Profile* profile, const ThemeHelper& theme_helper);
  ~BraveThemeService() override;

  // ThemeService:
  bool GetIsGrayscale() const override;

 private:
#if defined(TOOLKIT_VIEWS)
  // Called when the darker theme pref is changed to notify that theme has
  // changed.
  void OnDarkerThemePrefChanged();
#endif  // defined(TOOLKIT_VIEWS)

  void MigrateBrowserColorSchemeFromBraveDarkModePrefs(Profile* profile);

  BooleanPrefMember darker_theme_enabled_;
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
