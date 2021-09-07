/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/ad_rewards/payments/payments_url_request_builder.h"

#include <cstdint>

#include "base/base64.h"
#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ads/internal/security/crypto_util.h"
#include "bat/ads/internal/server/confirmations_server_util.h"

namespace ads {

PaymentsUrlRequestBuilder::PaymentsUrlRequestBuilder(const WalletInfo& wallet)
    : wallet_(wallet) {
  DCHECK(wallet_.IsValid());
}

PaymentsUrlRequestBuilder::~PaymentsUrlRequestBuilder() = default;

// GET /v1/confirmation/payment/{payment_id}

mojom::UrlRequestPtr PaymentsUrlRequestBuilder::Build() {
  mojom::UrlRequestPtr url_request = mojom::UrlRequest::New();
  url_request->url = BuildUrl();
  const std::string body = BuildBody();
  url_request->headers = BuildHeaders(body);
  url_request->method = mojom::UrlRequestMethod::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string PaymentsUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v1/confirmation/payment/%s",
                            confirmations::server::GetHost().c_str(),
                            wallet_.id.c_str());
}

std::vector<std::string> PaymentsUrlRequestBuilder::BuildHeaders(
    const std::string& body) const {
  const std::string digest_header_value = BuildDigestHeaderValue(body);
  const std::string digest_header =
      base::StringPrintf("digest: %s", digest_header_value.c_str());

  const std::string signature_header_value = BuildSignatureHeaderValue(body);
  const std::string signature_header =
      base::StringPrintf("signature: %s", signature_header_value.c_str());

  const std::string accept_header = "accept: application/json";

  return {digest_header, signature_header, accept_header};
}

std::string PaymentsUrlRequestBuilder::BuildDigestHeaderValue(
    const std::string& body) const {
  DCHECK(!body.empty());

  const std::vector<uint8_t> body_sha256 = security::Sha256Hash(body);
  const std::string body_sha256_base64 = base::Base64Encode(body_sha256);

  return base::StringPrintf("SHA-256=%s", body_sha256_base64.c_str());
}

std::string PaymentsUrlRequestBuilder::BuildSignatureHeaderValue(
    const std::string& body) const {
  DCHECK(!body.empty());

  const std::string digest_header_value = BuildDigestHeaderValue(body);

  return security::Sign({{"digest", digest_header_value}}, "primary",
                        wallet_.secret_key);
}

std::string PaymentsUrlRequestBuilder::BuildBody() const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

}  // namespace ads
