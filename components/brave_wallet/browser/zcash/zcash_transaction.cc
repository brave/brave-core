/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace {

constexpr uint8_t kZCashSigHashAll = 0x01;

}  // namespace

ZCashTransaction::ZCashTransaction() = default;
ZCashTransaction::~ZCashTransaction() = default;
ZCashTransaction& ZCashTransaction::operator=(const ZCashTransaction& other) =
    default;
ZCashTransaction::ZCashTransaction(const ZCashTransaction& other) = default;
ZCashTransaction::ZCashTransaction(ZCashTransaction&& other) = default;
ZCashTransaction& ZCashTransaction::operator=(ZCashTransaction&& other) =
    default;
bool ZCashTransaction::operator==(const ZCashTransaction& other) const =
    default;

ZCashTransaction::Outpoint::Outpoint() = default;
ZCashTransaction::Outpoint::~Outpoint() = default;
ZCashTransaction::Outpoint::Outpoint(const Outpoint& other) = default;
ZCashTransaction::Outpoint& ZCashTransaction::Outpoint::operator=(
    const Outpoint& other) = default;
ZCashTransaction::Outpoint::Outpoint(Outpoint&& other) = default;
ZCashTransaction::Outpoint& ZCashTransaction::Outpoint::operator=(
    Outpoint&& other) = default;
bool ZCashTransaction::Outpoint::operator==(
    const ZCashTransaction::Outpoint& other) const = default;

ZCashTransaction::ShieldedPool::ShieldedPool() = default;
ZCashTransaction::ShieldedPool::~ShieldedPool() = default;
ZCashTransaction::ShieldedPool::ShieldedPool(ShieldedPool&& other) = default;
ZCashTransaction::ShieldedPool::ShieldedPool(const ShieldedPool& other) =
    default;
ZCashTransaction::ShieldedPool& ZCashTransaction::ShieldedPool::operator=(
    ShieldedPool&& other) = default;
ZCashTransaction::ShieldedPool& ZCashTransaction::ShieldedPool::operator=(
    const ShieldedPool& other) = default;
bool ZCashTransaction::ShieldedPool::operator==(
    const ShieldedPool& other) const = default;

base::CheckedNumeric<uint64_t>
ZCashTransaction::ShieldedPool::TotalInputsAmount() const {
  base::CheckedNumeric<uint64_t> result = 0;
  for (const auto& input : inputs) {
    result += input.note.amount;
  }
  return result;
}

base::CheckedNumeric<uint64_t>
ZCashTransaction::ShieldedPool::TotalOutputsAmount() const {
  base::CheckedNumeric<uint64_t> result = 0;
  for (const auto& output : outputs) {
    result += output.value;
  }
  return result;
}

base::DictValue ZCashTransaction::ShieldedPool::ToValue() const {
  base::DictValue dict;
  auto& inputs_value = dict.Set("inputs", base::ListValue())->GetList();
  for (auto& input : inputs) {
    inputs_value.Append(input.ToValue());
  }
  auto& outputs_value = dict.Set("outputs", base::ListValue())->GetList();
  for (auto& output : outputs) {
    outputs_value.Append(output.ToValue());
  }
  if (anchor_block_height) {
    dict.Set("anchor_block_height", base::NumberToString(*anchor_block_height));
  }
  return dict;
}

// static
std::optional<ZCashTransaction::ShieldedPool>
ZCashTransaction::ShieldedPool::FromValue(const base::DictValue& dict) {
  ShieldedPool pool;
  auto* inputs_list = dict.FindList("inputs");
  if (inputs_list) {
    for (auto& item : *inputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto input_opt = OrchardInput::FromValue(item.GetDict());
      if (!input_opt) {
        return std::nullopt;
      }
      pool.inputs.push_back(std::move(*input_opt));
    }
  }
  auto* outputs_list = dict.FindList("outputs");
  if (outputs_list) {
    for (auto& item : *outputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto output_opt = OrchardOutput::FromValue(item.GetDict());
      if (!output_opt) {
        return std::nullopt;
      }
      pool.outputs.push_back(std::move(*output_opt));
    }
  }
  if (dict.Find("anchor_block_height")) {
    uint32_t height = 0;
    if (!ReadUint32StringTo(dict, "anchor_block_height", height)) {
      return std::nullopt;
    }
    pool.anchor_block_height = height;
  }
  return pool;
}

