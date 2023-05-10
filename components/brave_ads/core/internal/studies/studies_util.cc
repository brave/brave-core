/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/studies/studies_util.h"

#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads {

namespace {
constexpr char kStudyPrefixTag[] = "BraveAds.";
}  // namespace

base::FieldTrial::ActiveGroups GetActiveFieldTrialStudyGroups() {
  base::FieldTrial::ActiveGroups active_field_trial_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_field_trial_groups);

  base::FieldTrial::ActiveGroups filtered_active_field_trial_groups;

  base::ranges::copy_if(active_field_trial_groups,
                        std::back_inserter(filtered_active_field_trial_groups),
                        [](const base::FieldTrial::ActiveGroup& active_group) {
                          return base::StartsWith(active_group.trial_name,
                                                  kStudyPrefixTag);
                        });

  return filtered_active_field_trial_groups;
}

void LogActiveStudies() {
  const base::FieldTrial::ActiveGroups active_field_trial_groups =
      GetActiveFieldTrialStudyGroups();
  if (active_field_trial_groups.empty()) {
    BLOG(1, "No active studies");
    return;
  }

  for (const auto& active_field_trial_group : active_field_trial_groups) {
    BLOG(1, "Study " << active_field_trial_group.trial_name << " is active ("
                     << active_field_trial_group.group_name << ")");
  }
}

}  // namespace brave_ads
