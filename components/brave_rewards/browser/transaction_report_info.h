/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TRANSACTION_REPORT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TRANSACTION_REPORT_INFO_H_

#include <stdint.h>

namespace brave_rewards {

struct TransactionReportInfo {
  TransactionReportInfo();
  ~TransactionReportInfo();
  TransactionReportInfo(const TransactionReportInfo& properties);

  double amount;
  uint32_t type;
  uint32_t processor;
  uint64_t created_at;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TRANSACTION_REPORT_INFO_H_
