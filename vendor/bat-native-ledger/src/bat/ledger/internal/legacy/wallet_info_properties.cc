/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/wallet_info_properties.h"

#include "base/base64.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/notreached.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kPaymentIdKey[] = "paymentId";
const char kAddressCardIdKey[] = "addressCARD_ID";
const char kKeyInfoSeedKey[] = "keyInfoSeed";

}  // namespace

WalletInfoProperties::WalletInfoProperties() = default;

WalletInfoProperties::WalletInfoProperties(
    const WalletInfoProperties& properties) {
  payment_id = properties.payment_id;
  address_card_id = properties.address_card_id;
  key_info_seed = properties.key_info_seed;
}

WalletInfoProperties::~WalletInfoProperties() = default;

bool WalletInfoProperties::operator==(
    const WalletInfoProperties& rhs) const {
  return payment_id == rhs.payment_id &&
      address_card_id == rhs.address_card_id &&
      key_info_seed == rhs.key_info_seed;
}

bool WalletInfoProperties::operator!=(
    const WalletInfoProperties& rhs) const {
  return !(*this == rhs);
}

base::Value::Dict WalletInfoProperties::ToValue() const {
  base::Value::Dict dict;

  dict.Set(kPaymentIdKey, payment_id);
  dict.Set(kAddressCardIdKey, address_card_id);

  if (!key_info_seed.empty())
    dict.Set(kKeyInfoSeedKey, base::Base64Encode(key_info_seed));
  else
    dict.Set(kKeyInfoSeedKey, std::string());

  return dict;
}

bool WalletInfoProperties::FromValue(const base::Value::Dict& dict) {
  // Payment Id
  if (const auto* value = dict.FindString(kPaymentIdKey)) {
    payment_id = *value;
  } else {
    NOTREACHED();
    return false;
  }

  // Address Card Id
  if (const auto* value = dict.FindString(kAddressCardIdKey)) {
    address_card_id = *value;
  } else {
    NOTREACHED();
    return false;
  }

  // Key Info Seed (Base64)
  const auto* base64_key_info_seed = dict.FindString(kKeyInfoSeedKey);
  if (!base64_key_info_seed) {
    NOTREACHED();
    return false;
  }
  std::string plain_key_info_seed;
  if (!base::Base64Decode(*base64_key_info_seed, &plain_key_info_seed)) {
    NOTREACHED();
    return false;
  }
  key_info_seed.assign(plain_key_info_seed.begin(), plain_key_info_seed.end());

  return true;
}

std::string WalletInfoProperties::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool WalletInfoProperties::FromJson(const std::string& json) {
  auto document = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);

  if (!document.has_value()) {
    LOG(ERROR) << "Invalid wallet info property. json=" << json
               << ", error line=" << document.error().line
               << ", error column=" << document.error().column
               << ", error message=" << document.error().message;
    return false;
  }

  const base::Value::Dict* root = document->GetIfDict();
  if (!root) {
    LOG(ERROR) << "Invalid wallet info property. json=" << json;
    return false;
  }

  return FromValue(*root);
}

}  // namespace ledger
