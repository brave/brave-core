/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/url_request_builders/redeem_payment_tokens_url_request_builder.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/credential_builder.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

std::vector<std::string> BuildHeaders() {
  return {"accept: application/json"};
}

}  // namespace

RedeemPaymentTokensUrlRequestBuilder::RedeemPaymentTokensUrlRequestBuilder(
    WalletInfo wallet,
    PaymentTokenList payment_tokens,
    base::Value::Dict user_data)
    : wallet_(std::move(wallet)),
      payment_tokens_(std::move(payment_tokens)),
      user_data_(std::move(user_data)) {
  CHECK(wallet_.IsValid());
  CHECK(!payment_tokens_.empty());
}

RedeemPaymentTokensUrlRequestBuilder::~RedeemPaymentTokensUrlRequestBuilder() =
    default;

mojom::UrlRequestInfoPtr RedeemPaymentTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr mojom_url_request = mojom::UrlRequestInfo::New();
  mojom_url_request->url = BuildUrl();
  mojom_url_request->headers = BuildHeaders();
  const std::string payload = BuildPayload();
  mojom_url_request->content = BuildBody(payload);
  mojom_url_request->content_type = "application/json";
  mojom_url_request->method = mojom::UrlRequestMethodType::kPut;

  return mojom_url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL RedeemPaymentTokensUrlRequestBuilder::BuildUrl() const {
  const std::string spec = base::ReplaceStringPlaceholders(
      "$1/v3/confirmation/payment/$2",
      {GetNonAnonymousUrlHost(), wallet_.payment_id}, nullptr);
  return GURL(spec);
}

std::string RedeemPaymentTokensUrlRequestBuilder::BuildBody(
    const std::string& payload) {
  CHECK(!payload.empty());
  CHECK(!user_data_.empty());

  auto dict = base::Value::Dict()
                  .Set("paymentCredentials", BuildPaymentRequestDTO(payload))
                  .Set("payload", payload);

  dict.Merge(std::move(user_data_));

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

std::string RedeemPaymentTokensUrlRequestBuilder::BuildPayload() const {
  std::string json;
  CHECK(base::JSONWriter::Write(
      base::Value::Dict().Set("paymentId", wallet_.payment_id), &json));
  return json;
}

base::Value::List RedeemPaymentTokensUrlRequestBuilder::BuildPaymentRequestDTO(
    const std::string& payload) const {
  CHECK(!payload.empty());

  base::Value::List list;

  for (const auto& payment_token : payment_tokens_) {
    std::optional<base::Value::Dict> credential =
        cbr::MaybeBuildCredential(payment_token.unblinded_token, payload);
    if (!credential) {
      continue;
    }

    const std::optional<std::string> public_key_base64 =
        payment_token.public_key.EncodeBase64();
    CHECK(public_key_base64);

    list.Append(
        base::Value::Dict()
            .Set("confirmationType", ToString(payment_token.confirmation_type))
            .Set("credential", std::move(*credential))
            .Set("publicKey", *public_key_base64));
  }

  return list;
}

}  // namespace brave_ads
