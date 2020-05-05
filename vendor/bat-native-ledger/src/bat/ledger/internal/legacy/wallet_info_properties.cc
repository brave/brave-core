/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/wallet_info_properties.h"

namespace ledger {

WalletInfoProperties::WalletInfoProperties() = default;

WalletInfoProperties::WalletInfoProperties(
    const WalletInfoProperties& properties) {
  payment_id = properties.payment_id;
  address_card_id = properties.address_card_id;
  key_info_seed = properties.key_info_seed;
}

WalletInfoProperties::~WalletInfoProperties() = default;

bool WalletInfoProperties::operator==(
    const WalletInfoProperties& rhs) const {
  return payment_id == rhs.payment_id &&
      address_card_id == rhs.address_card_id &&
      key_info_seed == rhs.key_info_seed;
}

bool WalletInfoProperties::operator!=(
    const WalletInfoProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
