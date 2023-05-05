/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

std::vector<std::string> BuildHeaders() {
  std::vector<std::string> headers;

  const std::string accept_header = "accept: application/json";
  headers.push_back(accept_header);

  return headers;
}

base::Value::Dict CreateCredential(
    const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token,
    const std::string& payload) {
  DCHECK(!payload.empty());

  absl::optional<privacy::cbr::VerificationKey> verification_key =
      unblinded_payment_token.value.DeriveVerificationKey();
  if (!verification_key) {
    NOTREACHED();
    return {};
  }

  const absl::optional<privacy::cbr::VerificationSignature>
      verification_signature = verification_key->Sign(payload);
  if (!verification_signature) {
    NOTREACHED();
    return {};
  }

  const absl::optional<std::string> verification_signature_base64 =
      verification_signature->EncodeBase64();
  if (!verification_signature_base64) {
    NOTREACHED();
    return {};
  }

  const absl::optional<privacy::cbr::TokenPreimage> token_preimage =
      unblinded_payment_token.value.GetTokenPreimage();
  if (!token_preimage) {
    NOTREACHED();
    return {};
  }

  const absl::optional<std::string> token_preimage_base64 =
      token_preimage->EncodeBase64();
  if (!token_preimage_base64) {
    NOTREACHED();
    return {};
  }

  base::Value::Dict dict;
  dict.Set("signature", *verification_signature_base64);
  dict.Set("t", *token_preimage_base64);

  return dict;
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
  const std::string spec = base::ReplaceStringPlaceholders(
      "$1/v3/confirmation/payment/$2",
      {GetNonAnonymousUrlHost(), wallet_.payment_id}, nullptr);
  return GURL(spec);
}

std::string RedeemUnblindedPaymentTokensUrlRequestBuilder::BuildBody(
    const std::string& payload) {
  DCHECK(!payload.empty());
  DCHECK(!user_data_.empty());

  base::Value::Dict dict;

  base::Value::List list = CreatePaymentRequestDTO(payload);
  dict.Set("paymentCredentials", std::move(list));
  dict.Set("payload", payload);

  dict.Merge(std::move(user_data_));

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

std::string RedeemUnblindedPaymentTokensUrlRequestBuilder::CreatePayload()
    const {
  base::Value::Dict dict;
  dict.Set("paymentId", wallet_.payment_id);

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

base::Value::List
RedeemUnblindedPaymentTokensUrlRequestBuilder::CreatePaymentRequestDTO(
    const std::string& payload) const {
  DCHECK(!payload.empty());

  base::Value::List list;

  for (const auto& unblinded_payment_token : unblinded_payment_tokens_) {
    base::Value::Dict dict;

    dict.Set("credential",
             base::Value(CreateCredential(unblinded_payment_token, payload)));

    dict.Set("confirmationType",
             unblinded_payment_token.confirmation_type.ToString());

    const absl::optional<std::string> public_key_base64 =
        unblinded_payment_token.public_key.EncodeBase64();
    if (!public_key_base64) {
      NOTREACHED();
    } else {
      dict.Set("publicKey", *public_key_base64);
    }

    list.Append(std::move(dict));
  }

  return list;
}

}  // namespace brave_ads
