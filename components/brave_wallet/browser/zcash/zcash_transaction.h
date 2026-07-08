/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_H_

#include <array>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "base/check.h"
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

    base::DictValue ToValue() const;
    static std::optional<Outpoint> FromValue(const base::DictValue& value);

    std::array<uint8_t, 32> txid{};
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

    base::DictValue ToValue() const;
    static std::optional<TxInput> FromValue(const base::DictValue& value);

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

    base::DictValue ToValue() const;
    static std::optional<TxOutput> FromValue(const base::DictValue& value);

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

    bool IsEmpty() const;

    std::vector<TxInput> inputs;
    std::vector<TxOutput> outputs;
  };

  using OrchardOutput = ::brave_wallet::OrchardOutput;
  using OrchardInput = ::brave_wallet::OrchardInput;

  // A single orchard-format shielded pool serialized as opaque rust bytes.
  // Used for v5's orchard pool and v6's legacy-orchard + ironwood pools.
  struct ShieldedPool {
    ShieldedPool();
    ~ShieldedPool();
    ShieldedPool(ShieldedPool&& other);
    ShieldedPool(const ShieldedPool& other);
    ShieldedPool& operator=(const ShieldedPool& other);
    ShieldedPool& operator=(ShieldedPool&& other);
    bool operator==(const ShieldedPool& other) const;

    base::DictValue ToValue() const;
    static std::optional<ShieldedPool> FromValue(const base::DictValue& value);

    uint64_t TotalInputsAmount() const;
    uint64_t TotalOutputsAmount() const;

    std::vector<OrchardInput> inputs;
    std::vector<OrchardOutput> outputs;
    std::optional<uint32_t> anchor_block_height;
    std::optional<std::array<uint8_t, kZCashDigestSize>> digest;
    std::optional<std::vector<uint8_t>> raw_tx;
  };

  // v5 transaction shielded data: a single orchard pool.
  struct V5Part {
    V5Part();
    ~V5Part();
    V5Part(V5Part&& other);
    V5Part(const V5Part& other);
    V5Part& operator=(const V5Part& other);
    V5Part& operator=(V5Part&& other);
    bool operator==(const V5Part& other) const;

    // Backward-compatible top-level v5 JSON (keys: orchard_inputs,
    // orchard_outputs, anchor_block_height) written directly onto the tx dict.
    void WriteTopLevel(base::DictValue& tx_dict) const;
    static std::optional<V5Part> ReadTopLevel(const base::DictValue& tx_dict);

    uint64_t TotalInputsAmount() const;
    uint64_t TotalOutputsAmount() const;

    ShieldedPool orchard;
  };

  // v6-only transaction data. A ZCashTransaction is v6 iff its version_part_
  // holds a V6Part.
  struct V6Part {
    V6Part();
    ~V6Part();
    V6Part(V6Part&& other);
    V6Part(const V6Part& other);
    V6Part& operator=(const V6Part& other);
    V6Part& operator=(V6Part&& other);
    bool operator==(const V6Part& other) const;

    base::DictValue ToValue() const;
    static std::optional<V6Part> FromValue(const base::DictValue& value);

    uint64_t TotalInputsAmount() const;
    uint64_t TotalOutputsAmount() const;

    // ZIP 233: value removed from circulation. u64 LE on the wire; stored as
    // int64_t to match rust's Zatoshis/ZatBalance i64 domain.
    int64_t zip233_amount = 0;

    ShieldedPool legacy_orchard;  // OrchardPool::kOrchard
    ShieldedPool ironwood;        // OrchardPool::kIronwood

    // ZIP 231 memo bundle segments. Serialization deferred; kept empty.
    std::vector<std::array<uint8_t, kOrchardMemoSize>> memo_segments;
  };

  ZCashTransaction();
  ~ZCashTransaction();
  ZCashTransaction(const ZCashTransaction& other);
  ZCashTransaction& operator=(const ZCashTransaction& other);
  ZCashTransaction(ZCashTransaction&& other);
  ZCashTransaction& operator=(ZCashTransaction&& other);
  bool operator==(const ZCashTransaction& other) const;

  base::DictValue ToValue() const;
  static std::optional<ZCashTransaction> FromValue(
      const base::DictValue& value);

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

  bool is_v5() const { return std::holds_alternative<V5Part>(version_part_); }
  bool is_v6() const { return std::holds_alternative<V6Part>(version_part_); }

  const V5Part& v5_part() const {
    CHECK(is_v5());
    return std::get<V5Part>(version_part_);
  }
  V5Part& v5_part() {
    CHECK(is_v5());
    return std::get<V5Part>(version_part_);
  }
  const V6Part& v6_part() const {
    CHECK(is_v6());
    return std::get<V6Part>(version_part_);
  }
  V6Part& v6_part() {
    CHECK(is_v6());
    return std::get<V6Part>(version_part_);
  }

  // Replaces a default (v5) transaction's shielded data with an empty v6 part.
  void ConvertToV6() { version_part_ = V6Part{}; }

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

  bool ValidateAmounts();

 private:
  TransparentPart transparent_part_;
  // V5Part is the first alternative, so a default-constructed transaction is v5.
  std::variant<V5Part, V6Part> version_part_;

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
