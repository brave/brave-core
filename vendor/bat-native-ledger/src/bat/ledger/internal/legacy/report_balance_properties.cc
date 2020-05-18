/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/report_balance_properties.h"

namespace ledger {

ReportBalanceProperties::ReportBalanceProperties()
    : grants(0.0),
      ad_earnings(0.0),
      auto_contributions(0.0),
      recurring_donations(0.0),
      one_time_donations(0.0) {}

ReportBalanceProperties::ReportBalanceProperties(
    const ReportBalanceProperties& properties) {
  grants = properties.grants;
  ad_earnings = properties.ad_earnings;
  auto_contributions = properties.auto_contributions;
  recurring_donations = properties.recurring_donations;
  one_time_donations = properties.one_time_donations;
}

ReportBalanceProperties::~ReportBalanceProperties() = default;

bool ReportBalanceProperties::operator==(
    const ReportBalanceProperties& rhs) const {
  return grants == rhs.grants &&
      ad_earnings == rhs.ad_earnings &&
      auto_contributions == rhs.auto_contributions &&
      recurring_donations == rhs.recurring_donations &&
      one_time_donations == rhs.one_time_donations;
}

bool ReportBalanceProperties::operator!=(
    const ReportBalanceProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
