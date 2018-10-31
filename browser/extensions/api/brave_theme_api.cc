/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"

#include <string>

#include "base/values.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/extensions/api/brave_theme.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace {
void SetBraveThemeTypePref(Profile* profile,
                           BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}

BraveThemeType GetBraveThemeTypeFromString(
    base::StringPiece theme) {
  if (theme == "Default")
    return BraveThemeType::BRAVE_THEME_TYPE_DEFAULT;

  if (theme == "Light")
    return BraveThemeType::BRAVE_THEME_TYPE_LIGHT;

  if (theme == "Dark")
    return BraveThemeType::BRAVE_THEME_TYPE_DARK;

  NOTREACHED();
  return BraveThemeType::BRAVE_THEME_TYPE_DEFAULT;
}

std::string GetStringFromBraveThemeType(
    BraveThemeType theme) {
  switch (theme) {
    case BraveThemeType::BRAVE_THEME_TYPE_DEFAULT:
      return "Default";
    case BraveThemeType::BRAVE_THEME_TYPE_LIGHT:
      return "Light";
    case BraveThemeType::BRAVE_THEME_TYPE_DARK:
      return "Dark";
    default:
      NOTREACHED();
  }
}
}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction BraveThemeSetBraveThemeTypeFunction::Run() {
  std::unique_ptr<brave_theme::SetBraveThemeType::Params> params(
      brave_theme::SetBraveThemeType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  SetBraveThemeTypePref(profile, GetBraveThemeTypeFromString(params->type));

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveThemeGetBraveThemeTypeFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  const std::string theme_type = GetStringFromBraveThemeType(
      BraveThemeService::GetUserPreferredBraveThemeType(profile));
  return RespondNow(OneArgument(std::make_unique<base::Value>(theme_type)));
}

}  // namespace api
}  // namespace extensions
