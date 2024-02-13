/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_account_meta.h"

#include <optional>
#include <utility>

namespace {

constexpr char kPubkey[] = "pubkey";
constexpr char kIsSigner[] = "is_signer";
constexpr char kIsWritable[] = "is_writable";
constexpr char kAddressTableLookupIndex[] = "address_table_lookup_index";

}  // namespace

namespace brave_wallet {

SolanaAccountMeta::SolanaAccountMeta(
    const std::string& pubkey,
    std::optional<uint8_t> address_table_lookup_index,
    bool is_signer,
    bool is_writable)
    : pubkey(pubkey),
      address_table_lookup_index(address_table_lookup_index),
      is_signer(is_signer),
      is_writable(is_writable) {}

SolanaAccountMeta::SolanaAccountMeta(const SolanaAccountMeta&) = default;

SolanaAccountMeta::~SolanaAccountMeta() = default;

bool SolanaAccountMeta::operator==(const SolanaAccountMeta& meta) const {
  return pubkey == meta.pubkey &&
         address_table_lookup_index == meta.address_table_lookup_index &&
         is_signer == meta.is_signer && is_writable == meta.is_writable;
}

mojom::SolanaAccountMetaPtr SolanaAccountMeta::ToMojomSolanaAccountMeta()
    const {
  mojom::OptionalUint8Ptr index = nullptr;
  if (address_table_lookup_index) {
    index = mojom::OptionalUint8::New(*address_table_lookup_index);
  }
  return mojom::SolanaAccountMeta::New(pubkey, std::move(index), is_signer,
                                       is_writable);
}

base::Value::Dict SolanaAccountMeta::ToValue() const {
  base::Value::Dict dict;
  dict.Set(kPubkey, pubkey);
  dict.Set(kIsSigner, is_signer);
  dict.Set(kIsWritable, is_writable);
  if (address_table_lookup_index) {
    dict.Set(kAddressTableLookupIndex,
             base::NumberToString(*address_table_lookup_index));
  }
  return dict;
}

// static
std::optional<SolanaAccountMeta> SolanaAccountMeta::FromValue(
    const base::Value::Dict& value) {
  const std::string* pubkey = value.FindString(kPubkey);
  if (!pubkey) {
    return std::nullopt;
  }
  std::optional<bool> is_signer = value.FindBool(kIsSigner);
  if (!is_signer) {
    return std::nullopt;
  }
  std::optional<bool> is_writable = value.FindBool(kIsWritable);
  if (!is_writable) {
    return std::nullopt;
  }

  std::optional<uint8_t> address_table_lookup_index;
  const std::string* address_table_lookup_index_string =
      value.FindString(kAddressTableLookupIndex);
  if (address_table_lookup_index_string) {
    unsigned val = 0;
    if (!base::StringToUint(*address_table_lookup_index_string, &val)) {
      return std::nullopt;
    }
    if (val > UINT8_MAX) {
      return std::nullopt;
    }
    address_table_lookup_index = static_cast<uint8_t>(val);
  }

  return SolanaAccountMeta(*pubkey, address_table_lookup_index, *is_signer,
                           *is_writable);
}

// static
void SolanaAccountMeta::FromMojomSolanaAccountMetas(
    const std::vector<mojom::SolanaAccountMetaPtr>& mojom_account_metas,
    std::vector<SolanaAccountMeta>* account_metas) {
  if (!account_metas) {
    return;
  }
  account_metas->clear();
  for (const auto& mojom_account_meta : mojom_account_metas) {
    std::optional<uint8_t> addr_table_lookup_index = std::nullopt;
    if (mojom_account_meta->addr_table_lookup_index) {
      addr_table_lookup_index =
          mojom_account_meta->addr_table_lookup_index->val;
    }
    account_metas->push_back(SolanaAccountMeta(
        mojom_account_meta->pubkey, addr_table_lookup_index,
        mojom_account_meta->is_signer, mojom_account_meta->is_writable));
  }
}

}  // namespace brave_wallet
