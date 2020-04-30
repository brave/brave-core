/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/balance_report.h"

namespace brave_rewards {

BalanceReport::BalanceReport() {
}

BalanceReport::~BalanceReport() = default;

BalanceReport::BalanceReport(
    const BalanceReport &properties) {
  id = properties.id;
  grants = properties.grants;
  earning_from_ads = properties.earning_from_ads;
  auto_contribute = properties.auto_contribute;
  recurring_donation = properties.recurring_donation;
  one_time_donation = properties.one_time_donation;
}

}  // namespace brave_rewards
