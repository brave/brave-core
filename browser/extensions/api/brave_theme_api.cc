/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"

#include <optional>

#include "base/notreached.h"
#include "brave/common/extensions/api/brave_theme.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"

namespace {

ThemeService::BrowserColorScheme ConvertToBrowserColorScheme(
    const std::string scheme) {
  if (scheme == "System") {
    return ThemeService::BrowserColorScheme::kSystem;
  } else if (scheme == "Light") {
    return ThemeService::BrowserColorScheme::kLight;
  } else if (scheme == "Dark") {
    return ThemeService::BrowserColorScheme::kDark;
  }

  NOTREACHED();
}

}  // namespace

namespace extensions::api {

// TODO(): Remove this. Only Welcome UI uses this api.
// Welcome UI can use CustomizeColorSchemeModeHandler instead.
ExtensionFunction::ResponseAction BraveThemeSetBraveThemeTypeFunction::Run() {
  std::optional<brave_theme::SetBraveThemeType::Params> params =
      brave_theme::SetBraveThemeType::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* theme_service = ThemeServiceFactory::GetForProfile(
      Profile::FromBrowserContext(browser_context()));
  CHECK(theme_service);
  theme_service->SetBrowserColorScheme(
      ConvertToBrowserColorScheme(params->type));

  return RespondNow(NoArguments());
}

}  // namespace extensions::api
