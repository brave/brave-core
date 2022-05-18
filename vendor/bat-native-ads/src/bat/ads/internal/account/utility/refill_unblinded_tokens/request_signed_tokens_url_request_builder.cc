/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/request_signed_tokens_url_request_builder.h"

#include <cstdint>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ads/internal/base/crypto_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/server/headers/via_header_util.h"
#include "bat/ads/internal/server/hosts/server_host_util.h"

namespace ads {

RequestSignedTokensUrlRequestBuilder::RequestSignedTokensUrlRequestBuilder(
    const WalletInfo& wallet,
    const std::vector<privacy::cbr::BlindedToken>& blinded_tokens)
    : wallet_(wallet), blinded_tokens_(blinded_tokens) {
  DCHECK(wallet_.IsValid());
  DCHECK(!blinded_tokens_.empty());
}

RequestSignedTokensUrlRequestBuilder::~RequestSignedTokensUrlRequestBuilder() =
    default;

// POST /v2/confirmation/token/{payment_id}

mojom::UrlRequestPtr RequestSignedTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestPtr url_request = mojom::UrlRequest::New();
  url_request->url = BuildUrl();
  const std::string body = BuildBody();
  url_request->headers = BuildHeaders(body);
  url_request->content = body;
  url_request->content_type = "application/json";
  url_request->method = mojom::UrlRequestMethod::kPost;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL RequestSignedTokensUrlRequestBuilder::BuildUrl() const {
  const std::string spec = base::StringPrintf(
      "%s/v2/confirmation/token/%s", server::GetNonAnonymousHost().c_str(),
      wallet_.id.c_str());
  return GURL(spec);
}

std::vector<std::string> RequestSignedTokensUrlRequestBuilder::BuildHeaders(
    const std::string& body) const {
  std::vector<std::string> headers;

  const std::string digest_header_value = BuildDigestHeaderValue(body);
  const std::string digest_header =
      base::StringPrintf("digest: %s", digest_header_value.c_str());
  headers.push_back(digest_header);

  const std::string signature_header_value = BuildSignatureHeaderValue(body);
  const std::string signature_header =
      base::StringPrintf("signature: %s", signature_header_value.c_str());
  headers.push_back(signature_header);

  const std::string content_type_header = "content-type: application/json";
  headers.push_back(content_type_header);

  const std::string via_header = server::BuildViaHeader();
  headers.push_back(via_header);

  const std::string accept_header = "accept: application/json";
  headers.push_back(accept_header);

  return headers;
}

std::string RequestSignedTokensUrlRequestBuilder::BuildDigestHeaderValue(
    const std::string& body) const {
  DCHECK(!body.empty());

  const std::vector<uint8_t> body_sha256 = security::Sha256Hash(body);
  const std::string body_sha256_base64 = base::Base64Encode(body_sha256);

  return base::StringPrintf("SHA-256=%s", body_sha256_base64.c_str());
}

std::string RequestSignedTokensUrlRequestBuilder::BuildSignatureHeaderValue(
    const std::string& body) const {
  DCHECK(!body.empty());

  const std::string digest_header_value = BuildDigestHeaderValue(body);

  return security::Sign({{"digest", digest_header_value}}, "primary",
                        wallet_.secret_key);
}

std::string RequestSignedTokensUrlRequestBuilder::BuildBody() const {
  base::Value list(base::Value::Type::LIST);

  for (const auto& blinded_token : blinded_tokens_) {
    const absl::optional<std::string> blinded_token_base64_optional =
        blinded_token.EncodeBase64();
    if (!blinded_token_base64_optional) {
      continue;
    }
    base::Value blinded_token_base64_value =
        base::Value(blinded_token_base64_optional.value());
    list.Append(std::move(blinded_token_base64_value));
  }

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetKey("blindedTokens", base::Value(std::move(list)));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

}  // namespace ads
