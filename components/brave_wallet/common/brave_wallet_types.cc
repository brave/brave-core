/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

#include <optional>
#include <utility>

#include "base/strings/string_number_conversions.h"

namespace brave_wallet {

Log::Log() = default;
Log::~Log() = default;
Log::Log(const Log&) = default;

bool Log::operator==(const Log& log) const {
  return address == log.address && block_hash == log.block_hash &&
         block_number == log.block_number && data == log.data &&
         log_index == log.log_index && removed == log.removed &&
         topics == log.topics && transaction_hash == log.transaction_hash &&
         transaction_index == log.transaction_index;
}

bool Log::operator!=(const Log& log) const {
  return !operator==(log);
}

TransactionReceipt::TransactionReceipt() = default;
TransactionReceipt::~TransactionReceipt() = default;
TransactionReceipt::TransactionReceipt(const TransactionReceipt&) = default;

bool TransactionReceipt::operator==(
    const TransactionReceipt& tx_receipt) const {
  return transaction_hash == tx_receipt.transaction_hash &&
         transaction_index == tx_receipt.transaction_index &&
         block_hash == tx_receipt.block_hash &&
         block_number == tx_receipt.block_number && from == tx_receipt.from &&
         to == tx_receipt.to &&
         cumulative_gas_used == tx_receipt.cumulative_gas_used &&
         gas_used == tx_receipt.gas_used &&
         contract_address == tx_receipt.contract_address &&
         logs_bloom == tx_receipt.logs_bloom && status == tx_receipt.status &&
         logs == tx_receipt.logs;
}

bool TransactionReceipt::operator!=(
    const TransactionReceipt& tx_receipt) const {
  return !operator==(tx_receipt);
}

SolanaSignatureStatus::SolanaSignatureStatus(
    uint64_t slot,
    uint64_t confirmations,
    const std::string& err,
    const std::string& confirmation_status)
    : slot(slot),
      confirmations(confirmations),
      err(err),
      confirmation_status(confirmation_status) {}

bool SolanaSignatureStatus::operator==(
    const SolanaSignatureStatus& sig_status) const {
  return slot == sig_status.slot && confirmations == sig_status.confirmations &&
         err == sig_status.err &&
         confirmation_status == sig_status.confirmation_status;
}

bool SolanaSignatureStatus::operator!=(
    const SolanaSignatureStatus& sig_status) const {
  return !operator==(sig_status);
}

base::Value::Dict SolanaSignatureStatus::ToValue() const {
  base::Value::Dict dict;
  dict.Set("slot", base::NumberToString(slot));
  dict.Set("confirmations", base::NumberToString(confirmations));
  dict.Set("err", err);
  dict.Set("confirmation_status", confirmation_status);
  return dict;
}

// static
std::optional<SolanaSignatureStatus> SolanaSignatureStatus::FromValue(
    const base::Value::Dict& value) {
  SolanaSignatureStatus status;
  const std::string* slot_string = value.FindString("slot");
  if (!slot_string || !base::StringToUint64(*slot_string, &status.slot)) {
    return std::nullopt;
  }

  const std::string* confirmations_string = value.FindString("confirmations");
  if (!confirmations_string ||
      !base::StringToUint64(*confirmations_string, &status.confirmations)) {
    return std::nullopt;
  }

  const std::string* err = value.FindString("err");
  if (!err) {
    return std::nullopt;
  }
  status.err = *err;

  const std::string* confirmation_status =
      value.FindString("confirmation_status");
  if (!confirmation_status) {
    return std::nullopt;
  }
  status.confirmation_status = *confirmation_status;

  return status;
}

bool SolanaAccountInfo::operator==(const SolanaAccountInfo& info) const {
  return lamports == info.lamports && owner == info.owner &&
         data == info.data && executable == info.executable &&
         rent_epoch == info.rent_epoch;
}

bool SolanaAccountInfo::operator!=(const SolanaAccountInfo& info) const {
  return !operator==(info);
}

bool ValidSolidityBits(size_t bits) {
  return bits != 0 && bits % 8 == 0 && bits <= 256;
}

std::optional<uint256_t> MaxSolidityUint(size_t bits) {
  if (!ValidSolidityBits(bits)) {
    return std::nullopt;
  }
  // Max hex for intN value is 0x[ff]... for num bytes
  uint256_t value = 0;
  const size_t num_bytes = bits / 8;
  for (size_t i = 0; i < num_bytes; i++) {
    value <<= 8;
    value += 0xff;
  }
  return value;
}

std::optional<int256_t> MaxSolidityInt(size_t bits) {
  if (!ValidSolidityBits(bits)) {
    return std::nullopt;
  }
  // Max hex for intN value is 0x7f[ff]... for num bytes - 1
  int256_t value = 0x7f;
  const size_t num_bytes = bits / 8;
  for (size_t i = 0; i < num_bytes - 1; i++) {
    value <<= 8;
    value += 0xff;
  }
  return value;
}

std::optional<int256_t> MinSolidityInt(size_t bits) {
  if (!ValidSolidityBits(bits)) {
    return std::nullopt;
  }
  // Min hex for intN value is 0x80[00]... for num bytes - 1
  // A simple bit shift doesn't work quite right because of
  // using boost's int256_t type.
  int256_t value = 0x80 * -1;
  const size_t num_bytes = bits / 8;
  for (size_t i = 0; i < num_bytes - 1; i++) {
    value *= 256;
  }
  return value;
}

}  // namespace brave_wallet
