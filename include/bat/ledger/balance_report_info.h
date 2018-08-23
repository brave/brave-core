/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_BALANCE_REPORT_INFO_HANDLER_
#define BAT_LEDGER_BALANCE_REPORT_INFO_HANDLER_

#include <string>

#include "bat/ledger/export.h"

namespace ledger {

LEDGER_EXPORT struct BalanceReportInfo {
  BalanceReportInfo();

  double opening_balance_ = .0;
  double closing_balance_ = .0;
  double grants_ = .0;
  double earning_from_ads_ = .0;
  double auto_contribute_ = .0;
  double recurring_donation_ = .0;
  double one_time_donation_ = .0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_BALANCE_REPORT_INFO_HANDLER_
