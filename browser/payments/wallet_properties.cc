/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/payments/wallet_properties.h"

namespace payments {

  WalletProperties::WalletProperties() :
      balance(0.0),
      parameters_days(0) {
  }

  WalletProperties::~WalletProperties() { }

  WalletProperties::WalletProperties(const WalletProperties &properties) {
    probi = properties.probi;
    balance = properties.balance;
    rates = properties.rates;
    parameters_choices = properties.parameters_choices;
    parameters_range = properties.parameters_range;
    parameters_days = properties.parameters_days;
    grants = properties.grants;
  }

}  // namespace payments
