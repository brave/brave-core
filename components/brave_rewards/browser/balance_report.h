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

    double opening_balance = .0;
    double closing_balance = .0;
    double grants = .0;
    double earning_from_ads = .0;
    double auto_contribute = .0;
    double recurring_donation = .0;
    double one_time_donation = .0;
  };

}  // namespace brave_rewards

#endif //BRAVE_BROWSER_PAYMENTS_BALANCE_REPORT_
