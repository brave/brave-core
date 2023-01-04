/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmation_payload_json_writer.h"

#include <utility>

#include "absl/types/optional.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace ads::json::writer {

namespace {

constexpr char kTransactionIdKey[] = "transactionId";
constexpr char kCreativeInstanceIdKey[] = "creativeInstanceId";
constexpr char kTypeKey[] = "type";
constexpr char kBlindedTokensKey[] = "blindedPaymentTokens";
constexpr char kPublicKeyKey[] = "publicKey";

}  // namespace

std::string WriteConfirmationPayload(const ConfirmationInfo& confirmation) {
  base::Value::Dict payload;

  payload.Set(kTransactionIdKey, confirmation.transaction_id);

  payload.Set(kCreativeInstanceIdKey, confirmation.creative_instance_id);

  payload.Set(kTypeKey, confirmation.type.ToString());

  if (confirmation.opted_in) {
    base::Value::List blinded_tokens;
    if (const auto value =
            confirmation.opted_in->blinded_token.EncodeBase64()) {
      blinded_tokens.Append(*value);
    }
    payload.Set(kBlindedTokensKey, std::move(blinded_tokens));

    if (const auto value =
            confirmation.opted_in->unblinded_token.public_key.EncodeBase64()) {
      payload.Set(kPublicKeyKey, *value);
    }

    payload.Merge(confirmation.opted_in->user_data.Clone());
  }

  std::string json;
  CHECK(base::JSONWriter::Write(payload, &json));
  return json;
}

}  // namespace ads::json::writer
