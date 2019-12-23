/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_BALANCE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_BALANCE_H_

#include <map>
#include <string>

namespace brave_rewards {

struct Balance {
  Balance();
  ~Balance();
  Balance(const Balance& properties);

  double total;
  std::map<std::string, double> rates;
  std::map<std::string, double> wallets;
  std::string user_funds;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_BALANCE_H_