ZCashTransaction::V5Part::V5Part() = default;
ZCashTransaction::V5Part::~V5Part() = default;
ZCashTransaction::V5Part::V5Part(V5Part&& other) = default;
ZCashTransaction::V5Part::V5Part(const V5Part& other) = default;
ZCashTransaction::V5Part& ZCashTransaction::V5Part::operator=(V5Part&& other) =
    default;
ZCashTransaction::V5Part& ZCashTransaction::V5Part::operator=(
    const V5Part& other) = default;
bool ZCashTransaction::V5Part::operator==(const V5Part& other) const = default;

base::CheckedNumeric<uint64_t> ZCashTransaction::V5Part::TotalInputsAmount()
    const {
  return orchard.TotalInputsAmount();
}

base::CheckedNumeric<uint64_t> ZCashTransaction::V5Part::TotalOutputsAmount()
    const {
  return orchard.TotalOutputsAmount();
}

base::DictValue ZCashTransaction::V5Part::ToValue() const {
  base::DictValue dict;
  dict.Set("orchard", orchard.ToValue());
  return dict;
}

// static
std::optional<ZCashTransaction::V5Part> ZCashTransaction::V5Part::FromValue(
    const base::DictValue& value) {
  auto* orchard_dict = value.FindDict("orchard");
  if (!orchard_dict) {
    return std::nullopt;
  }
  auto orchard = ShieldedPool::FromValue(*orchard_dict);
  if (!orchard) {
    return std::nullopt;
  }

  V5Part result;
  result.orchard = std::move(*orchard);
  return result;
}

// static
std::optional<ZCashTransaction::V5Part> ZCashTransaction::V5Part::ReadTopLevel(
    const base::DictValue& value) {
  V5Part result;
  auto* orchard_inputs_list = value.FindList("orchard_inputs");
  if (orchard_inputs_list) {
    for (auto& item : *orchard_inputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto input_opt = OrchardInput::FromValue(item.GetDict());
      if (!input_opt) {
        return std::nullopt;
      }
      result.orchard.inputs.push_back(std::move(*input_opt));
    }
  }
  auto* orchard_outputs_list = value.FindList("orchard_outputs");
  if (orchard_outputs_list) {
    for (auto& item : *orchard_outputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto output_opt = OrchardOutput::FromValue(item.GetDict());
      if (!output_opt) {
        return std::nullopt;
      }
      result.orchard.outputs.push_back(std::move(*output_opt));
    }
  }
  if (value.Find("anchor_block_height")) {
    uint32_t anchor_block_height = 0;
    if (!ReadUint32StringTo(value, "anchor_block_height",
                            anchor_block_height)) {
      return std::nullopt;
    }
    result.orchard.anchor_block_height = anchor_block_height;
  }
  return result;
}

ZCashTransaction::V6Part::V6Part() = default;
ZCashTransaction::V6Part::~V6Part() = default;
ZCashTransaction::V6Part::V6Part(V6Part&& other) = default;
ZCashTransaction::V6Part::V6Part(const V6Part& other) = default;
ZCashTransaction::V6Part& ZCashTransaction::V6Part::operator=(V6Part&& other) =
    default;
ZCashTransaction::V6Part& ZCashTransaction::V6Part::operator=(
    const V6Part& other) = default;
bool ZCashTransaction::V6Part::operator==(const V6Part& other) const = default;

base::CheckedNumeric<uint64_t> ZCashTransaction::V6Part::TotalInputsAmount()
    const {
  return legacy_orchard.TotalInputsAmount() + ironwood.TotalInputsAmount();
}

base::CheckedNumeric<uint64_t> ZCashTransaction::V6Part::TotalOutputsAmount()
    const {
  return legacy_orchard.TotalOutputsAmount() + ironwood.TotalOutputsAmount();
}

