/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_SERIALIZER_H_

#include <vector>

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

namespace brave_wallet {

// Utility class for serializing Cardano transactions and related functionality.
class CardanoSerializer {
 public:
  // Serializes a Cardano transaction into a byte vector (CBOR format).
  static std::vector<uint8_t> SerializeTransaction(
      const CardanoTransaction& tx);

  // Calculates the size (in bytes) of the serialized transaction.
  static uint32_t CalcTransactionSize(const CardanoTransaction& tx);

  // Computes the transaction hash (Blake2b-256 hash of the serialized
  // transaction body)
  static std::array<uint8_t, kCardanoTxHashSize> GetTxHash(
      const CardanoTransaction& tx);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_SERIALIZER_H_
