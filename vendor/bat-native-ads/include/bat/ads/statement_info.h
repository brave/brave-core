/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_STATEMENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_STATEMENT_INFO_H_

#include <string>

#include "base/time/time.h"
#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT StatementInfo final {
  StatementInfo();
  StatementInfo(const StatementInfo& info);
  StatementInfo& operator=(const StatementInfo& info);
  ~StatementInfo();

  bool operator==(const StatementInfo& rhs) const;
  bool operator!=(const StatementInfo& rhs) const;

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  base::Time next_payment_date;
  double earnings_this_month = 0.0;
  double earnings_last_month = 0.0;
  int ads_received_this_month = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_STATEMENT_INFO_H_
