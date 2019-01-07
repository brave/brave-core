/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PENDING_CONTRIBUTION_
#define BRAVE_BROWSER_PAYMENTS_PENDING_CONTRIBUTION_

#include <string>

namespace brave_rewards {
  struct PendingContribution {
    PendingContribution();
    ~PendingContribution();
    PendingContribution(const PendingContribution& data);

    std::string publisher_key;
    double amount = 0;
    uint32_t added_date = 0;
    uint32_t reconcile_date = 0;
  };

}  // namespace brave_rewards

#endif //BRAVE_BROWSER_PAYMENTS_PENDING_CONTRIBUTION_
