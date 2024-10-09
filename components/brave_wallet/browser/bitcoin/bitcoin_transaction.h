/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/bitcoin_rpc_responses.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

class BitcoinTransaction {
 public:
  // Bitcoin tx outpoint. Pair of transaction id and its output index.
  struct Outpoint {
    Outpoint();
    ~Outpoint();
    Outpoint(const Outpoint& other);
    Outpoint& operator=(const Outpoint& other);
    Outpoint(Outpoint&& other);
    Outpoint& operator=(Outpoint&& other);
    bool operator==(const Outpoint& other) const;
    bool operator!=(const Outpoint& other) const;
    bool operator<(const Outpoint& other) const;

    base::Value::Dict ToValue() const;
    static std::optional<Outpoint> FromValue(const base::Value::Dict& value);

    SHA256HashArray txid;
    uint32_t index = 0;
  };

  // Input of bitcoin transaction.
  struct TxInput {
    TxInput();
    ~TxInput();
    TxInput(const TxInput& other);
    TxInput& operator=(const TxInput& other);
    TxInput(TxInput&& other);
    TxInput& operator=(TxInput&& other);
    bool operator==(const TxInput& other) const;
    bool operator!=(const TxInput& other) const;

    base::Value::Dict ToValue() const;
    static std::optional<TxInput> FromValue(const base::Value::Dict& value);

    static std::optional<TxInput> FromRpcUtxo(
        const std::string& address,
        const bitcoin_rpc::UnspentOutput& utxo);

    std::string utxo_address;
    Outpoint utxo_outpoint;
    uint64_t utxo_value = 0;
    std::optional<std::vector<uint8_t>> raw_outpoint_tx;

    std::vector<uint8_t> script_sig;  // scriptSig aka unlock script.
    std::vector<uint8_t> witness;     // serialized witness stack.
    uint32_t n_sequence() const;

    bool IsSigned() const;
  };

  // A set of inputs for bitcoin transaction which should be spent together. Now
  // just grouped by same address.
  class TxInputGroup {
   public:
    TxInputGroup();
    ~TxInputGroup();
    TxInputGroup(const TxInputGroup& other);
    TxInputGroup& operator=(const TxInputGroup& other);
    TxInputGroup(TxInputGroup&& other);
    TxInputGroup& operator=(TxInputGroup&& other);

    const std::vector<BitcoinTransaction::TxInput>& inputs() const {
      return inputs_;
    }

    void AddInput(BitcoinTransaction::TxInput input);
    void AddInputs(std::vector<BitcoinTransaction::TxInput> inputs);

    uint64_t total_amount() const { return total_amount_; }

   private:
    std::vector<BitcoinTransaction::TxInput> inputs_;
    uint64_t total_amount_ = 0;
  };

  enum class TxOutputType { kTarget, kChange };

  // Output of bitcoin transaction. Has type of either `kTarget` or `kChange`.
  struct TxOutput {
    TxOutput();
    ~TxOutput();
    TxOutput(const TxOutput& other);
    TxOutput& operator=(const TxOutput& other);
    TxOutput(TxOutput&& other);
    TxOutput& operator=(TxOutput&& other);
    bool operator==(const TxOutput& other) const;
    bool operator!=(const TxOutput& other) const;

    base::Value::Dict ToValue() const;
    static std::optional<TxOutput> FromValue(const base::Value::Dict& value);

    TxOutputType type = TxOutputType::kTarget;
    std::string address;
    std::vector<uint8_t> script_pubkey;  // Lock script.
    uint64_t amount = 0;
  };

  BitcoinTransaction();
  ~BitcoinTransaction();
  BitcoinTransaction(const BitcoinTransaction& other);
  BitcoinTransaction& operator=(const BitcoinTransaction& other);
  BitcoinTransaction(BitcoinTransaction&& other);
  BitcoinTransaction& operator=(BitcoinTransaction&& other);
  bool operator==(const BitcoinTransaction& other) const;
  bool operator!=(const BitcoinTransaction& other) const;

  base::Value::Dict ToValue() const;
  static std::optional<BitcoinTransaction> FromValue(
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

  uint8_t sighash_type() const;

  std::string to() const { return to_; }
  void set_to(const std::string& to) { to_ = to; }

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
  void SetInputWitness(size_t input_index, std::vector<uint8_t> witness);
  void SetInputRawTransaction(size_t input_index, std::vector<uint8_t> raw_tx);

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

  uint32_t locktime() const { return locktime_; }
  void set_locktime(uint32_t locktime) { locktime_ = locktime; }

  // Shuffle order of inputs and outputs to increase privacy.
  void ShuffleTransaction();
  // Arrange order of inputs and outputs so transaction binary form is suitable
  // for testing.
  void ArrangeTransactionForTesting();

 private:
  std::vector<TxInput> inputs_;
  std::vector<TxOutput> outputs_;
  uint32_t locktime_ = 0;
  std::string to_;
  uint64_t amount_ = 0;
  bool sending_max_amount_ = false;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TRANSACTION_H_
