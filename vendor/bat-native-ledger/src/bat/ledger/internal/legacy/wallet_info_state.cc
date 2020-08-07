/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/logging/logging.h"
#include "bat/ledger/internal/legacy/wallet_info_state.h"
#include "base/json/json_reader.h"
#include "base/base64.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kPaymentIdKey[] = "paymentId";
const char kAddressCardIdKey[] = "addressCARD_ID";
const char kKeyInfoSeedKey[] = "keyInfoSeed";

}  // namespace

WalletInfoState::WalletInfoState() = default;

WalletInfoState::~WalletInfoState() = default;

bool WalletInfoState::FromJson(
    const std::string& json,
    WalletInfoProperties* properties) const {
  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  auto json_value = base::JSONReader::Read(json);
  if (!json_value) {
    NOTREACHED();
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  json_value->GetAsDictionary(&dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  return FromDict(dictionary, properties);
}

bool WalletInfoState::FromDict(
    const base::DictionaryValue* dictionary,
    WalletInfoProperties* properties) const {
  DCHECK(dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  WalletInfoProperties wallet_info_properties;

  // Payment Id
  const auto* payment_id = dictionary->FindStringKey(kPaymentIdKey);
  if (!payment_id) {
    NOTREACHED();
    return false;
  }
  wallet_info_properties.payment_id = *payment_id;

  // Address Card Id
  const auto* address_card_id = dictionary->FindStringKey(kAddressCardIdKey);
  if (!address_card_id) {
    NOTREACHED();
    return false;
  }
  wallet_info_properties.address_card_id = *address_card_id;

  // Key Info Seed (Base64)
  const auto* base64_key_info_seed = dictionary->FindStringKey(kKeyInfoSeedKey);
  if (!base64_key_info_seed) {
    NOTREACHED();
    return false;
  }

  std::string key_info_seed;
  if (!base::Base64Decode(*base64_key_info_seed, &key_info_seed)) {
    NOTREACHED();
    return false;
  }
  wallet_info_properties.key_info_seed.assign(key_info_seed.begin(),
      key_info_seed.end());

  *properties = wallet_info_properties;

  return true;
}

bool WalletInfoState::ToJson(
    JsonWriter* writer,
    const WalletInfoProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kPaymentIdKey);
  writer->String(properties.payment_id.c_str());

  writer->String(kAddressCardIdKey);
  writer->String(properties.address_card_id.c_str());

  writer->String(kKeyInfoSeedKey);
  if (!properties.key_info_seed.empty()) {
    const std::string base64_encoded_key_info_seed =
        base::Base64Encode(properties.key_info_seed);
    writer->String(base64_encoded_key_info_seed.c_str());
  } else {
    writer->String("");
  }

  writer->EndObject();

  return true;
}

std::string WalletInfoState::ToJson(
    const WalletInfoProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
