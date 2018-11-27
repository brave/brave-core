/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_CONTRIBUTION_INFO_
#define BRAVE_BROWSER_PAYMENTS_CONTRIBUTION_INFO_

#include <string>

namespace brave_rewards {
  struct ContributionInfo {
    ContributionInfo();
    ~ContributionInfo();
    ContributionInfo(const ContributionInfo& properties);

    std::string probi;
    int month;
    int year;
    int category;
    uint32_t date = 0;
    std::string publisher_key;
  };

}  // namespace brave_rewards

#endif //BRAVE_BROWSER_PAYMENTS_CONTRIBUTION_INFO_
