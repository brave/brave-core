// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/urls.h"

#include <string>

#include "base/command_line.h"
#include "brave/components/brave_today/common/switches.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_today {

std::string GetHostname() {
  std::string from_switch =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kBraveTodayHost);
  if (from_switch.empty()) {
    return "brave-today-cdn.brave.com";
  } else {
    return from_switch;
  }
}

std::string GetRegionUrlPart() {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string language_code = brave_l10n::GetLanguageCode(locale);
  // TODO(petemill): Have a remotely-updatable list of supported language
  // variations.
  if (language_code == "ja") {
    return "ja";
  }
  return "";
}

}  // namespace brave_today
