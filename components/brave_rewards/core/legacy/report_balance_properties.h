/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_REPORT_BALANCE_PROPERTIES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_REPORT_BALANCE_PROPERTIES_H_

#include <string>

#include "base/values.h"

namespace brave_rewards::internal {

struct ReportBalanceProperties {
  ReportBalanceProperties();
  ReportBalanceProperties(const ReportBalanceProperties& properties);
  ~ReportBalanceProperties();

  bool operator==(const ReportBalanceProperties& rhs) const;

  bool operator!=(const ReportBalanceProperties& rhs) const;

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

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_REPORT_BALANCE_PROPERTIES_H_
