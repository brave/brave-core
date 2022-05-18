/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"

#include <utility>

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/internal/server/headers/via_header_util.h"
#include "bat/ads/internal/server/hosts/server_host_util.h"

namespace ads {

RedeemUnblindedPaymentTokensUrlRequestBuilder::
    RedeemUnblindedPaymentTokensUrlRequestBuilder(
        const WalletInfo& wallet,
        const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
        const base::Value& user_data)
    : wallet_(wallet),
      unblinded_payment_tokens_(unblinded_payment_tokens),
      user_data_(user_data.Clone()) {
  DCHECK(wallet_.IsValid());
  DCHECK(!unblinded_payment_tokens_.empty());
}

RedeemUnblindedPaymentTokensUrlRequestBuilder::
    ~RedeemUnblindedPaymentTokensUrlRequestBuilder() = default;

// PUT /v2/confirmation/payment/{payment_id}

mojom::UrlRequestPtr RedeemUnblindedPaymentTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestPtr url_request = mojom::UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->headers = BuildHeaders();
  const std::string payload = CreatePayload();
  url_request->content = BuildBody(payload);
  url_request->content_type = "application/json";
  url_request->method = mojom::UrlRequestMethod::kPut;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL RedeemUnblindedPaymentTokensUrlRequestBuilder::BuildUrl() const {
  const std::string spec = base::StringPrintf(
      "%s/v2/confirmation/payment/%s", server::GetNonAnonymousHost().c_str(),
      wallet_.id.c_str());
  return GURL(spec);
}

std::vector<std::string>
RedeemUnblindedPaymentTokensUrlRequestBuilder::BuildHeaders() const {
  std::vector<std::string> headers;

  const std::string via_header = server::BuildViaHeader();
  headers.push_back(via_header);

  const std::string accept_header = "accept: application/json";
  headers.push_back(accept_header);

  return headers;
}

std::string RedeemUnblindedPaymentTokensUrlRequestBuilder::BuildBody(
    const std::string& payload) const {
  DCHECK(!payload.empty());

  base::DictionaryValue dictionary;

  base::Value payment_request_dto = CreatePaymentRequestDTO(payload);
  dictionary.SetKey("paymentCredentials", std::move(payment_request_dto));

  dictionary.SetStringKey("payload", payload);

  dictionary.MergeDictionary(&user_data_);

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

std::string RedeemUnblindedPaymentTokensUrlRequestBuilder::CreatePayload()
    const {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("paymentId", wallet_.id);

  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

base::Value
RedeemUnblindedPaymentTokensUrlRequestBuilder::CreatePaymentRequestDTO(
    const std::string& payload) const {
  DCHECK(!payload.empty());

  base::Value payment_request_dto(base::Value::Type::LIST);

  for (const auto& unblinded_payment_token : unblinded_payment_tokens_) {
    base::Value payment_credential(base::Value::Type::DICTIONARY);

    base::Value credential = CreateCredential(unblinded_payment_token, payload);
    payment_credential.SetKey("credential", base::Value(std::move(credential)));

    payment_credential.SetStringKey(
        "confirmationType",
        unblinded_payment_token.confirmation_type.ToString());

    const absl::optional<std::string> public_key_base64_optional =
        unblinded_payment_token.public_key.EncodeBase64();
    if (!public_key_base64_optional) {
      NOTREACHED();
    } else {
      payment_credential.SetStringKey("publicKey",
                                      public_key_base64_optional.value());
    }

    payment_request_dto.Append(std::move(payment_credential));
  }

  return payment_request_dto;
}

base::Value RedeemUnblindedPaymentTokensUrlRequestBuilder::CreateCredential(
    const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token,
    const std::string& payload) const {
  DCHECK(!payload.empty());

  base::Value credential(base::Value::Type::DICTIONARY);

  const absl::optional<privacy::cbr::VerificationKey>
      verification_key_optional =
          unblinded_payment_token.value.DeriveVerificationKey();
  if (!verification_key_optional) {
    NOTREACHED();
    return credential;
  }
  privacy::cbr::VerificationKey verification_key =
      verification_key_optional.value();

  const absl::optional<privacy::cbr::VerificationSignature>
      verification_signature_optional = verification_key.Sign(payload);
  if (!verification_signature_optional) {
    NOTREACHED();
    return credential;
  }
  const privacy::cbr::VerificationSignature& verification_signature =
      verification_signature_optional.value();

  const absl::optional<std::string> verification_signature_base64_optional =
      verification_signature.EncodeBase64();
  if (!verification_signature_base64_optional) {
    NOTREACHED();
    return credential;
  }

  const absl::optional<privacy::cbr::TokenPreimage> token_preimage_optional =
      unblinded_payment_token.value.GetTokenPreimage();
  if (!token_preimage_optional) {
    NOTREACHED();
    return credential;
  }
  const privacy::cbr::TokenPreimage& token_preimage =
      token_preimage_optional.value();

  const absl::optional<std::string> token_preimage_base64_optional =
      token_preimage.EncodeBase64();
  if (!token_preimage_base64_optional) {
    NOTREACHED();
    return credential;
  }

  credential.SetStringKey("signature",
                          verification_signature_base64_optional.value());
  credential.SetStringKey("t", token_preimage_base64_optional.value());

  return credential;
}

}  // namespace ads
