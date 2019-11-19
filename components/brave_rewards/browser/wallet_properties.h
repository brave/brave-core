/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_WALLET_PROPERTIES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_WALLET_PROPERTIES_H_

#include <string>
#include <map>
#include <vector>

namespace brave_rewards {

struct WalletProperties {
  WalletProperties();
  ~WalletProperties();
  WalletProperties(const WalletProperties& properties);

  double monthly_amount;
  std::vector<double> parameters_choices;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_WALLET_PROPERTIES_H_
