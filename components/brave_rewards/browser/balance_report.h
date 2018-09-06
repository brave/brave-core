/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_BALANCE_REPORT_
#define BRAVE_BROWSER_PAYMENTS_BALANCE_REPORT_

#include <string>

namespace brave_rewards {
  struct BalanceReport {
    BalanceReport();
    ~BalanceReport();
    BalanceReport(const BalanceReport& properties);

    uint64_t opening_balance = 0;
    uint64_t closing_balance = 0;
    uint64_t deposits = 0;
    uint64_t grants = 0;
    uint64_t earning_from_ads = 0;
    uint64_t auto_contribute = 0;
    uint64_t recurring_donation = 0;
    uint64_t one_time_donation = 0;
    uint64_t total = 0;
  };

}  // namespace brave_rewards

#endif //BRAVE_BROWSER_PAYMENTS_BALANCE_REPORT_
