/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/values.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/browser/external_wallet.h"

namespace brave_rewards {

  ExternalWallet::ExternalWallet() :
      status(0) {
  }

  ExternalWallet::~ExternalWallet() { }

  ExternalWallet::ExternalWallet(const ExternalWallet& properties) =  default;

  std::string ExternalWallet::toJson() {
    std::string json_wallet;
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey("token", token);
    dict.SetStringKey("address", address);

    // enum class WalletStatus : int32_t
    dict.SetIntKey("status", static_cast<int32_t>(status));
    dict.SetStringKey("type", type);
    dict.SetStringKey("verify_url", verify_url);
    dict.SetStringKey("add_url", add_url);
    dict.SetStringKey("withdraw_url", withdraw_url);
    dict.SetStringKey("user_name", user_name);
    dict.SetStringKey("account_url", account_url);
    dict.SetStringKey("login_url", login_url);
    base::JSONWriter::Write(dict, &json_wallet);
    return json_wallet;
  }
}  // namespace brave_rewards
