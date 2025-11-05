/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"

#include <optional>

#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/common/extensions/api/brave_theme.h"

namespace extensions::api {

ExtensionFunction::ResponseAction BraveThemeSetBraveThemeTypeFunction::Run() {
  std::optional<brave_theme::SetBraveThemeType::Params> params =
      brave_theme::SetBraveThemeType::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  dark_mode::SetBraveDarkModeType(params->type);

  return RespondNow(NoArguments());
}

}  // namespace extensions::api
