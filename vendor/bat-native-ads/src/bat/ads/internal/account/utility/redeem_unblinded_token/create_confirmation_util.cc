/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/create_confirmation_util.h"

#include <utility>

#include "absl/types/optional.h"
#include "base/base64url.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/values.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace ads {

std::string CreateConfirmationRequestDTO(const ConfirmationInfo& confirmation) {
  base::Value::Dict confirmation_request_dto;

  confirmation_request_dto.Set("creativeInstanceId",
                               confirmation.creative_instance_id);

  confirmation_request_dto.Set("transactionId", confirmation.transaction_id);

  confirmation_request_dto.Set("payload", base::Value::Dict());

  base::Value::List blinded_payment_tokens;
  if (const auto value = confirmation.blinded_payment_token.EncodeBase64()) {
    blinded_payment_tokens.Append(*value);
  }
  confirmation_request_dto.Set("blindedPaymentTokens",
                               std::move(blinded_payment_tokens));

  confirmation_request_dto.Set("type", confirmation.type.ToString());

  if (const auto value =
          confirmation.unblinded_token.public_key.EncodeBase64()) {
    confirmation_request_dto.Set("publicKey", *value);
  }

  absl::optional<base::Value> user_data =
      base::JSONReader::Read(confirmation.user_data);
  if (user_data && user_data->is_dict()) {
    confirmation_request_dto.Merge(std::move(user_data->GetDict()));
  }

  std::string json;
  base::JSONWriter::Write(confirmation_request_dto, &json);

  return json;
}

std::string CreateCredential(const privacy::UnblindedTokenInfo& unblinded_token,
                             const std::string& payload) {
  DCHECK(!payload.empty());

  absl::optional<privacy::cbr::VerificationKey> verification_key =
      unblinded_token.value.DeriveVerificationKey();
  if (!verification_key) {
    NOTREACHED();
    return "";
  }

  const absl::optional<privacy::cbr::VerificationSignature>
      verification_signature = verification_key->Sign(payload);
  if (!verification_signature) {
    NOTREACHED();
    return "";
  }

  const absl::optional<std::string> verification_signature_base64 =
      verification_signature->EncodeBase64();
  if (!verification_signature_base64) {
    NOTREACHED();
    return "";
  }

  const absl::optional<privacy::cbr::TokenPreimage> token_preimage =
      unblinded_token.value.GetTokenPreimage();
  if (!token_preimage) {
    NOTREACHED();
    return "";
  }

  const absl::optional<std::string> token_preimage_base64 =
      token_preimage->EncodeBase64();
  if (!token_preimage_base64) {
    NOTREACHED();
    return "";
  }

  base::Value::Dict dict;

  dict.Set("payload", payload);
  dict.Set("signature", *verification_signature_base64);
  dict.Set("t", *token_preimage_base64);

  std::string json;
  base::JSONWriter::Write(dict, &json);

  std::string credential_base64url;
  base::Base64UrlEncode(json, base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &credential_base64url);

  return credential_base64url;
}

}  // namespace ads
