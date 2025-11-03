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

namespace brave_wallet {

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

    std::array<uint8_t, 32u> tx_hash = {};
    uint32_t index = 0;
  };

  struct SerializableTxOutput {
    SerializableTxOutput();
    SerializableTxOutput(const SerializableTxOutput&);
    SerializableTxOutput& operator=(const SerializableTxOutput&);
    SerializableTxOutput(SerializableTxOutput&&);
    SerializableTxOutput& operator=(SerializableTxOutput&&);
    ~SerializableTxOutput();

    std::vector<uint8_t> address_bytes;
    uint64_t amount = 0;
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
  };

  struct SerializableVkeyWitness {
    SerializableVkeyWitness();
    ~SerializableVkeyWitness();
    SerializableVkeyWitness(const SerializableVkeyWitness&);
    SerializableVkeyWitness& operator=(const SerializableVkeyWitness&);
    SerializableVkeyWitness(SerializableVkeyWitness&&);
    SerializableVkeyWitness& operator=(SerializableVkeyWitness&&);

    std::vector<uint8_t> signature_bytes;
    std::vector<uint8_t> public_key;
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

  static std::optional<DecodedTx> DecodeTransaction(
      base::span<const uint8_t> cbor_bytes);

  static std::optional<std::vector<uint8_t>> AddWitnessesToTransaction(
      const std::vector<uint8_t>& unsigned_tx_bytes,
      const SerializableTxWitness& witness);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_CARDANO_TX_DECODER_H_
