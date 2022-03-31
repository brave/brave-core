/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/redeem_unblinded_token/create_confirmation_util.h"

#include <utility>

#include "base/base64url.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::TokenPreimage;
using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;

std::string CreateConfirmationRequestDTO(const ConfirmationInfo& confirmation) {
  base::Value dto(base::Value::Type::DICTIONARY);

  dto.SetStringKey("creativeInstanceId", confirmation.creative_instance_id);

  dto.SetKey("payload", base::Value(base::Value::Type::DICTIONARY));

  const std::string blinded_payment_token_base64 =
      confirmation.blinded_payment_token.encode_base64();
  if (!blinded_payment_token_base64.empty()) {
    base::Value list(base::Value::Type::LIST);

    list.Append(blinded_payment_token_base64);

    dto.SetKey("blindedPaymentTokens", std::move(list));
  }

  dto.SetStringKey("type", confirmation.type.ToString());

  const std::string public_key =
      confirmation.unblinded_token.public_key.encode_base64();
  dto.SetStringKey("publicKey", public_key);

  absl::optional<base::Value> user_data =
      base::JSONReader::Read(confirmation.user_data);
  if (user_data && user_data->is_dict()) {
    base::DictionaryValue* user_data_dictionary = nullptr;
    if (user_data->GetAsDictionary(&user_data_dictionary)) {
      dto.MergeDictionary(user_data_dictionary);
    }
  }

  std::string json;
  base::JSONWriter::Write(dto, &json);

  return json;
}

std::string CreateCredential(const privacy::UnblindedTokenInfo& unblinded_token,
                             const std::string& payload) {
  DCHECK(!payload.empty());

  VerificationKey verification_key =
      unblinded_token.value.derive_verification_key();
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  VerificationSignature verification_signature = verification_key.sign(payload);
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  const std::string verification_signature_base64 =
      verification_signature.encode_base64();
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  TokenPreimage token_preimage = unblinded_token.value.preimage();
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  const std::string token_preimage_base64 = token_preimage.encode_base64();
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  base::Value dictionary(base::Value::Type::DICTIONARY);

  dictionary.SetStringKey("payload", payload);
  dictionary.SetStringKey("signature", verification_signature_base64);
  dictionary.SetStringKey("t", token_preimage_base64);

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  std::string credential_base64url;
  base::Base64UrlEncode(json, base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &credential_base64url);

  return credential_base64url;
}

}  // namespace ads
