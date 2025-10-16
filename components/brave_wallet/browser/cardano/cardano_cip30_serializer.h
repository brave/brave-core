/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CIP30_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CIP30_SERIALIZER_H_

#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "components/cbor/values.h"

namespace brave_wallet {

// Utility class for serializing data for CIP-30/CIP-8 signing.
// https://github.com/cardano-foundation/CIPs/tree/master/CIP-0030#apisigndataaddr-address-payload-bytes-promisedatasignature
class CardanoCip30Serializer {
 public:
  CardanoCip30Serializer();
  ~CardanoCip30Serializer();

  // Returns CBOR-serialized payload for further Ed25519 signing.
  static std::vector<uint8_t> SerializedSignPayload(
      const CardanoAddress& payment_address,
      base::span<const uint8_t> message);

  // Returns CBOR-serialized COSE_Key to be a part of `DataSignature` as a
  // `key` field.
  static std::vector<uint8_t> SerializeSignedDataKey(
      const CardanoAddress& payment_address,
      base::span<const uint8_t> pubkey);

  // Returns CBOR-serialized COSE_Sign1 to be a part of `DataSignature` as a
  // `signature ` field.
  static std::vector<uint8_t> SerializeSignedDataSignature(
      const CardanoAddress& payment_address,
      base::span<const uint8_t> message,
      base::span<const uint8_t> signature);

  static std::optional<std::string> SerializeAmount(uint64_t amount);

  static std::optional<uint64_t> DeserializeAmount(
      const std::string& amount_cbor);

  static std::optional<std::vector<std::string>> SerializeUtxos(
      base::span<const cardano_rpc::UnspentOutput> utxos);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CIP30_SERIALIZER_H_
