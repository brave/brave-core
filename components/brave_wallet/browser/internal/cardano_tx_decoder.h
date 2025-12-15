/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_CARDANO_TX_DECODER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_CARDANO_TX_DECODER_H_

#include <array>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "base/numerics/safe_conversions.h"

namespace brave_wallet {

inline constexpr uint32_t kCardanoTxHashSize = 32u;
inline constexpr uint32_t kCardanoPubKeySize = 32u;
inline constexpr uint32_t kCardanoSignatureSize = 64u;

// Wrapper class for Cardano transaction decoding functionality
// This class provides a C++ interface to the Rust cardano_tx_decoder
// implementation
class CardanoTxDecoder {
 public:
  struct SerializableTxInput {
    SerializableTxInput();
    SerializableTxInput(const SerializableTxInput&);
    SerializableTxInput& operator=(const SerializableTxInput&);
    SerializableTxInput(SerializableTxInput&&);
    SerializableTxInput& operator=(SerializableTxInput&&);
    ~SerializableTxInput();

    std::array<uint8_t, kCardanoTxHashSize> tx_hash = {};
    base::StrictNumeric<uint32_t> index = 0u;
  };

  struct SerializableTxOutput {
    SerializableTxOutput();
    SerializableTxOutput(const SerializableTxOutput&);
    SerializableTxOutput& operator=(const SerializableTxOutput&);
    SerializableTxOutput(SerializableTxOutput&&);
    SerializableTxOutput& operator=(SerializableTxOutput&&);
    ~SerializableTxOutput();

    std::vector<uint8_t> address_bytes;
    base::StrictNumeric<uint64_t> amount = 0u;
  };

  struct SerializableTxBody {
    SerializableTxBody();
    ~SerializableTxBody();
    SerializableTxBody(const SerializableTxBody&);
    SerializableTxBody& operator=(const SerializableTxBody&);
    SerializableTxBody(SerializableTxBody&&);
    SerializableTxBody& operator=(SerializableTxBody&&);

    std::vector<SerializableTxInput> inputs;
    std::vector<SerializableTxOutput> outputs;
    base::StrictNumeric<uint64_t> fee = 0u;
    std::optional<base::StrictNumeric<uint64_t>> ttl;
  };

  struct SerializableVkeyWitness {
    SerializableVkeyWitness();
    ~SerializableVkeyWitness();
    SerializableVkeyWitness(const SerializableVkeyWitness&);
    SerializableVkeyWitness& operator=(const SerializableVkeyWitness&);
    SerializableVkeyWitness(SerializableVkeyWitness&&);
    SerializableVkeyWitness& operator=(SerializableVkeyWitness&&);

    std::array<uint8_t, kCardanoSignatureSize> signature_bytes = {};
    std::array<uint8_t, kCardanoPubKeySize> public_key = {};
  };

  struct SerializableTxWitness {
    SerializableTxWitness();
    ~SerializableTxWitness();
    SerializableTxWitness(const SerializableTxWitness&);
    SerializableTxWitness& operator=(const SerializableTxWitness&);
    SerializableTxWitness(SerializableTxWitness&&);
    SerializableTxWitness& operator=(SerializableTxWitness&&);

    std::vector<SerializableVkeyWitness> vkey_witness_set;
  };

  struct SerializableTx {
    SerializableTx();
    ~SerializableTx();
    SerializableTx(const SerializableTx&);
    SerializableTx& operator=(const SerializableTx&);
    SerializableTx(SerializableTx&&);
    SerializableTx& operator=(SerializableTx&&);

    SerializableTxBody tx_body;
    SerializableTxWitness tx_witness;
  };

  struct DecodedTx {
    DecodedTx();
    ~DecodedTx();
    DecodedTx(const DecodedTx&);
    DecodedTx(DecodedTx&&);

    SerializableTx tx;
    std::vector<uint8_t> raw_tx_bytes;
    std::vector<uint8_t> raw_body_bytes;
  };

  CardanoTxDecoder();
  ~CardanoTxDecoder();

  static void SetUseSetTagForTesting(bool enable);

  static std::optional<std::vector<uint8_t>> EncodeTransaction(
      const SerializableTx& tx);
  static std::optional<std::vector<uint8_t>> EncodeTransactionOutput(
      const SerializableTxOutput& output);
  static std::optional<std::array<uint8_t, kCardanoTxHashSize>>
  GetTransactionHash(const SerializableTx& tx);

  static std::optional<DecodedTx> DecodeTransaction(
      base::span<const uint8_t> cbor_bytes);

  static std::optional<std::vector<uint8_t>> AddWitnessesToTransaction(
      const std::vector<uint8_t>& unsigned_tx_bytes,
      const SerializableTxWitness& witness);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_CARDANO_TX_DECODER_H_
