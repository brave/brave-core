/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include <stdint.h>
#include <charconv>
#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {

std::string Uint256ValueToString(uint256_t value) {
  // 80 as max length of uint256 string
  std::array<char, 80> buffer{0};
  auto [p, ec] =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  if (ec != std::errc())
    return std::string();
  std::string response;
  response.assign(buffer.begin(), p);
  return response;
}

}  // namespace

FilTransaction::FilTransaction() : gas_limit_(0) {}

FilTransaction::FilTransaction(const FilTransaction&) = default;
FilTransaction::FilTransaction(absl::optional<uint64_t> nonce,
                               const std::string& gas_premium,
                               const std::string& gas_fee_cap,
                               const std::string& max_fee,
                               uint64_t gas_limit,
                               const FilAddress& to,
                               const std::string& value,
                               const std::string& cid)
    : nonce_(nonce),
      gas_premium_(gas_premium),
      gas_fee_cap_(gas_fee_cap),
      max_fee_(max_fee),
      gas_limit_(gas_limit),
      cid_(cid),
      to_(to),
      value_(value) {}
FilTransaction::~FilTransaction() = default;

bool FilTransaction::operator==(const FilTransaction& tx) const {
  return nonce_ == tx.nonce_ && gas_premium_ == tx.gas_premium_ &&
         gas_fee_cap_ == tx.gas_fee_cap_ && gas_limit_ == tx.gas_limit_ &&
         to_ == tx.to_ && value_ == tx.value_ && cid_ == tx.cid_;
}

// static
absl::optional<FilTransaction> FilTransaction::FromTxData(
    const mojom::FilTxDataPtr& tx_data) {
  FilTransaction tx;
  uint64_t nonce = 0;
  if (!tx_data->nonce.empty() && base::StringToUint64(tx_data->nonce, &nonce)) {
    tx.nonce_ = nonce;
  }
  tx.set_nonce(nonce);
  tx.to_ = FilAddress::FromAddress(tx_data->to);

  uint256_t value = 0;
  HexValueToUint256(tx_data->value, &value);
  tx.set_value(Uint256ValueToString(value));
  tx.set_fee_cap(tx_data->gas_fee_cap);
  tx.set_gas_premium(tx_data->gas_premium);
  tx.set_max_fee(tx_data->max_fee);
  tx.set_cid(tx_data->cid);
  uint64_t gas_limit = 0;
  if (!tx_data->gas_limit.empty() &&
      base::StringToUint64(tx_data->gas_limit, &gas_limit)) {
    tx.set_gas_limit(gas_limit);
  }
  return tx;
}

base::Value FilTransaction::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("nonce",
                    nonce_ ? base::NumberToString(nonce_.value()) : "");
  dict.SetStringKey("gas_premium", gas_premium_);
  dict.SetStringKey("gas_fee_cap", gas_fee_cap_);
  dict.SetStringKey("max_fee", max_fee_);
  dict.SetStringKey("gas_limit", base::NumberToString(gas_limit_));
  dict.SetStringKey("to", to_.ToChecksumAddress());
  dict.SetStringKey("value", value_);
  base::Value cid(base::Value::Type::DICTIONARY);
  cid.SetStringKey("/", cid_);
  dict.SetKey("CID", std::move(cid));
  return dict;
}

// static
absl::optional<FilTransaction> FilTransaction::FromValue(
    const base::Value& value) {
  FilTransaction tx;
  const std::string* nonce = value.FindStringKey("nonce");
  if (!nonce)
    return absl::nullopt;

  if (!nonce->empty()) {
    uint64_t nonce_uint;
    if (!base::StringToUint64(*nonce, &nonce_uint))
      return absl::nullopt;
    tx.nonce_ = nonce_uint;
  }

  const std::string* gas_premium = value.FindStringKey("gas_premium");
  if (!gas_premium)
    return absl::nullopt;
  tx.gas_premium_ = *gas_premium;

  const std::string* gas_fee_cap = value.FindStringKey("gas_fee_cap");
  if (!gas_fee_cap)
    return absl::nullopt;
  tx.gas_fee_cap_ = *gas_fee_cap;

  const std::string* gas_limit = value.FindStringKey("gas_limit");
  if (!gas_limit || !base::StringToUint64(*gas_limit, &tx.gas_limit_))
    return absl::nullopt;

  const std::string* to = value.FindStringKey("to");
  if (!to)
    return absl::nullopt;
  tx.to_ = FilAddress::FromAddress(*to);

  const std::string* tx_value = value.FindStringKey("value");
  if (!tx_value)
    return absl::nullopt;
  tx.value_ = *tx_value;

  const std::string* cid = value.FindStringKey("cid");
  if (cid) {
    tx.cid_ = *cid;
  }

  return tx;
}

std::string FilTransaction::GetMessageToSign() const {
  std::string json;
  base::JSONWriter::WriteWithOptions(
      ToValue(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
  return json;
}

}  // namespace brave_wallet
