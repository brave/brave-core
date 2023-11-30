/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message_header.h"

#include <optional>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace {

constexpr char kNumRequiredSignatures[] = "num_required_signatures";
constexpr char kNumReadonlySignedAccounts[] = "num_readonly_signed_accounts";
constexpr char kNumReadonlyUnsignedAccounts[] =
    "num_readonly_unsigned_accounts";

}  // namespace

namespace brave_wallet {

SolanaMessageHeader::SolanaMessageHeader(uint8_t num_required_signatures,
                                         uint8_t num_readonly_signed_accounts,
                                         uint8_t num_readonly_unsigned_accounts)
    : num_required_signatures(num_required_signatures),
      num_readonly_signed_accounts(num_readonly_signed_accounts),
      num_readonly_unsigned_accounts(num_readonly_unsigned_accounts) {}

bool SolanaMessageHeader::operator==(const SolanaMessageHeader& header) const {
  return num_required_signatures == header.num_required_signatures &&
         num_readonly_signed_accounts == header.num_readonly_signed_accounts &&
         num_readonly_unsigned_accounts ==
             header.num_readonly_unsigned_accounts;
}

base::Value::Dict SolanaMessageHeader::ToValue() const {
  base::Value::Dict dict;
  dict.Set(kNumRequiredSignatures,
           base::NumberToString(num_required_signatures));
  dict.Set(kNumReadonlySignedAccounts,
           base::NumberToString(num_readonly_signed_accounts));
  dict.Set(kNumReadonlyUnsignedAccounts,
           base::NumberToString(num_readonly_unsigned_accounts));
  return dict;
}

// static
std::optional<SolanaMessageHeader> SolanaMessageHeader::FromValue(
    const base::Value::Dict& value) {
  auto num_required_signatures =
      GetUint8FromStringDict(value, kNumRequiredSignatures);
  if (!num_required_signatures) {
    return std::nullopt;
  }

  auto num_readonly_signed_accounts =
      GetUint8FromStringDict(value, kNumReadonlySignedAccounts);
  if (!num_readonly_signed_accounts) {
    return std::nullopt;
  }

  auto num_readonly_unsigned_accounts =
      GetUint8FromStringDict(value, kNumReadonlyUnsignedAccounts);
  if (!num_readonly_unsigned_accounts) {
    return std::nullopt;
  }

  return SolanaMessageHeader(*num_required_signatures,
                             *num_readonly_signed_accounts,
                             *num_readonly_unsigned_accounts);
}

mojom::SolanaMessageHeaderPtr SolanaMessageHeader::ToMojom() const {
  return mojom::SolanaMessageHeader::New(num_required_signatures,
                                         num_readonly_signed_accounts,
                                         num_readonly_unsigned_accounts);
}

// static
SolanaMessageHeader SolanaMessageHeader::FromMojom(
    const mojom::SolanaMessageHeaderPtr& mojom_msg_header) {
  return SolanaMessageHeader(mojom_msg_header->num_required_signatures,
                             mojom_msg_header->num_readonly_signed_accounts,
                             mojom_msg_header->num_readonly_unsigned_accounts);
}

}  // namespace brave_wallet
