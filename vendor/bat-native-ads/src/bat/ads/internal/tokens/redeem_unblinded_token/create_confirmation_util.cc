/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"

#include "base/base64url.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "wrapper.hpp"
#include "bat/ads/ads.h"
#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/locale/country_code_util.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"

namespace ads {

using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;
using challenge_bypass_ristretto::TokenPreimage;

std::string CreateConfirmationRequestDTO(
    const ConfirmationInfo& confirmation) {
  base::Value payload(base::Value::Type::DICTIONARY);

  payload.SetKey("creativeInstanceId",
      base::Value(confirmation.creative_instance_id));

  payload.SetKey("payload", base::Value(base::Value::Type::DICTIONARY));

  const std::string blinded_payment_token_base64 =
      confirmation.blinded_payment_token.encode_base64();
  payload.SetKey("blindedPaymentToken",
      base::Value(blinded_payment_token_base64));

  const std::string type = std::string(confirmation.type);
  payload.SetKey("type", base::Value(type));

  DCHECK(!_build_channel.name.empty());
  payload.SetKey("buildChannel", base::Value(_build_channel.name));

  if (_build_channel.is_release) {
    const std::string locale =
        brave_l10n::LocaleHelper::GetInstance()->GetLocale();

    if (locale::HasLargeAnonymity(locale)) {
      const std::string country_code = brave_l10n::GetCountryCode(locale);
      payload.SetKey("countryCode", base::Value(country_code));
    } else {
      if (locale::IsAnonymous(locale)) {
        payload.SetKey("countryCode", base::Value("??"));
      }
    }
  }

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();
  payload.SetKey("platform", base::Value(platform));

  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

std::string CreateCredential(
    const privacy::UnblindedTokenInfo& unblinded_token,
    const std::string& payload) {
  DCHECK(!payload.empty());

  base::Value dictionary(base::Value::Type::DICTIONARY);

  dictionary.SetKey("payload", base::Value(payload));

  VerificationKey verification_key =
      unblinded_token.value.derive_verification_key();
  VerificationSignature verification_signature = verification_key.sign(payload);
  const std::string verification_signature_base64 =
      verification_signature.encode_base64();
  dictionary.SetKey("signature", base::Value(verification_signature_base64));

  TokenPreimage token_preimage = unblinded_token.value.preimage();
  const std::string token_preimage_base64 = token_preimage.encode_base64();
  dictionary.SetKey("t", base::Value(token_preimage_base64));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  std::string credential_base64url;
  base::Base64UrlEncode(json, base::Base64UrlEncodePolicy::INCLUDE_PADDING,
      &credential_base64url);

  return credential_base64url;
}

}  // namespace ads
