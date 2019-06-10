/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/devtools/brave_devtools_ui_bindings.h"

#include "base/values.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace {
std::string GetDevToolsUIThemeValue(Profile* profile) {
  BraveThemeType theme_type =
      BraveThemeService::GetActiveBraveThemeType(profile);
  // In devtools' theme, default is translated to light.
  return theme_type == BRAVE_THEME_TYPE_DARK ? "\"dark\"" : "\"default\"";
}
}  // namespace

void BraveDevToolsUIBindings::GetPreferences(const DispatchCallback& callback) {
  const base::DictionaryValue* prefs =
      profile_->GetPrefs()->GetDictionary(prefs::kDevToolsPreferences);

  if (prefs->FindKey("uiTheme"))
    return DevToolsUIBindings::GetPreferences(callback);

  base::Value new_prefs(prefs->Clone());
  new_prefs.SetKey("uiTheme", base::Value(GetDevToolsUIThemeValue(profile())));
  callback.Run(&new_prefs);
}
