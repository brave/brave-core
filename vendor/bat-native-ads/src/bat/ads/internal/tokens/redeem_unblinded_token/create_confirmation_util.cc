/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"

#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/guid.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "wrapper.hpp"
#include "bat/ads/ads.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/features/features.h"
#include "bat/ads/internal/locale/country_code_util.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/privacy/privacy_util.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"

namespace ads {

using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;
using challenge_bypass_ristretto::TokenPreimage;

ConfirmationInfo CreateConfirmationInfo(
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type,
    const privacy::UnblindedTokenInfo& unblinded_token) {
  DCHECK(!creative_instance_id.empty());

  ConfirmationInfo confirmation;

  confirmation.id = base::GenerateGUID();
  confirmation.creative_instance_id = creative_instance_id;
  confirmation.type = confirmation_type;
  confirmation.unblinded_token = unblinded_token;

  const std::vector<Token> tokens = privacy::GenerateTokens(1);
  confirmation.payment_token = tokens.front();

  const std::vector<BlindedToken> blinded_tokens = privacy::BlindTokens(tokens);
  const BlindedToken blinded_token = blinded_tokens.front();
  confirmation.blinded_payment_token = blinded_token;

  const std::string payload = CreateConfirmationRequestDTO(confirmation);
  confirmation.credential = CreateCredential(unblinded_token, payload);

  confirmation.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());

  return confirmation;
}

std::string CreateConfirmationRequestDTO(
    const ConfirmationInfo& confirmation) {
  base::Value dto(base::Value::Type::DICTIONARY);

  dto.SetKey("creativeInstanceId",
      base::Value(confirmation.creative_instance_id));

  dto.SetKey("payload", base::Value(base::Value::Type::DICTIONARY));

  const std::string blinded_payment_token_base64 =
      confirmation.blinded_payment_token.encode_base64();
  dto.SetKey("blindedPaymentToken",
      base::Value(blinded_payment_token_base64));

  const std::string type = std::string(confirmation.type);
  dto.SetKey("type", base::Value(type));

  DCHECK(!_build_channel.name.empty());
  dto.SetKey("buildChannel", base::Value(_build_channel.name));

  if (_build_channel.is_release) {
    const std::string locale =
        brave_l10n::LocaleHelper::GetInstance()->GetLocale();

    if (locale::IsMemberOfAnonymitySet(locale)) {
      const std::string country_code = brave_l10n::GetCountryCode(locale);
      dto.SetKey("countryCode", base::Value(country_code));
    } else {
      if (locale::ShouldClassifyAsOther(locale)) {
        dto.SetKey("countryCode", base::Value("??"));
      }
    }
  }

  if (!features::IsPageProbabilitiesStudyActive()) {
    dto.SetKey("experiment", base::Value(base::Value::Type::DICTIONARY));
  } else {
    std::string study = features::GetPageProbabilitiesStudy();
    std::string group = features::GetPageProbabilitiesFieldTrialGroup();
    std::string history_size =
        base::NumberToString(features::GetPageProbabilitiesHistorySize());
    base::Value dictionary(base::Value::Type::DICTIONARY);
    dictionary.SetKey("name", base::Value(study));
    dictionary.SetKey("group", base::Value(group));
    dictionary.SetKey("value", base::Value(history_size));
    dto.SetKey("experiment", std::move(dictionary));
  }

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();
  dto.SetKey("platform", base::Value(platform));

  std::string json;
  base::JSONWriter::Write(dto, &json);

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
