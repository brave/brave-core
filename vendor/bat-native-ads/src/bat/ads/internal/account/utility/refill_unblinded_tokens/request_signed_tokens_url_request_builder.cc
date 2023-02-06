/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/request_signed_tokens_url_request_builder.h"

#include <cstdint>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ads/internal/common/crypto/crypto_util.h"
#include "bat/ads/internal/server/headers/via_header_util.h"
#include "bat/ads/internal/server/url/hosts/server_host_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {

namespace {

std::string BuildDigestHeaderValue(const std::string& body) {
  DCHECK(!body.empty());

  const std::vector<uint8_t> body_sha256 = crypto::Sha256(body);
  const std::string body_sha256_base64 = base::Base64Encode(body_sha256);

  return base::StringPrintf("SHA-256=%s", body_sha256_base64.c_str());
}

}  // namespace

RequestSignedTokensUrlRequestBuilder::RequestSignedTokensUrlRequestBuilder(
    WalletInfo wallet,
    std::vector<privacy::cbr::BlindedToken> blinded_tokens)
    : wallet_(std::move(wallet)), blinded_tokens_(std::move(blinded_tokens)) {
  DCHECK(wallet_.IsValid());
  DCHECK(!blinded_tokens_.empty());
}

RequestSignedTokensUrlRequestBuilder::~RequestSignedTokensUrlRequestBuilder() =
    default;

// POST /v3/confirmation/token/{paymentId}

mojom::UrlRequestInfoPtr RequestSignedTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  const std::string body = BuildBody();
  url_request->headers = BuildHeaders(body);
  url_request->content = body;
  url_request->content_type = "application/json";
  url_request->method = mojom::UrlRequestMethodType::kPost;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL RequestSignedTokensUrlRequestBuilder::BuildUrl() const {
  const std::string spec = base::StringPrintf(
      "%s/v3/confirmation/token/%s", server::GetNonAnonymousHost().c_str(),
      wallet_.payment_id.c_str());
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

std::string RequestSignedTokensUrlRequestBuilder::BuildSignatureHeaderValue(
    const std::string& body) const {
  DCHECK(!body.empty());

  const base::flat_map<std::string, std::string> headers = {
      {"digest", BuildDigestHeaderValue(body)}};

  std::string concatenated_header;
  std::string concatenated_message;

  unsigned int index = 0;
  for (const auto& header : headers) {
    if (index != 0) {
      concatenated_header += " ";
      concatenated_message += "\n";
    }

    concatenated_header += header.first;
    concatenated_message += header.first + ": " + header.second;

    index++;
  }

  const absl::optional<std::string> signature_base64 =
      crypto::Sign(concatenated_message, wallet_.secret_key);
  if (!signature_base64) {
    return {};
  }

  return R"(keyId="primary",algorithm="ed25519",headers=")" +
         concatenated_header + R"(",signature=")" + *signature_base64 + R"(")";
}

std::string RequestSignedTokensUrlRequestBuilder::BuildBody() const {
  base::Value::List list;

  for (const auto& blinded_token : blinded_tokens_) {
    if (const auto blinded_token_base64 = blinded_token.EncodeBase64()) {
      base::Value value = base::Value(*blinded_token_base64);
      list.Append(std::move(value));
    }
  }

  base::Value::Dict dict;
  dict.Set("blindedTokens", std::move(list));

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

}  // namespace ads
