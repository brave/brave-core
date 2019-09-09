/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/balance_report.h"

namespace brave_rewards {

BalanceReport::BalanceReport() :
    deposits("0"),
    grants("0"),
    earning_from_ads("0"),
    auto_contribute("0"),
    recurring_donation("0"),
    one_time_donation("0"),
    total("0") {
}

BalanceReport::~BalanceReport() { }

BalanceReport::BalanceReport(
    const BalanceReport &properties) {
  grants = properties.grants;
  deposits = properties.deposits;
  earning_from_ads = properties.earning_from_ads;
  auto_contribute = properties.auto_contribute;
  recurring_donation = properties.recurring_donation;
  one_time_donation = properties.one_time_donation;
  total = properties.total;
}

}  // namespace brave_rewards
