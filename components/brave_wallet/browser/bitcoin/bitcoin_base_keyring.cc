/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_base_keyring.h"

#include "base/check.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

BitcoinBaseKeyring::BitcoinBaseKeyring(mojom::KeyringId keyring_id)
    : keyring_id_(keyring_id) {
  CHECK(IsBitcoinKeyring(keyring_id_));
}

BitcoinBaseKeyring::~BitcoinBaseKeyring() = default;

bool BitcoinBaseKeyring::IsTestnet() const {
  return IsBitcoinTestnetKeyring(keyring_id_);
}

}  // namespace brave_wallet
