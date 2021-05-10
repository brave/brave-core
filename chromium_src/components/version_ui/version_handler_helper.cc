/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define GetVariationsList GetVariationsList_ChromiumImpl
#include "../../../../components/version_ui/version_handler_helper.cc"
#undef GetVariationsList

namespace version_ui {

// Brave always shows full variations names instead of hashes.
std::unique_ptr<base::Value> GetVariationsList() {
  std::vector<std::string> variations;
  base::FieldTrial::ActiveGroups active_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_groups);

  const unsigned char kNonBreakingHyphenUTF8[] = {0xE2, 0x80, 0x91, '\0'};
  const std::string kNonBreakingHyphenUTF8String(
      reinterpret_cast<const char*>(kNonBreakingHyphenUTF8));
  for (const auto& group : active_groups) {
    std::string line = group.trial_name + ":" + group.group_name;
    base::ReplaceChars(line, "-", kNonBreakingHyphenUTF8String, &line);
    variations.push_back(line);
  }

  std::unique_ptr<base::ListValue> variations_list(new base::ListValue);
  for (std::vector<std::string>::const_iterator it = variations.begin();
       it != variations.end(); ++it) {
    variations_list->AppendString(*it);
  }

  return std::move(variations_list);
}

}  // namespace version_ui
