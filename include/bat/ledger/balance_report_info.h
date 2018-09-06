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

  uint64_t opening_balance_ = 0;
  uint64_t closing_balance_ = 0;
  uint64_t deposits_ = 0;
  uint64_t grants_ = 0;
  uint64_t earning_from_ads_ = 0;
  uint64_t auto_contribute_ = 0;
  uint64_t recurring_donation_ = 0;
  uint64_t one_time_donation_ = 0;
  uint64_t total_ = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_BALANCE_REPORT_INFO_HANDLER_
