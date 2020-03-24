/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EXTERNAL_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EXTERNAL_WALLET_H_

#include <string>

namespace brave_rewards {

struct ExternalWallet {
  ExternalWallet();
  ~ExternalWallet();
  ExternalWallet(const ExternalWallet& properties);
  std::string toJson();

  std::string token;
  std::string address;
  uint32_t status;
  std::string type;
  std::string verify_url;
  std::string add_url;
  std::string withdraw_url;
  std::string user_name;
  std::string account_url;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EXTERNAL_WALLET_H_
