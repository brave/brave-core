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

    std::string opening_balance = "0";
    std::string closing_balance = "0";
    std::string deposits = "0";
    std::string grants = "0";
    std::string earning_from_ads = "0";
    std::string auto_contribute = "0";
    std::string recurring_donation = "0";
    std::string one_time_donation = "0";
    std::string total = "0";
  };

}  // namespace brave_rewards

#endif //BRAVE_BROWSER_PAYMENTS_BALANCE_REPORT_
