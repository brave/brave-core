/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_BALANCE_REPORT_INFO_HANDLER_
#define BAT_LEDGER_BALANCE_REPORT_INFO_HANDLER_

#include <string>

#include "bat/ledger/export.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

LEDGER_EXPORT enum ReportType {
  GRANT = 0,
  AUTO_CONTRIBUTION = 1,
  DEPOSIT = 2,
  ADS = 3,
  TIP_RECURRING = 4,
  TIP = 5
};

using BalanceReportInfo = ledger::mojom::BalanceReportInfo;
using BalanceReportInfoPtr = ledger::mojom::BalanceReportInfoPtr;

}  // namespace ledger

#endif  // BAT_LEDGER_BALANCE_REPORT_INFO_HANDLER_
