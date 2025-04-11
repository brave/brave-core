/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/common/cardano_address.h"

namespace brave_wallet {

inline constexpr uint32_t kCardanoTxHashSize = 32u;
inline constexpr uint32_t kCardanoWitnessSize = 96u;

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

    base::Value::Dict ToValue() const;
    static std::optional<Outpoint> FromValue(const base::Value::Dict& value);

    std::array<uint8_t, kCardanoTxHashSize> txid;
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

    base::Value::Dict ToValue() const;
    static std::optional<TxInput> FromValue(const base::Value::Dict& value);

    static std::optional<TxInput> FromRpcUtxo(
        const CardanoAddress& address,
        const cardano_rpc::UnspentOutput& utxo);

    CardanoAddress utxo_address;
    Outpoint utxo_outpoint;
    uint64_t utxo_value = 0;
  };

  // Transaction witness. Matches a TxInput within transaction based on its
  // position. A pair of pubkey and signature bytes.
  struct TxWitness {
    TxWitness();
    explicit TxWitness(std::array<uint8_t, kCardanoWitnessSize> witness_bytes);
    ~TxWitness();
    TxWitness(const TxWitness& other);
    TxWitness& operator=(const TxWitness& other);
    TxWitness(TxWitness&& other);
    TxWitness& operator=(TxWitness&& other);
    auto operator<=>(const TxWitness& other) const = default;

    base::Value::Dict ToValue() const;
    static std::optional<TxWitness> FromValue(const base::Value::Dict& value);

    static TxWitness DummyTxWitness();

    std::array<uint8_t, kCardanoWitnessSize> witness_bytes = {};
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

    base::Value::Dict ToValue() const;
    static std::optional<TxOutput> FromValue(const base::Value::Dict& value);

    TxOutputType type = TxOutputType::kTarget;
    CardanoAddress address;
    uint64_t amount = 0;
  };

  CardanoTransaction();
  ~CardanoTransaction();
  CardanoTransaction(const CardanoTransaction& other);
  CardanoTransaction& operator=(const CardanoTransaction& other);
  CardanoTransaction(CardanoTransaction&& other);
  CardanoTransaction& operator=(CardanoTransaction&& other);
  bool operator==(const CardanoTransaction& other) const;
  bool operator!=(const CardanoTransaction& other) const;

  base::Value::Dict ToValue() const;
  static std::optional<CardanoTransaction> FromValue(
      const base::Value::Dict& value);

  // All inputs are signed.
  bool IsSigned() const;

  // Sum of all inputs' amounts.
  uint64_t TotalInputsAmount() const;

  // Sum of all outputs' amounts.
  uint64_t TotalOutputsAmount() const;

  // Checks if sum of inputs is GE than sum of outputs plus fee.
  bool AmountsAreValid(uint64_t min_fee) const;

  // Fee is calculated as sum of inputs which minus sum of outputs.
  uint64_t EffectiveFeeAmount() const;

  const CardanoAddress& to() const { return to_; }
  void set_to(CardanoAddress to) { to_ = std::move(to); }

  uint64_t amount() const { return amount_; }
  void set_amount(uint64_t amount) { amount_ = amount; }

  bool sending_max_amount() const { return sending_max_amount_; }
  void set_sending_max_amount(bool sending_max_amount) {
    sending_max_amount_ = sending_max_amount;
  }

  const std::vector<TxInput>& inputs() const { return inputs_; }
  void AddInput(TxInput input);
  void AddInputs(std::vector<TxInput> input);
  void ClearInputs();

  const std::vector<TxWitness>& witnesses() const { return witnesses_; }
  void AddWitness(TxWitness witnesses);
  void SetWitnesses(std::vector<TxWitness> witnesses);

  const std::vector<TxOutput>& outputs() const { return outputs_; }
  void AddOutput(TxOutput output);
  void ClearOutputs();
  void ClearChangeOutput();
  const TxOutput* TargetOutput() const;
  const TxOutput* ChangeOutput() const;
  TxOutput* TargetOutput();
  TxOutput* ChangeOutput();

  // Adjust amount of change output so transaction fee is equal to `min_fee`.
  uint64_t MoveSurplusFeeToChangeOutput(uint64_t min_fee);

  uint32_t invalid_after() const { return invalid_after_; }
  void set_invalid_after(uint32_t invalid_after) {
    invalid_after_ = invalid_after;
  }

 private:
  std::vector<TxInput> inputs_;
  std::vector<TxOutput> outputs_;
  std::vector<TxWitness> witnesses_;
  uint32_t invalid_after_ = 0;
  CardanoAddress to_;
  uint64_t amount_ = 0;
  bool sending_max_amount_ = false;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_H_
