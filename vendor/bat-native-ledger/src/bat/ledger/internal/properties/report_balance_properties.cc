/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/report_balance_properties.h"

namespace ledger {

ReportBalanceProperties::ReportBalanceProperties()
    : opening_balance("0"),
      closing_balance("0"),
      deposits("0"),
      grants("0"),
      ad_earnings("0"),
      auto_contributions("0"),
      recurring_donations("0"),
      one_time_donations("0"),
      total("0") {}

ReportBalanceProperties::ReportBalanceProperties(
    const ReportBalanceProperties& properties) {
  opening_balance = properties.opening_balance;
  closing_balance = properties.closing_balance;
  deposits = properties.deposits;
  grants = properties.grants;
  ad_earnings = properties.ad_earnings;
  auto_contributions = properties.auto_contributions;
  recurring_donations = properties.recurring_donations;
  one_time_donations = properties.one_time_donations;
  total = properties.total;
}

ReportBalanceProperties::~ReportBalanceProperties() = default;

bool ReportBalanceProperties::operator==(
    const ReportBalanceProperties& rhs) const {
  return opening_balance == rhs.opening_balance &&
      closing_balance == rhs.closing_balance &&
      deposits == rhs.deposits &&
      grants == rhs.grants &&
      ad_earnings == rhs.ad_earnings &&
      auto_contributions == rhs.auto_contributions &&
      recurring_donations == rhs.recurring_donations &&
      one_time_donations == rhs.one_time_donations &&
      total == rhs.total;
}

bool ReportBalanceProperties::operator!=(
    const ReportBalanceProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
