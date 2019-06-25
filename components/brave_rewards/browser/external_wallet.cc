/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/external_wallet.h"

namespace brave_rewards {

  ExternalWallet::ExternalWallet() :
      status(0) {
  }

  ExternalWallet::~ExternalWallet() { }

  ExternalWallet::ExternalWallet(const ExternalWallet &properties) {
    token = properties.token;
    address = properties.address;
    status = properties.status;
    type = properties.type;
    verify_url = properties.verify_url;
    add_url = properties.add_url;
    withdraw_url = properties.withdraw_url;
  }

}  // namespace brave_rewards
