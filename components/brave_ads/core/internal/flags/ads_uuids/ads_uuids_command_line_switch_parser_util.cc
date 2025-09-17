/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/ads_uuids/ads_uuids_command_line_switch_parser_util.h"

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/uuid.h"

namespace brave_ads {

base::flat_map<std::string, bool> ParseAdsUuidsCommandLineSwitch() {
  const auto* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line) {
    return {};
  }

  const std::string switch_value = command_line->GetSwitchValueASCII("ads");
  if (switch_value.empty()) {
    return {};
  }

  const std::vector<std::string> components = base::SplitString(
      switch_value, "=", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (components.size() != 2 || base::ToLowerASCII(components[0]) != "uuids") {
    return {};
  }

  base::flat_map<std::string, bool> normalized_uuids;

  const std::vector<std::string> list = base::SplitString(
      components[1], ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (const auto& uuid_as_string : list) {
    const base::Uuid uuid = base::Uuid::ParseCaseInsensitive(uuid_as_string);
    if (uuid.is_valid()) {
      normalized_uuids[uuid.AsLowercaseString()] = true;
    }
  }
  return normalized_uuids;
}

}  // namespace brave_ads
