/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/extensions/api/brave_theme.h"
#include "chrome/browser/profiles/profile.h"

using BTS = BraveThemeService;

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction BraveThemeGetBraveThemeListFunction::Run() {
  std::string json_string;
  base::JSONWriter::Write(BTS::GetBraveThemeList(), &json_string);
  return RespondNow(OneArgument(std::make_unique<base::Value>(json_string)));
}

ExtensionFunction::ResponseAction BraveThemeGetBraveThemeTypeFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  const std::string theme_type = BTS::GetStringFromBraveThemeType(
      BTS::GetActiveBraveThemeType(profile));
  return RespondNow(OneArgument(std::make_unique<base::Value>(theme_type)));
}

ExtensionFunction::ResponseAction BraveThemeSetBraveThemeTypeFunction::Run() {
  std::unique_ptr<brave_theme::SetBraveThemeType::Params> params(
      brave_theme::SetBraveThemeType::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  BTS::SetBraveThemeType(profile, params->type);

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
