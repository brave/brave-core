/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_REPORT_BALANCE_PROPERTIES_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_REPORT_BALANCE_PROPERTIES_H_

#include <string>

#include "base/values.h"

namespace ledger {

struct ReportBalanceProperties {
  ReportBalanceProperties();
  ReportBalanceProperties(
      const ReportBalanceProperties& properties);
  ~ReportBalanceProperties();

  bool operator==(
      const ReportBalanceProperties& rhs) const;

  bool operator!=(
      const ReportBalanceProperties& rhs) const;

  base::Value::Dict ToValue() const;
  bool FromValue(const base::Value::Dict& value);

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  double grants;
  double ad_earnings;
  double auto_contributions;
  double recurring_donations;
  double one_time_donations;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_REPORT_BALANCE_PROPERTIES_H_
