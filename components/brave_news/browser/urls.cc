// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/urls.h"

#include <string>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/switches.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_news {
namespace {
// TODO(petemill): Have a remotely-updatable list of supported language
// variations.
const base::flat_set<std::string> kSupportedLocales = {"en_US", "ja_JP",
                                                       "en_ES", "en_MX"};
}  // namespace

std::string GetHostname() {
  std::string from_switch =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kBraveNewsHost);
  if (from_switch.empty()) {
    return "brave-today-cdn.brave.com";
  } else {
    return from_switch;
  }
}

std::string GetV1RegionUrlPart() {
  if (brave_l10n::GetDefaultISOLanguageCodeString() == "ja") {
    return "ja";
  }
  return "";
}

std::string GetRegionUrlPart() {
  if (!base::FeatureList::IsEnabled(
          brave_news::features::kBraveNewsV2Feature)) {
    return GetV1RegionUrlPart();
  }

  return "global.";
}

}  // namespace brave_news