base::DictValue ZCashTransaction::V6Part::ToValue() const {
  base::DictValue dict;
  dict.Set("legacy_orchard", legacy_orchard.ToValue());
  dict.Set("ironwood", ironwood.ToValue());
  return dict;
}

// static
std::optional<ZCashTransaction::V6Part> ZCashTransaction::V6Part::FromValue(
    const base::DictValue& value) {
  V6Part result;

  auto* legacy_dict = value.FindDict("legacy_orchard");
  if (!legacy_dict) {
    return std::nullopt;
  }
  auto legacy = ShieldedPool::FromValue(*legacy_dict);
  if (!legacy) {
    return std::nullopt;
  }
  result.legacy_orchard = std::move(*legacy);

  auto* ironwood_dict = value.FindDict("ironwood");
  if (!ironwood_dict) {
    return std::nullopt;
  }
  auto ironwood = ShieldedPool::FromValue(*ironwood_dict);
  if (!ironwood) {
    return std::nullopt;
  }
  result.ironwood = std::move(*ironwood);

  return result;
}

ZCashTransaction::TransparentPart::TransparentPart() = default;
ZCashTransaction::TransparentPart::~TransparentPart() = default;
ZCashTransaction::TransparentPart::TransparentPart(
    const TransparentPart& other) = default;
ZCashTransaction::TransparentPart::TransparentPart(TransparentPart&& other) =
    default;
ZCashTransaction::TransparentPart& ZCashTransaction::TransparentPart::operator=(
    TransparentPart&& other) = default;
ZCashTransaction::TransparentPart& ZCashTransaction::TransparentPart::operator=(
    const TransparentPart& other) = default;
bool ZCashTransaction::TransparentPart::operator==(
    const TransparentPart& other) const = default;
bool ZCashTransaction::TransparentPart::IsEmpty() const {
  return inputs.empty() && outputs.empty();
}

base::DictValue ZCashTransaction::Outpoint::ToValue() const {
  base::DictValue dict;

  dict.Set("txid", base::HexEncode(txid));
  dict.Set("index", static_cast<int>(index));

  return dict;
}

// static
std::optional<ZCashTransaction::Outpoint> ZCashTransaction::Outpoint::FromValue(
    const base::DictValue& value) {
  Outpoint result;

  auto* txid_hex = value.FindString("txid");
  if (!txid_hex) {
    return std::nullopt;
  }
  if (!base::HexStringToSpan(*txid_hex, result.txid)) {
    return std::nullopt;
  }

  auto index_value = value.FindInt("index");
  if (!index_value) {
    return std::nullopt;
  }
  result.index = *index_value;

  return result;
}

ZCashTransaction::TxInput::TxInput() = default;
ZCashTransaction::TxInput::~TxInput() = default;
ZCashTransaction::TxInput::TxInput(const ZCashTransaction::TxInput& other) =
    default;
ZCashTransaction::TxInput::TxInput(ZCashTransaction::TxInput&& other) = default;
ZCashTransaction::TxInput& ZCashTransaction::TxInput::operator=(
    const ZCashTransaction::TxInput& other) = default;
ZCashTransaction::TxInput& ZCashTransaction::TxInput::operator=(
    ZCashTransaction::TxInput&& other) = default;
bool ZCashTransaction::TxInput::operator==(
    const ZCashTransaction::TxInput& other) const = default;

base::DictValue ZCashTransaction::TxInput::ToValue() const {
  base::DictValue dict;

  dict.Set("utxo_address", utxo_address);
  dict.Set("utxo_outpoint", utxo_outpoint.ToValue());
  dict.Set("utxo_value", base::NumberToString(utxo_value));
  dict.Set("script_pub_key", base::HexEncode(script_pub_key));
  dict.Set("script_sig", base::HexEncode(script_sig));

  return dict;
}

// static
std::optional<ZCashTransaction::TxInput> ZCashTransaction::TxInput::FromValue(
    const base::DictValue& value) {
  ZCashTransaction::TxInput result;

  if (!ReadStringTo(value, "utxo_address", result.utxo_address)) {
    return std::nullopt;
  }

  if (!ReadDictTo(value, "utxo_outpoint", result.utxo_outpoint)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "utxo_value", result.utxo_value)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_pub_key", result.script_pub_key)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_sig", result.script_sig)) {
    return std::nullopt;
  }

  return result;
}

