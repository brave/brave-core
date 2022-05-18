/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/studies/studies_util.h"

#include <string>

#include "bat/ads/internal/base/logging_util.h"

namespace ads {

namespace {
constexpr char kAdsTrialTag[] = "BraveAds";
}  // namespace

base::FieldTrial::ActiveGroups GetActiveStudies() {
  base::FieldTrial::ActiveGroups studies;
  base::FieldTrialList::GetActiveFieldTrialGroups(&studies);

  base::FieldTrial::ActiveGroups filtered_studies;

  for (const auto& group : studies) {
    if (group.trial_name.find(kAdsTrialTag) == std::string::npos) {
      continue;
    }

    filtered_studies.push_back(group);
  }

  return filtered_studies;
}

void LogActiveStudies() {
  const base::FieldTrial::ActiveGroups active_studies = GetActiveStudies();
  if (active_studies.empty()) {
    BLOG(1, "No active studies");
    return;
  }

  for (const auto& study : active_studies) {
    BLOG(1, "Study " << study.trial_name << " is active (" << study.group_name
                     << ")");
  }
}

}  // namespace ads
