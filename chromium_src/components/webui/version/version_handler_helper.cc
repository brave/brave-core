/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define GetVariationsList GetVariationsList_ChromiumImpl
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"

#include <components/webui/version/version_handler_helper.cc>
#undef GetVariationsList

namespace version_ui {

// Brave always shows full variations names instead of hashes.
base::Value::List GetVariationsList() {
  std::vector<std::string> variations;
  base::FieldTrial::ActiveGroups active_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_groups);

  const unsigned char kNonBreakingHyphenUTF8[] = {0xE2, 0x80, 0x91};
  for (const auto& group : active_groups) {
    std::string line = group.trial_name + ":" + group.group_name;
    base::ReplaceChars(line, "-", base::as_string_view(kNonBreakingHyphenUTF8),
                       &line);
    variations.push_back(line);
  }

  base::Value::List variations_list;
  const std::string& seed_version = variations::GetSeedVersion();
  if (!seed_version.empty() && seed_version != "1") {
    variations_list.Append(seed_version);
  }
  for (std::string& variation : variations) {
    variations_list.Append(std::move(variation));
  }

  return variations_list;
}

}  // namespace version_ui
