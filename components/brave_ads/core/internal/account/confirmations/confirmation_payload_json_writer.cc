/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_payload_json_writer.h"

#include <utility>

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::json::writer {

namespace {

constexpr char kTransactionIdKey[] = "transactionId";
constexpr char kCreativeInstanceIdKey[] = "creativeInstanceId";
constexpr char kTypeKey[] = "type";
constexpr char kBlindedTokensKey[] = "blindedPaymentTokens";
constexpr char kPublicKeyKey[] = "publicKey";

void WriteOptedInConfirmationPayload(const ConfirmationInfo& confirmation,
                                     base::Value::Dict& dict) {
  CHECK(confirmation.opted_in);

  base::Value::List list;
  if (const absl::optional<std::string> value =
          confirmation.opted_in->blinded_token.EncodeBase64()) {
    list.Append(*value);
  }
  dict.Set(kBlindedTokensKey, std::move(list));

  if (const absl::optional<std::string> value =
          confirmation.opted_in->unblinded_token.public_key.EncodeBase64()) {
    dict.Set(kPublicKeyKey, *value);
  }

  dict.Merge(confirmation.opted_in->user_data.dynamic.Clone());

  dict.Merge(confirmation.opted_in->user_data.fixed.Clone());
}

}  // namespace

std::string WriteConfirmationPayload(const ConfirmationInfo& confirmation) {
  base::Value::Dict dict;

  dict.Set(kTransactionIdKey, confirmation.transaction_id);

  dict.Set(kCreativeInstanceIdKey, confirmation.creative_instance_id);

  dict.Set(kTypeKey, confirmation.type.ToString());

  if (confirmation.opted_in) {
    WriteOptedInConfirmationPayload(confirmation, dict);
  }

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

}  // namespace brave_ads::json::writer
