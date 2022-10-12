/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/client_properties.h"

#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "bat/ledger/internal/constants.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAutoContributeKey[] = "auto_contribute";
const char kBootTimestampKey[] = "bootStamp";
const char kFeeAmountKey[] = "fee_amount";
const char kInlineTipsKey[] = "inlineTip";
const char kReconcileTimestampKey[] = "reconcileStamp";
const char kRewardsEnabledKey[] = "rewards_enabled";
const char kUserChangedFeeKey[] = "user_changed_fee";
const char kWalletInfoKey[] = "walletInfo";

}  // namespace

ClientProperties::ClientProperties()
    : boot_timestamp(0),
      reconcile_timestamp(0),
      fee_amount(0),
      user_changed_fee(false),
      auto_contribute(false),
      rewards_enabled(false) {}

ClientProperties::ClientProperties(
    const ClientProperties& properties) {
  wallet_info = properties.wallet_info;
  boot_timestamp = properties.boot_timestamp;
  reconcile_timestamp = properties.reconcile_timestamp;
  fee_amount = properties.fee_amount;
  user_changed_fee = properties.user_changed_fee;
  auto_contribute = properties.auto_contribute;
  rewards_enabled = properties.rewards_enabled;
  inline_tips = properties.inline_tips;
}

ClientProperties::~ClientProperties() = default;

bool ClientProperties::operator==(
    const ClientProperties& rhs) const {
  return wallet_info == rhs.wallet_info &&
      boot_timestamp == rhs.boot_timestamp &&
      reconcile_timestamp == rhs.reconcile_timestamp &&
      fee_amount == rhs.fee_amount &&
      user_changed_fee == rhs.user_changed_fee &&
      auto_contribute == rhs.auto_contribute &&
      rewards_enabled == rhs.rewards_enabled &&
      inline_tips == rhs.inline_tips;
}

bool ClientProperties::operator!=(
    const ClientProperties& rhs) const {
  return !(*this == rhs);
}

base::Value::Dict ClientProperties::ToValue() const {
  base::Value::Dict dict;
  dict.Set(kWalletInfoKey, wallet_info.ToValue());
  dict.Set(kBootTimestampKey, base::Int64ToValue(boot_timestamp));
  dict.Set(kReconcileTimestampKey, base::Int64ToValue(reconcile_timestamp));
  dict.Set(kFeeAmountKey, fee_amount);
  dict.Set(kUserChangedFeeKey, user_changed_fee);
  dict.Set(kRewardsEnabledKey, rewards_enabled);
  dict.Set(kAutoContributeKey, auto_contribute);

  base::Value::Dict inline_tips_dict;
  for (const auto& [key, value] : inline_tips) {
    inline_tips_dict.Set(key, value);
  }
  dict.Set(kInlineTipsKey, std::move(inline_tips_dict));

  return dict;
}

bool ClientProperties::FromValue(const base::Value::Dict& dict) {
  // Wallet Info
  const auto* wallet_info_dict = dict.FindDict(kWalletInfoKey);
  if (!wallet_info_dict) {
    NOTREACHED();
    return false;
  }
  if (!wallet_info.FromValue(*wallet_info_dict)) {
    NOTREACHED();
    return false;
  }

  // Boot Timestamp (This timestamps used to be saved as uint64_t, and read back
  // as a double, because `base::Value` doesn't support int64 types, according
  // to the JS spec. Since then, the value is now transported as a string and
  // then converted to an int64_t. In case that fails, there's a fallback to old
  // double conversion, for backwards compatibility.)
  if (auto boot_timestamp_value_int64 =
          base::ValueToInt64(dict.Find(kBootTimestampKey))) {
    boot_timestamp = *boot_timestamp_value_int64;
  } else if (auto boot_timestamp_value_double =
                 dict.FindDouble(kBootTimestampKey)) {
    boot_timestamp = static_cast<uint64_t>(*boot_timestamp_value_double);
  } else {
    NOTREACHED();
    return false;
  }

  // Reconcile Timestamp (This timestamps used to be saved as uint64_t, and read
  // back as a double, because `base::Value` doesn't support int64 types,
  // according to the JS spec. Since then, the value is now transported as a
  // string and then converted to an int64_t. In case that fails, there's a
  // fallback to old double conversion, for backwards compatibility.)
  if (auto reconcile_timestamp_value_int64 =
          base::ValueToInt64(dict.Find(kReconcileTimestampKey))) {
    reconcile_timestamp = *reconcile_timestamp_value_int64;
  } else if (auto reconcile_timestamp_value_double =
                 dict.FindDouble(kReconcileTimestampKey)) {
    reconcile_timestamp =
        static_cast<uint64_t>(*reconcile_timestamp_value_double);
  } else {
    NOTREACHED();
    return false;
  }

  // Fee Amount
  if (auto value = dict.FindDouble(kFeeAmountKey)) {
    fee_amount = *value;
  } else {
    NOTREACHED();
    return false;
  }

  // User Changed Fee
  if (auto value = dict.FindBool(kUserChangedFeeKey)) {
    user_changed_fee = *value;
  } else {
    NOTREACHED();
    return false;
  }

  // Auto Contribute
  if (auto value = dict.FindBool(kAutoContributeKey)) {
    auto_contribute = *value;
  } else {
    NOTREACHED();
    return false;
  }

  // Rewards Enabled
  if (auto value = dict.FindBool(kRewardsEnabledKey)) {
    rewards_enabled = *value;
  } else {
    NOTREACHED();
    return false;
  }

  // Inline Tips
  if (const auto* value = dict.FindDict(kInlineTipsKey)) {
    for (const auto [inline_tips_key, inline_tips_value] : *value) {
      inline_tips.emplace(inline_tips_key, inline_tips_value.GetBool());
    }
  } else {
    NOTREACHED();
    return false;
  }

  return true;
}

std::string ClientProperties::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool ClientProperties::FromJson(const std::string& json) {
  auto document = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);

  if (!document.has_value()) {
    LOG(ERROR) << "Invalid client property. json=" << json
               << ", error line=" << document.error().line
               << ", error column=" << document.error().column
               << ", error message=" << document.error().message;
    return false;
  }

  const base::Value::Dict* root = document->GetIfDict();
  if (!root) {
    LOG(ERROR) << "Invalid client property. json=" << json;
    return false;
  }

  return FromValue(*root);
}

}  // namespace ledger
