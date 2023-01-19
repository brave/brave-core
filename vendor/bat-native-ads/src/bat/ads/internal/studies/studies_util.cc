/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/studies/studies_util.h"

#include <string>

#include "bat/ads/internal/common/logging_util.h"

namespace ads {

namespace {
constexpr char kAdsTrialTag[] = "BraveAds";
}  // namespace

base::FieldTrial::ActiveGroups GetActiveStudies() {
  base::FieldTrial::ActiveGroups active_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_groups);

  base::FieldTrial::ActiveGroups filtered_active_groups;

  for (const auto& active_group : active_groups) {
    if (active_group.trial_name.find(kAdsTrialTag) == std::string::npos) {
      continue;
    }

    filtered_active_groups.push_back(active_group);
  }

  return filtered_active_groups;
}

void LogActiveStudies() {
  const base::FieldTrial::ActiveGroups active_groups = GetActiveStudies();
  if (active_groups.empty()) {
    BLOG(1, "No active studies");
    return;
  }

  for (const auto& active_group : active_groups) {
    BLOG(1, "Study " << active_group.trial_name << " is active ("
                     << active_group.group_name << ")");
  }
}

}  // namespace ads
