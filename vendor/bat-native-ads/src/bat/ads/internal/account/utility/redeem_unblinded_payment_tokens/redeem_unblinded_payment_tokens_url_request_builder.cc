/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"

#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "bat/ads/internal/server/headers/via_header_util.h"
#include "bat/ads/internal/server/url/hosts/server_host_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {

namespace {

std::vector<std::string> BuildHeaders() {
  std::vector<std::string> headers;

  const std::string via_header = server::BuildViaHeader();
  headers.push_back(via_header);

  const std::string accept_header = "accept: application/json";
  headers.push_back(accept_header);

  return headers;
}

base::Value::Dict CreateCredential(
    const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token,
    const std::string& payload) {
  DCHECK(!payload.empty());

  base::Value::Dict credential;

  absl::optional<privacy::cbr::VerificationKey> verification_key =
      unblinded_payment_token.value.DeriveVerificationKey();
  if (!verification_key) {
    NOTREACHED();
    return credential;
  }

  const absl::optional<privacy::cbr::VerificationSignature>
      verification_signature = verification_key->Sign(payload);
  if (!verification_signature) {
    NOTREACHED();
    return credential;
  }

  const absl::optional<std::string> verification_signature_base64 =
      verification_signature->EncodeBase64();
  if (!verification_signature_base64) {
    NOTREACHED();
    return credential;
  }

  const absl::optional<privacy::cbr::TokenPreimage> token_preimage =
      unblinded_payment_token.value.GetTokenPreimage();
  if (!token_preimage) {
    NOTREACHED();
    return credential;
  }

  const absl::optional<std::string> token_preimage_base64 =
      token_preimage->EncodeBase64();
  if (!token_preimage_base64) {
    NOTREACHED();
    return credential;
  }

  credential.Set("signature", *verification_signature_base64);
  credential.Set("t", *token_preimage_base64);

  return credential;
}

}  // namespace

RedeemUnblindedPaymentTokensUrlRequestBuilder::
    RedeemUnblindedPaymentTokensUrlRequestBuilder(
        WalletInfo wallet,
        privacy::UnblindedPaymentTokenList unblinded_payment_tokens,
        base::Value::Dict user_data)
    : wallet_(std::move(wallet)),
      unblinded_payment_tokens_(std::move(unblinded_payment_tokens)),
      user_data_(std::move(user_data)) {
  DCHECK(wallet_.IsValid());
  DCHECK(!unblinded_payment_tokens_.empty());
}

RedeemUnblindedPaymentTokensUrlRequestBuilder::
    ~RedeemUnblindedPaymentTokensUrlRequestBuilder() = default;

// PUT /v3/confirmation/payment/{paymentId}

mojom::UrlRequestInfoPtr
RedeemUnblindedPaymentTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  url_request->headers = BuildHeaders();
  const std::string payload = CreatePayload();
  url_request->content = BuildBody(payload);
  url_request->content_type = "application/json";
  url_request->method = mojom::UrlRequestMethodType::kPut;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL RedeemUnblindedPaymentTokensUrlRequestBuilder::BuildUrl() const {
  const std::string spec = base::StringPrintf(
      "%s/v3/confirmation/payment/%s", server::GetNonAnonymousHost().c_str(),
      wallet_.payment_id.c_str());
  return GURL(spec);
}

std::string RedeemUnblindedPaymentTokensUrlRequestBuilder::BuildBody(
    const std::string& payload) {
  DCHECK(!payload.empty());
  DCHECK(!user_data_.empty());

  base::Value::Dict dict;

  base::Value::List payment_request_dto = CreatePaymentRequestDTO(payload);
  dict.Set("paymentCredentials", std::move(payment_request_dto));
  dict.Set("payload", payload);

  dict.Merge(std::move(user_data_));

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

std::string RedeemUnblindedPaymentTokensUrlRequestBuilder::CreatePayload()
    const {
  base::Value::Dict payload;
  payload.Set("paymentId", wallet_.payment_id);

  std::string json;
  CHECK(base::JSONWriter::Write(payload, &json));
  return json;
}

base::Value::List
RedeemUnblindedPaymentTokensUrlRequestBuilder::CreatePaymentRequestDTO(
    const std::string& payload) const {
  DCHECK(!payload.empty());

  base::Value::List payment_request_dto;

  for (const auto& unblinded_payment_token : unblinded_payment_tokens_) {
    base::Value::Dict payment_credential;

    base::Value::Dict credential =
        CreateCredential(unblinded_payment_token, payload);
    payment_credential.Set("credential", base::Value(std::move(credential)));

    payment_credential.Set(
        "confirmationType",
        unblinded_payment_token.confirmation_type.ToString());

    const absl::optional<std::string> public_key_base64 =
        unblinded_payment_token.public_key.EncodeBase64();
    if (!public_key_base64) {
      NOTREACHED();
    } else {
      payment_credential.Set("publicKey", *public_key_base64);
    }

    payment_request_dto.Append(std::move(payment_credential));
  }

  return payment_request_dto;
}

}  // namespace ads
