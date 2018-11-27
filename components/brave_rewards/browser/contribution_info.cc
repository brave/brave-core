/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/browser/contribution_info.h"

namespace brave_rewards {

  ContributionInfo::ContributionInfo() :
      date(0) {
  }

  ContributionInfo::~ContributionInfo() { }

  ContributionInfo::ContributionInfo(const ContributionInfo &data) {
    probi = data.probi;
    month = data.month;
    year = data.year;
    category = data.category;
    date = data.date;
    publisher_key = data.publisher_key;
  }

}  // namespace brave_rewards
