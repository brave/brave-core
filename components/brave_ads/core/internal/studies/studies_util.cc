/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/studies/studies_util.h"

#include <iterator>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads {

namespace {

constexpr char kActiveFieldTrialStudyPrefix[] = "BraveAds.";

base::FieldTrial::ActiveGroups GetActiveFieldTrialStudyGroups() {
  base::FieldTrial::ActiveGroups active_field_trial_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_field_trial_groups);

  base::FieldTrial::ActiveGroups filtered_active_field_trial_groups;

  base::ranges::copy_if(
      active_field_trial_groups,
      std::back_inserter(filtered_active_field_trial_groups),
      [](const base::FieldTrial::ActiveGroup& active_field_trial_group) {
        return active_field_trial_group.trial_name.starts_with(
            kActiveFieldTrialStudyPrefix);
      });

  return filtered_active_field_trial_groups;
}

}  // namespace

std::optional<base::FieldTrial::ActiveGroup> GetActiveFieldTrialStudyGroup() {
  const base::FieldTrial::ActiveGroups active_field_trial_groups =
      GetActiveFieldTrialStudyGroups();

  if (active_field_trial_groups.size() == 1) {
    // Only one `BraveAds.` study is allowed to be active at any given time.
    return active_field_trial_groups.front();
  }

  return std::nullopt;
}

void LogActiveFieldTrialStudyGroups() {
  const base::FieldTrial::ActiveGroups active_field_trial_groups =
      GetActiveFieldTrialStudyGroups();
  if (active_field_trial_groups.empty()) {
    return BLOG(1, "No active studies");
  }

  if (active_field_trial_groups.size() == 1) {
    // Only one `BraveAds.` study is allowed to be active at any given time.
    const base::FieldTrial::ActiveGroup& active_field_trial_group =
        active_field_trial_groups.front();
    BLOG(1, "Study " << active_field_trial_group.trial_name << " is active ("
                     << active_field_trial_group.group_name << ")");
  } else {
    for (const auto& active_field_trial_group : active_field_trial_groups) {
      BLOG(1, "Skipping study " << active_field_trial_group.trial_name << " ("
                                << active_field_trial_group.group_name << ")");
    }
  }
}

}  // namespace brave_ads
