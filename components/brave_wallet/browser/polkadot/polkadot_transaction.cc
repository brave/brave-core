/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_transaction.h"

#include "base/numerics/checked_math.h"
#include "base/strings/string_number_conversions.h"

namespace brave_wallet {

PolkadotTransaction::PolkadotTransaction() = default;
PolkadotTransaction::~PolkadotTransaction() = default;
PolkadotTransaction::PolkadotTransaction(PolkadotTransaction&&) = default;

base::Value::Dict PolkadotTransaction::ToValue() const {
  base::Value::Dict dict;

  dict.Set("amount", base::HexEncodeLower(base::byte_span_from_ref(amount_)));
  dict.Set("fee", base::HexEncodeLower(base::byte_span_from_ref(fee_)));
  dict.Set("recipient", base::HexEncodeLower(recipient_.pubkey));
  dict.Set("transfer_all", transfer_all_);
  if (recipient_.ss58_prefix) {
    dict.Set("ss58_prefix", *recipient_.ss58_prefix);
  }

  return dict;
}

// static
std::optional<PolkadotTransaction> PolkadotTransaction::FromValue(
    const base::Value::Dict& value) {
  uint128_t amount = 0;
  uint128_t fee = 0;
  bool transfer_all = false;

  const auto* json_amount = value.FindString("amount");
  if (!json_amount) {
    return std::nullopt;
  }

  if (!base::HexStringToSpan(*json_amount, base::byte_span_from_ref(amount))) {
    return std::nullopt;
  }

  const auto* json_fee = value.FindString("fee");
  if (!json_fee) {
    return std::nullopt;
  }

  if (!base::HexStringToSpan(*json_fee, base::byte_span_from_ref(fee))) {
    return std::nullopt;
  }

  auto json_transfer_all = value.FindBool("transfer_all");
  if (!json_transfer_all) {
    return std::nullopt;
  }

  transfer_all = *json_transfer_all;

  const auto* json_recipient = value.FindString("recipient");
  if (!json_recipient) {
    return std::nullopt;
  }

  PolkadotAddress recipient;

  if (!base::HexStringToSpan(*json_recipient, recipient.pubkey)) {
    return std::nullopt;
  }

  const auto ss58_prefix_json = value.FindInt("ss58_prefix");
  if (ss58_prefix_json) {
    recipient.ss58_prefix.emplace();
    if (!base::CheckedNumeric<uint16_t>(*ss58_prefix_json)
             .AssignIfValid(&recipient.ss58_prefix.value())) {
      return std::nullopt;
    }
  } else if (value.contains("ss58_prefix")) {
    return std::nullopt;
  }

  PolkadotTransaction tx;
  tx.amount_ = amount;
  tx.fee_ = fee;
  tx.recipient_ = recipient;
  tx.transfer_all_ = transfer_all;

  return tx;
}

}  // namespace brave_wallet
