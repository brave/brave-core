/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/balance.h"

namespace brave_rewards {

Balance::Balance() : total(0.0) {}

Balance::~Balance() {}

Balance::Balance(const Balance &properties) {
  alt_currency = properties.alt_currency;
  probi = properties.probi;
  total = properties.total;
  rates = properties.rates;
  wallets = properties.wallets;
}

}  // namespace brave_rewards