// static
std::optional<ZCashTransaction::TxInput> ZCashTransaction::TxInput::FromRpcUtxo(
    const std::string& address,
    const zcash::mojom::ZCashUtxo& utxo) {
  if (address != utxo.address) {
    return std::nullopt;
  }
  ZCashTransaction::TxInput result;
  result.utxo_address = utxo.address;
  result.script_pub_key.insert(result.script_pub_key.begin(),
                               utxo.script.begin(), utxo.script.end());
  if (utxo.tx_id.size() != 32) {
    return std::nullopt;
  }
  std::copy_n(utxo.tx_id.begin(), 32, result.utxo_outpoint.txid.begin());
  result.utxo_outpoint.index = utxo.index;
  result.utxo_value = utxo.value_zat;
  return result;
}

bool ZCashTransaction::TxInput::IsSigned() const {
  return !script_sig.empty();
}

ZCashTransaction::TxOutput::TxOutput() = default;
ZCashTransaction::TxOutput::~TxOutput() = default;
ZCashTransaction::TxOutput::TxOutput(ZCashTransaction::TxOutput&& other) =
    default;
ZCashTransaction::TxOutput::TxOutput(const ZCashTransaction::TxOutput& other) =
    default;
ZCashTransaction::TxOutput& ZCashTransaction::TxOutput::operator=(
    ZCashTransaction::TxOutput&& other) = default;
ZCashTransaction::TxOutput& ZCashTransaction::TxOutput::operator=(
    const ZCashTransaction::TxOutput& other) = default;
bool ZCashTransaction::TxOutput::operator==(
    const ZCashTransaction::TxOutput& other) const = default;

base::DictValue ZCashTransaction::TxOutput::ToValue() const {
  base::DictValue dict;

  dict.Set("address", address);
  dict.Set("amount", base::NumberToString(amount));
  dict.Set("script_pub_key", base::HexEncode(script_pubkey));

  return dict;
}

// static
std::optional<ZCashTransaction::TxOutput> ZCashTransaction::TxOutput::FromValue(
    const base::DictValue& value) {
  ZCashTransaction::TxOutput result;
  if (!ReadStringTo(value, "address", result.address)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_pub_key", result.script_pubkey)) {
    return std::nullopt;
  }

  return result;
}

base::DictValue ZCashTransaction::ToValue() const {
  CHECK(is_v5() || is_v6());

  base::DictValue dict;

  auto& inputs_value = dict.Set("inputs", base::ListValue())->GetList();
  for (auto& input : transparent_part_.inputs) {
    inputs_value.Append(input.ToValue());
  }

  auto& outputs_value = dict.Set("outputs", base::ListValue())->GetList();
  for (auto& output : transparent_part_.outputs) {
    outputs_value.Append(output.ToValue());
  }

  if (is_v5()) {
    dict.Set("v5_part", v5_part().ToValue());
  } else {
    dict.Set("v6_part", v6_part().ToValue());
  }

  dict.Set("locktime", base::NumberToString(locktime_));
  dict.Set("to", to_);
  dict.Set("amount", base::NumberToString(amount_));
  dict.Set("fee", base::NumberToString(fee_));
  dict.Set("expiry_height", base::NumberToString(expiry_height_));
  if (memo_) {
    dict.Set("memo", base::HexEncode(memo_.value()));
  }

  return dict;
}

