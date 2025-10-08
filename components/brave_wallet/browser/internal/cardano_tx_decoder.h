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
#include "brave/components/brave_wallet/common/cardano_address.h"

namespace brave_wallet {

// Wrapper class for Cardano transaction decoding functionality
// This class provides a C++ interface to the Rust cardano_tx_decoder
// implementation
class CardanoTxDecoder {
 public:
  struct CardanoSignMessageResult {
    CardanoSignMessageResult();
    CardanoSignMessageResult(std::vector<uint8_t> signature_bytes,
                             std::vector<uint8_t> public_key);
    CardanoSignMessageResult(const CardanoSignMessageResult&);
    CardanoSignMessageResult(CardanoSignMessageResult&&);
    ~CardanoSignMessageResult();

    std::vector<uint8_t> signature_bytes;
    std::vector<uint8_t> public_key;
  };

  struct RestoredTransactionInput {
    RestoredTransactionInput();
    RestoredTransactionInput(const RestoredTransactionInput&);
    RestoredTransactionInput(RestoredTransactionInput&&);
    ~RestoredTransactionInput();

    std::array<uint8_t, 32u> tx_hash = {};
    uint32_t index = 0;

    // This is non-null for the account-related inputs.
    std::optional<CardanoAddress> address;
    std::optional<uint64_t> amount;
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
    RestoredTransactionBody& operator=(RestoredTransactionBody&&);

    std::vector<RestoredTransactionInput> inputs;
    std::vector<RestoredTransactionOutput> outputs;
    std::vector<uint8_t> raw_body_bytes;
  };

  struct RestoredTransaction {
    RestoredTransaction();
    ~RestoredTransaction();
    RestoredTransaction(const RestoredTransaction&);
    RestoredTransaction(RestoredTransaction&&);

    RestoredTransactionBody tx_body;
    std::vector<uint8_t> raw_tx_bytes;
  };

  CardanoTxDecoder();
  ~CardanoTxDecoder();

  static std::optional<RestoredTransaction> DecodeTransaction(
      base::span<const uint8_t> cbor_bytes);

  static std::optional<std::vector<uint8_t>> AddWitnessesToTransaction(
      const std::vector<uint8_t>& unsigned_tx_bytes,
      const std::vector<CardanoSignMessageResult>& witness);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_CARDANO_TX_DECODER_H_
