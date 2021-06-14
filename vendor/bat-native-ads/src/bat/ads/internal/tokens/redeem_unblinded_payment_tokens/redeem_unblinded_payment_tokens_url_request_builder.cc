/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/server/confirmations_server_util.h"
#include "bat/ads/internal/server/via_header_util.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::TokenPreimage;
using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;

RedeemUnblindedPaymentTokensUrlRequestBuilder::
    RedeemUnblindedPaymentTokensUrlRequestBuilder(
        const WalletInfo& wallet,
        const privacy::UnblindedTokenList& unblinded_tokens)
    : wallet_(wallet), unblinded_tokens_(unblinded_tokens) {
  DCHECK(wallet_.IsValid());
  DCHECK(!unblinded_tokens_.empty());
}

RedeemUnblindedPaymentTokensUrlRequestBuilder::
    ~RedeemUnblindedPaymentTokensUrlRequestBuilder() = default;

// PUT /v1/confirmation/payment/{payment_id}

UrlRequestPtr RedeemUnblindedPaymentTokensUrlRequestBuilder::Build() {
  UrlRequestPtr url_request = UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->headers = BuildHeaders();
  const std::string payload = CreatePayload();
  url_request->content = BuildBody(payload);
  url_request->content_type = "application/json";
  url_request->method = UrlRequestMethod::PUT;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string RedeemUnblindedPaymentTokensUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v1/confirmation/payment/%s",
                            confirmations::server::GetHost().c_str(),
                            wallet_.id.c_str());
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

  base::Value dictionary(base::Value::Type::DICTIONARY);

  base::Value payment_request_dto = CreatePaymentRequestDTO(payload);
  dictionary.SetKey("paymentCredentials", std::move(payment_request_dto));

  dictionary.SetKey("payload", base::Value(payload));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

std::string RedeemUnblindedPaymentTokensUrlRequestBuilder::CreatePayload()
    const {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetKey("paymentId", base::Value(wallet_.id));

  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

base::Value
RedeemUnblindedPaymentTokensUrlRequestBuilder::CreatePaymentRequestDTO(
    const std::string& payload) const {
  DCHECK(!payload.empty());

  base::Value payment_request_dto(base::Value::Type::LIST);

  for (const auto& unblinded_token : unblinded_tokens_) {
    base::Value payment_credential(base::Value::Type::DICTIONARY);

    base::Value credential = CreateCredential(unblinded_token, payload);
    payment_credential.SetKey("credential", base::Value(std::move(credential)));

    payment_credential.SetKey(
        "publicKey", base::Value(unblinded_token.public_key.encode_base64()));

    payment_request_dto.Append(std::move(payment_credential));
  }

  return payment_request_dto;
}

base::Value RedeemUnblindedPaymentTokensUrlRequestBuilder::CreateCredential(
    const privacy::UnblindedTokenInfo& unblinded_token,
    const std::string& payload) const {
  DCHECK(!payload.empty());

  base::Value credential(base::Value::Type::DICTIONARY);

  VerificationKey verification_key =
      unblinded_token.value.derive_verification_key();
  VerificationSignature verification_signature = verification_key.sign(payload);
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return credential;
  }

  const std::string verification_signature_base64 =
      verification_signature.encode_base64();
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return credential;
  }

  TokenPreimage token_preimage = unblinded_token.value.preimage();
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return credential;
  }

  const std::string token_preimage_base64 = token_preimage.encode_base64();
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return credential;
  }

  credential.SetKey("signature", base::Value(verification_signature_base64));
  credential.SetKey("t", base::Value(token_preimage_base64));

  return credential;
}

}  // namespace ads
