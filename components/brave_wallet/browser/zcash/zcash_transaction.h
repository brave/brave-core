/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_H_

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"

namespace brave_wallet {

class ZCashTransaction {
 public:
  struct Outpoint {
    Outpoint();
    ~Outpoint();
    Outpoint(const Outpoint& other);
    Outpoint& operator=(const Outpoint& other);
    Outpoint(Outpoint&& other);
    Outpoint& operator=(Outpoint&& other);
    bool operator==(const Outpoint& other) const;
    bool operator!=(const Outpoint& other) const;

    base::Value::Dict ToValue() const;
    static std::optional<Outpoint> FromValue(const base::Value::Dict& value);

    std::array<uint8_t, 32> txid;
    uint32_t index = 0;
  };

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
        const zcash::mojom::ZCashUtxo& utxo);

    std::string utxo_address;
    Outpoint utxo_outpoint;
    uint64_t utxo_value = 0;
    uint32_t n_sequence = 0xffffffff;

    std::vector<uint8_t> script_pub_key;
    std::vector<uint8_t> script_sig;  // scriptSig aka unlock script.

    bool IsSigned() const;
  };

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

    std::string address;
    std::vector<uint8_t> script_pubkey;
    uint64_t amount = 0;
  };

  struct TransparentPart {
    TransparentPart();
    ~TransparentPart();
    TransparentPart(TransparentPart&& other);
    TransparentPart(const TransparentPart& other);
    TransparentPart& operator=(const TransparentPart& other);
    TransparentPart& operator=(TransparentPart&& other);
    bool operator==(const TransparentPart& other) const;
    bool operator!=(const TransparentPart& other) const;

    std::vector<TxInput> inputs;
    std::vector<TxOutput> outputs;
  };

  using OrchardOutput = ::brave_wallet::OrchardOutput;

  struct OrchardPart {
    OrchardPart();
    ~OrchardPart();
    OrchardPart(OrchardPart&& other);
    OrchardPart(const OrchardPart& other);
    OrchardPart& operator=(const OrchardPart& other);
    OrchardPart& operator=(OrchardPart&& other);
    bool operator==(const OrchardPart& other) const;
    bool operator!=(const OrchardPart& other) const;

    // Only outputs are supported
    std::vector<OrchardOutput> outputs;
    std::optional<std::array<uint8_t, kZCashDigestSize>> digest;
    std::optional<std::vector<uint8_t>> raw_tx;
  };

  ZCashTransaction();
  ~ZCashTransaction();
  ZCashTransaction(const ZCashTransaction& other);
  ZCashTransaction& operator=(const ZCashTransaction& other);
  ZCashTransaction(ZCashTransaction&& other);
  ZCashTransaction& operator=(ZCashTransaction&& other);
  bool operator==(const ZCashTransaction& other) const;
  bool operator!=(const ZCashTransaction& other) const;

  base::Value::Dict ToValue() const;
  static std::optional<ZCashTransaction> FromValue(
      const base::Value::Dict& value);

  bool IsTransparentPartSigned() const;
  uint64_t TotalInputsAmount() const;

  uint8_t sighash_type() const;

  std::string to() const { return to_; }
  void set_to(const std::string& to) { to_ = to; }

  const std::optional<OrchardMemo>& memo() const { return memo_; }
  void set_memo(const std::optional<OrchardMemo>& memo) { memo_ = memo; }

  uint64_t amount() const { return amount_; }
  void set_amount(uint64_t amount) { amount_ = amount; }

  uint64_t fee() const { return fee_; }
  void set_fee(uint64_t fee) { fee_ = fee; }

  const TransparentPart& transparent_part() const { return transparent_part_; }
  TransparentPart& transparent_part() { return transparent_part_; }

  const OrchardPart& orchard_part() const { return orchard_part_; }
  OrchardPart& orchard_part() { return orchard_part_; }

  uint32_t locktime() const { return locktime_; }
  void set_locktime(uint32_t locktime) { locktime_ = locktime; }

  uint32_t expiry_height() const { return expiry_height_; }
  void set_expiry_height(uint32_t expiry_height) {
    expiry_height_ = expiry_height;
  }

  uint32_t consensus_brach_id() const { return consensus_brach_id_; }
  void set_consensus_brach_id(uint32_t consensus_brach_id) {
    consensus_brach_id_ = consensus_brach_id;
  }

 private:
  TransparentPart transparent_part_;
  OrchardPart orchard_part_;

  uint32_t locktime_ = 0;
  uint32_t expiry_height_ = 0;
  std::string to_;
  std::optional<OrchardMemo> memo_;
  uint64_t amount_ = 0;
  uint64_t fee_ = 0;
  uint32_t consensus_brach_id_ = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_H_
