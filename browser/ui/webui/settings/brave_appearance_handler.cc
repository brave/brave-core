/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"

namespace {
void SetBraveThemeTypePref(Profile* profile,
                           BraveThemeService::BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}

BraveThemeService::BraveThemeType GetBraveThemeTypeFromString(
    base::StringPiece theme) {
  if (theme == "Default")
    return BraveThemeService::BRAVE_THEME_TYPE_DEFAULT;

  if (theme == "Light")
    return BraveThemeService::BRAVE_THEME_TYPE_LIGHT;

  if (theme == "Dark")
    return BraveThemeService::BRAVE_THEME_TYPE_DARK;

  NOTREACHED();
  return BraveThemeService::BRAVE_THEME_TYPE_DEFAULT;
}

std::string GetStringFromBraveThemeType(
    BraveThemeService::BraveThemeType theme) {
  switch (theme) {
    case BraveThemeService::BRAVE_THEME_TYPE_DEFAULT:
      return "Default";
    case BraveThemeService::BRAVE_THEME_TYPE_LIGHT:
      return "Light";
    case BraveThemeService::BRAVE_THEME_TYPE_DARK:
      return "Dark";
    default:
      NOTREACHED();
  }
}
}  // namespace

void BraveAppearanceHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());

  web_ui()->RegisterMessageCallback(
      "getBraveThemeType",
      base::BindRepeating(&BraveAppearanceHandler::GetBraveThemeType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setBraveThemeType",
      base::BindRepeating(&BraveAppearanceHandler::SetBraveThemeType,
                          base::Unretained(this)));
}

void BraveAppearanceHandler::SetBraveThemeType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  std::string theme;
  args->GetString(0, &theme);
  SetBraveThemeTypePref(profile_, GetBraveThemeTypeFromString(theme));
}

void BraveAppearanceHandler::GetBraveThemeType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0].Clone(),
      base::Value(GetStringFromBraveThemeType(
          BraveThemeService::GetBraveThemeType(profile_))));
}
