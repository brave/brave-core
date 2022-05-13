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
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
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

  base::Value blinded_payment_token_list(base::Value::Type::LIST);
  for (const auto& blinded_token : confirmation.blinded_tokens) {
    const std::string& blinded_token_base64 = blinded_token.encode_base64();
    if (blinded_token_base64.empty()) {
      continue;
    }

    blinded_payment_token_list.Append(blinded_token_base64);
  }
  dto.SetKey("blindedPaymentTokens", std::move(blinded_payment_token_list));

  const absl::optional<double> smallest_denomination_optional =
      GetSmallestNonZeroDenominationForIssuerType(IssuerType::kPayments);
  if (smallest_denomination_optional) {
    const double smallest_denomination = smallest_denomination_optional.value();
    dto.SetStringKey("lowestValue",
                     base::NumberToString(smallest_denomination));
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
  if (privacy::cbr::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  VerificationSignature verification_signature = verification_key.sign(payload);
  if (privacy::cbr::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  const std::string verification_signature_base64 =
      verification_signature.encode_base64();
  if (privacy::cbr::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  TokenPreimage token_preimage = unblinded_token.value.preimage();
  if (privacy::cbr::ExceptionOccurred()) {
    NOTREACHED();
    return "";
  }

  const std::string token_preimage_base64 = token_preimage.encode_base64();
  if (privacy::cbr::ExceptionOccurred()) {
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
