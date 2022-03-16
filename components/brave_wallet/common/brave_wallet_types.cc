/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace brave_wallet {

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
         logs_bloom == tx_receipt.logs_bloom && status == tx_receipt.status;
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

base::Value SolanaSignatureStatus::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("slot", base::NumberToString(slot));
  dict.SetStringKey("confirmations", base::NumberToString(confirmations));
  dict.SetStringKey("err", err);
  dict.SetStringKey("confirmation_status", confirmation_status);
  return dict;
}

// static
absl::optional<SolanaSignatureStatus> SolanaSignatureStatus::FromValue(
    const base::Value& value) {
  if (!value.is_dict())
    return absl::nullopt;

  SolanaSignatureStatus status;
  const std::string* slot_string = value.FindStringKey("slot");
  if (!slot_string || !base::StringToUint64(*slot_string, &status.slot))
    return absl::nullopt;

  const std::string* confirmations_string =
      value.FindStringKey("confirmations");
  if (!confirmations_string ||
      !base::StringToUint64(*confirmations_string, &status.confirmations))
    return absl::nullopt;

  const std::string* err = value.FindStringKey("err");
  if (!err)
    return absl::nullopt;
  status.err = *err;

  const std::string* confirmation_status =
      value.FindStringKey("confirmation_status");
  if (!confirmation_status)
    return absl::nullopt;
  status.confirmation_status = *confirmation_status;

  return status;
}

}  // namespace brave_wallet