// static
std::optional<ZCashTransaction> ZCashTransaction::FromValue(
    const base::DictValue& value) {
  ZCashTransaction result;

  auto* v6_dict = value.FindDict("v6_part");
  auto* v5_dict = value.FindDict("v5_part");

  // Legacy format was placed top-level.
  auto* inputs_list = value.FindList("inputs");
  auto* orchard_inputs_list = value.FindList("orchard_inputs");
  if (!inputs_list && !orchard_inputs_list && !v5_dict && !v6_dict) {
    return std::nullopt;
  }
  if (inputs_list) {
    for (auto& item : *inputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto input_opt = ZCashTransaction::TxInput::FromValue(item.GetDict());
      if (!input_opt) {
        return std::nullopt;
      }
      result.transparent_part_.inputs.push_back(std::move(*input_opt));
    }
  }

  auto* outputs_list = value.FindList("outputs");
  auto* orchard_outputs_list = value.FindList("orchard_outputs");
  if (!outputs_list && !orchard_outputs_list && !v5_dict && !v6_dict) {
    return std::nullopt;
  }
  if (outputs_list) {
    for (auto& item : *outputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto output_opt = ZCashTransaction::TxOutput::FromValue(item.GetDict());
      if (!output_opt) {
        return std::nullopt;
      }
      result.transparent_part_.outputs.push_back(std::move(*output_opt));
    }
  }

  if (v6_dict) {
    auto v6 = V6Part::FromValue(*v6_dict);
    if (!v6) {
      return std::nullopt;
    }
    result.version_part_ = std::move(*v6);
  } else if (v5_dict) {
    auto v5 = V5Part::FromValue(*v5_dict);
    if (!v5) {
      return std::nullopt;
    }
    result.version_part_ = std::move(*v5);
  } else {
    // Legacy format.
    auto v5 = V5Part::ReadTopLevel(value);
    if (!v5) {
      return std::nullopt;
    }
    result.version_part_ = std::move(*v5);
  }

  if (!ReadUint32StringTo(value, "locktime", result.locktime_)) {
    return std::nullopt;
  }
  if (!ReadStringTo(value, "to", result.to_)) {
    return std::nullopt;
  }
  if (!ReadUint64StringTo(value, "amount", result.amount_)) {
    return std::nullopt;
  }
  if (!ReadUint64StringTo(value, "fee", result.fee_)) {
    return std::nullopt;
  }
  if (value.Find("expiry_height")) {
    if (!ReadUint32StringTo(value, "expiry_height", result.expiry_height_)) {
      return std::nullopt;
    }
  }
  if (value.Find("memo")) {
    OrchardMemo memo;
    if (!ReadHexByteArrayTo<kOrchardMemoSize>(value, "memo", memo)) {
      return std::nullopt;
    }
    result.memo_ = memo;
  }

  return result;
}

bool ZCashTransaction::IsTransparentPartSigned() const {
  if (transparent_part_.inputs.empty()) {
    return false;
  }

  return std::ranges::all_of(transparent_part_.inputs,
                             [](auto& input) { return input.IsSigned(); });
}

base::CheckedNumeric<uint64_t> ZCashTransaction::TotalInputsAmount() const {
  CHECK(is_v5() || is_v6());

  base::CheckedNumeric<uint64_t> result = 0;
  for (auto& input : transparent_part_.inputs) {
    result += input.utxo_value;
  }
  if (is_v5()) {
    result += v5_part().TotalInputsAmount();
  } else if (is_v6()) {
    result += v6_part().TotalInputsAmount();
  }
  return result;
}

uint8_t ZCashTransaction::sighash_type() const {
  // We always sign all inputs.
  return kZCashSigHashAll;
}

bool ZCashTransaction::ValidateAmounts() {
  CHECK(is_v5() || is_v6());

  base::CheckedNumeric<uint64_t> inputs_sum = 0;
  base::CheckedNumeric<uint64_t> outputs_sum = 0;

  for (const auto& input : transparent_part_.inputs) {
    inputs_sum += input.utxo_value;
  }
  for (const auto& output : transparent_part_.outputs) {
    outputs_sum += output.amount;
  }

  if (is_v5()) {
    inputs_sum += v5_part().TotalInputsAmount();
    outputs_sum += v5_part().TotalOutputsAmount();
  } else if (is_v6()) {
    inputs_sum += v6_part().TotalInputsAmount();
    outputs_sum += v6_part().TotalOutputsAmount();
  }

  outputs_sum += fee_;

  if (!outputs_sum.IsValid() || !inputs_sum.IsValid()) {
    return false;
  }
  return outputs_sum.ValueOrDie() == inputs_sum.ValueOrDie();
}

}  // namespace brave_wallet
