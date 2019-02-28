/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/extensions/api/brave_theme.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction BraveThemeGetBraveThemeListFunction::Run() {
  std::string json_string;
  base::JSONWriter::Write(BraveThemeService::GetBraveThemeList(),
                          &json_string);
  return RespondNow(OneArgument(std::make_unique<base::Value>(json_string)));
}

}  // namespace api
}  // namespace extensions
