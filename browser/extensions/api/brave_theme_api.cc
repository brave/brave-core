/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"

#include <memory>
#include <optional>
#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/common/extensions/api/brave_theme.h"

namespace extensions::api {

ExtensionFunction::ResponseAction BraveThemeGetBraveThemeTypeFunction::Run() {
  // Fetch current type from theme service.
  const std::string theme_type = "Light";
  return RespondNow(WithArguments(theme_type));
}

ExtensionFunction::ResponseAction BraveThemeSetBraveThemeTypeFunction::Run() {
  std::optional<brave_theme::SetBraveThemeType::Params> params =
      brave_theme::SetBraveThemeType::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  // Set to theme service.

  return RespondNow(NoArguments());
}

}  // namespace extensions::api
