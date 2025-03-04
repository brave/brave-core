/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_SERIALIZER_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

class CardanoSerializer {
 public:
  static std::vector<uint8_t> SerializeTransaction(
      const CardanoTransaction& tx);

  static uint32_t CalcTransactionSize(const CardanoTransaction& tx);

  static std::array<uint8_t, kCardanoTxHashSize> GetTxHash(
      const CardanoTransaction& tx);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_SERIALIZER_H_
