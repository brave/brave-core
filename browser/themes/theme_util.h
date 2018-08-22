/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_THEME_UTIL_H_
#define BRAVE_BROWSER_THEMES_THEME_UTIL_H_

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}

enum BraveThemeType {
  BRAVE_THEME_TYPE_DEFAULT,  // Choose theme by channel
  BRAVE_THEME_TYPE_DARK,     // Use dark theme regardless of channel
  BRAVE_THEME_TYPE_LIGHT,    // Use light theme regardless of channel
};

int GetBraveThemeType(Profile* profile);

void SetBraveThemeType(Profile* profile, BraveThemeType type);

void RegisterProfilePrefsForBraveThemeType(user_prefs::PrefRegistrySyncable* registry);

#endif  // BRAVE_BROWSER_THEMES_THEME_UTIL_H_
