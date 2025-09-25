/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CIP30_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CIP30_SERIALIZER_H_

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
  using InputWitness = std::array<uint8_t, kCardanoWitnessSize>;

  struct RestoredTransactionInput {
    RestoredTransactionInput();
    RestoredTransactionInput(const RestoredTransactionInput&);
    RestoredTransactionInput& operator=(const RestoredTransactionInput&);
    RestoredTransactionInput(RestoredTransactionInput&&);
    RestoredTransactionInput& operator=(RestoredTransactionInput&&);
    ~RestoredTransactionInput();

    std::array<uint8_t, kCardanoTxHashSize> tx_hash = {};
    uint32_t index = 0;
    std::optional<CardanoAddress> address;  // nullopt for unknown input.
    std::optional<uint64_t> amount;         // nullopt for unknown input.
  };

  struct RestoredTransactionOutput {
    RestoredTransactionOutput();
    ~RestoredTransactionOutput();
    CardanoAddress address;
    uint64_t amount = 0;
  };

  struct RestoredTransactionBody {
    RestoredTransactionBody();
    ~RestoredTransactionBody();
    RestoredTransactionBody(const RestoredTransactionBody&);
    RestoredTransactionBody(RestoredTransactionBody&&);
    RestoredTransactionBody& operator=(const RestoredTransactionBody&);
    RestoredTransactionBody& operator=(RestoredTransactionBody&&);

    std::vector<RestoredTransactionInput> inputs;
    std::vector<RestoredTransactionOutput> outputs;
  };

  struct RestoredTransaction {
    RestoredTransaction();
    ~RestoredTransaction();
    RestoredTransaction(const RestoredTransaction&);
    RestoredTransaction& operator=(const RestoredTransaction&);
    RestoredTransaction(RestoredTransaction&&);
    RestoredTransaction& operator=(RestoredTransaction&&);

    RestoredTransactionBody tx_body;
    std::vector<InputWitness> witness_set;

    std::vector<uint8_t> raw_bytes;
  };

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

  static std::string SerializeAmount(uint64_t amount);

  static std::optional<uint64_t> DeserializeAmount(
      const std::string& amount_cbor);

  static std::vector<std::string> SerializeUtxos(
      const std::vector<std::pair<CardanoAddress, cardano_rpc::UnspentOutput>>&
          utxos);

  // Deserializes a Cardano transaction from a byte vector (CBOR format).
  static std::optional<RestoredTransaction> DeserializeTransaction(
      base::span<const uint8_t> bytes);

  static std::optional<std::vector<uint8_t>> ApplySignResults(
      const std::vector<uint8_t>& unsigned_tx_bytes,
      const std::vector<CardanoSignMessageResult>& witness);

 private:
  static std::optional<std::vector<RestoredTransactionInput>> DeserializeInputs(
      const cbor::Value::ArrayValue& input);
  static std::optional<std::vector<RestoredTransactionOutput>>
  DeserializeOutputs(const cbor::Value::ArrayValue& outputs);
  static std::optional<RestoredTransactionBody> DeserializeTxBody(
      const cbor::Value::MapValue& data);
  static std::optional<std::vector<InputWitness>> DeserializeWitnessSet(
      const cbor::Value::ArrayValue& input);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CIP30_SERIALIZER_H_
