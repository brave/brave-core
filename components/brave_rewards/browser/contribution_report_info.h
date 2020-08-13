/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_REPORT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_REPORT_INFO_H_

#include <stdint.h>
#include <vector>

#include "brave/components/brave_rewards/browser/publisher_info.h"

namespace brave_rewards {

struct ContributionReportInfo {
  ContributionReportInfo();
  ~ContributionReportInfo();
  ContributionReportInfo(const ContributionReportInfo& properties);

  double amount;
  uint32_t type;
  uint32_t processor;
  std::vector<PublisherInfo> publishers;
  uint64_t created_at;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_REPORT_INFO_H_
