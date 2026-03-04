/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_H_

#include <array>
#include <optional>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/gtest_prod_util.h"
#include "base/numerics/checked_math.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.h"
#include "brave/components/brave_wallet/common/cardano_address.h"

namespace brave_wallet {

struct TxBuilderParms {
  TxBuilderParms();
  ~TxBuilderParms();
  TxBuilderParms(const TxBuilderParms&);
  TxBuilderParms& operator=(const TxBuilderParms&);
  TxBuilderParms(TxBuilderParms&&);
  TxBuilderParms& operator=(TxBuilderParms&&);

  // Amount of a lovelaces or tokens being sent.
  uint64_t amount = 0;
  // True if exact amount was not specified but we are sending all possible
  // amount of given token or lovelaces.
  bool sending_max_amount = false;
  // Token being sent if set, otherwise lovelaces are being sent.
  std::optional<cardano_rpc::TokenId> token_to_send;

  // Destination address for funds being sent.
  CardanoAddress send_to_address;
  // Change address in case we need to send change.
  CardanoAddress change_address;
  // Current state of blockchain. Used to calculate fee.
  cardano_rpc::EpochParameters epoch_parameters;

  uint64_t invalid_after = 0u;
};

// This class is used to make Cardano transactions for sending to blockchain.
class CardanoTransaction {
 public:
  // Cardano tx outpoint. Pair of transaction id and its output index.
  struct Outpoint {
    Outpoint();
    ~Outpoint();
    Outpoint(const Outpoint& other);
    Outpoint& operator=(const Outpoint& other);
    Outpoint(Outpoint&& other);
    Outpoint& operator=(Outpoint&& other);
    auto operator<=>(const Outpoint& other) const = default;

    base::DictValue ToValue() const;
    static std::optional<Outpoint> FromValue(const base::DictValue& value);

    std::array<uint8_t, kCardanoTxHashSize> txid = {};
    uint32_t index = 0;
  };

  // Input of cardano transaction.
  struct TxInput {
    TxInput();
    ~TxInput();
    TxInput(const TxInput& other);
    TxInput& operator=(const TxInput& other);
    TxInput(TxInput&& other);
    TxInput& operator=(TxInput&& other);
    auto operator<=>(const TxInput& other) const = default;

    base::DictValue ToValue() const;
    static std::optional<TxInput> FromValue(const base::DictValue& value);

    static TxInput FromRpcUtxo(const cardano_rpc::UnspentOutput& utxo);

    CardanoAddress utxo_address;
    Outpoint utxo_outpoint;
    uint64_t utxo_value = 0u;
    cardano_rpc::Tokens utxo_tokens;
  };

  // Transaction witness. Matches a TxInput within transaction based on its
  // position. A pair of pubkey and signature bytes.
  struct TxWitness {
    TxWitness();
    TxWitness(std::array<uint8_t, kCardanoPubKeySize> public_key,
              std::array<uint8_t, kCardanoSignatureSize> signature);
    ~TxWitness();
    TxWitness(const TxWitness& other);
    TxWitness& operator=(const TxWitness& other);
    TxWitness(TxWitness&& other);
    TxWitness& operator=(TxWitness&& other);
    auto operator<=>(const TxWitness& other) const = default;

    base::DictValue ToValue() const;
    static std::optional<TxWitness> FromValue(const base::DictValue& value);

    std::array<uint8_t, kCardanoPubKeySize> public_key = {};
    std::array<uint8_t, kCardanoSignatureSize> signature = {};
  };

  enum class TxOutputType { kTarget, kChange };

  // Output of cardano transaction. Has type of either `kTarget` or `kChange`.
  struct TxOutput {
    TxOutput();
    ~TxOutput();
    TxOutput(const TxOutput& other);
    TxOutput& operator=(const TxOutput& other);
    TxOutput(TxOutput&& other);
    TxOutput& operator=(TxOutput&& other);
    auto operator<=>(const TxOutput& other) const = default;

    base::DictValue ToValue() const;
    static std::optional<TxOutput> FromValue(const base::DictValue& value);

    CardanoTxDecoder::SerializableTxOutput ToSerializableTxOutput() const;

    TxOutputType type = TxOutputType::kTarget;
    CardanoAddress address;
    uint64_t amount = 0;
    cardano_rpc::Tokens tokens;
  };

  CardanoTransaction();
  ~CardanoTransaction();
  CardanoTransaction(const CardanoTransaction& other);
  CardanoTransaction& operator=(const CardanoTransaction& other);
  CardanoTransaction(CardanoTransaction&& other);
  CardanoTransaction& operator=(CardanoTransaction&& other);
  bool operator==(const CardanoTransaction& other) const;

  uint64_t fee() const { return fee_; }
  void set_fee(uint64_t fee) { fee_ = fee; }

  uint64_t invalid_after() const { return invalid_after_; }
  void set_invalid_after(uint64_t invalid_after) {
    invalid_after_ = invalid_after;
  }

  base::DictValue ToValue() const;
  static std::optional<CardanoTransaction> FromValue(
      const base::DictValue& value);

  // Adds target output.
  void SetupTargetOutput(CardanoAddress target_address);

  // Adds change output.
  void SetupChangeOutput(CardanoAddress change_address);

  // Sum of all inputs' amounts.
  base::CheckedNumeric<uint64_t> GetTotalInputsAmount() const;

  // Sum of all outputs' amounts.
  base::CheckedNumeric<uint64_t> GetTotalOutputsAmount() const;

  // Sum of all inputs' token amounts.
  std::optional<cardano_rpc::Tokens> GetTotalInputTokensAmount() const;

  // Sum of all outputs' token amounts.
  std::optional<cardano_rpc::Tokens> GetTotalOutputTokensAmount() const;

  std::optional<CardanoAddress> GetToAddress() const;

  bool IsSendTokenTransaction() const;

  const std::vector<TxInput>& inputs() const { return inputs_; }
  void AddInput(TxInput input);
  void AddInputs(std::vector<TxInput> input);
  void ClearInputs();
  base::flat_set<CardanoAddress> GetInputAddresses() const;

  const std::vector<TxWitness>& witnesses() const { return witnesses_; }
  void SetWitnesses(std::vector<TxWitness> witnesses);
  void AddWitness(TxWitness);

  const std::vector<TxOutput>& outputs() const { return outputs_; }
  void AddOutput(TxOutput output);
  void ClearOutputs();
  void ClearChangeOutput();
  const TxOutput* TargetOutput() const;
  const TxOutput* ChangeOutput() const;
  TxOutput* TargetOutput();
  TxOutput* ChangeOutput();

  // Adjust change output so all input tokens are also sent to change output.
  bool EnsureTokensInChangeOutput();

  std::optional<CardanoTxDecoder::SerializableTx> ToSerializableTx() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(CardanoTransactionSerializerTest, ValidateAmounts);
  FRIEND_TEST_ALL_PREFIXES(CardanoTransactionSerializerTest,
                           ValidateAmountsWithTokens);

  std::vector<TxInput> inputs_;
  std::vector<TxOutput> outputs_;
  std::vector<TxWitness> witnesses_;
  uint64_t invalid_after_ = 0;
  uint64_t fee_ = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_H_
