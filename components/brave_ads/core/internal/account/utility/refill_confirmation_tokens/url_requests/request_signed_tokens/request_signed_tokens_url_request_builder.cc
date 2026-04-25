/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

std::string BuildDigestHeaderValue(const std::string& body) {
  CHECK(!body.empty());

  const std::vector<uint8_t> body_sha256 = crypto::Sha256(body);
  return "SHA-256=" + base::Base64Encode(body_sha256);
}

}  // namespace

RequestSignedTokensUrlRequestBuilder::RequestSignedTokensUrlRequestBuilder(
    WalletInfo wallet,
    cbr::BlindedTokenList blinded_tokens)
    : wallet_(std::move(wallet)), blinded_tokens_(std::move(blinded_tokens)) {
  CHECK(wallet_.IsValid());
  CHECK(!blinded_tokens_.empty());
}

RequestSignedTokensUrlRequestBuilder::~RequestSignedTokensUrlRequestBuilder() =
    default;

mojom::UrlRequestInfoPtr RequestSignedTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr mojom_url_request = mojom::UrlRequestInfo::New();
  mojom_url_request->url = BuildUrl();
  const std::string body = BuildBody();
  mojom_url_request->headers = BuildHeaders(body);
  mojom_url_request->content = body;
  mojom_url_request->content_type = "application/json";
  mojom_url_request->method = mojom::UrlRequestMethodType::kPost;

  return mojom_url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL RequestSignedTokensUrlRequestBuilder::BuildUrl() const {
  const std::string spec = GetNonAnonymousUrlHost() +
                           BuildRequestSignedTokensUrlPath(wallet_.payment_id);
  return GURL(spec);
}

std::vector<std::string> RequestSignedTokensUrlRequestBuilder::BuildHeaders(
    const std::string& body) const {
  std::vector<std::string> headers;
  headers.push_back("digest: " + BuildDigestHeaderValue(body));
  headers.push_back("signature: " + BuildSignatureHeaderValue(body));
  headers.emplace_back("content-type: application/json");
  headers.emplace_back("accept: application/json");
  return headers;
}

std::string RequestSignedTokensUrlRequestBuilder::BuildSignatureHeaderValue(
    const std::string& body) const {
  CHECK(!body.empty());

  const base::flat_map<std::string, std::string> headers = {
      {"digest", BuildDigestHeaderValue(body)}};

  std::string concatenated_header;
  std::string concatenated_message;

  size_t index = 0;
  for (const auto& [header, value] : headers) {
    if (index != 0) {
      concatenated_header += " ";
      concatenated_message += "\n";
    }

    concatenated_header += header;
    concatenated_message += base::StrCat({header, ": ", value});

    ++index;
  }

  std::optional<std::string> signature_base64 =
      crypto::Sign(concatenated_message, wallet_.secret_key_base64);
  if (!signature_base64) {
    return {};
  }

  return base::ReplaceStringPlaceholders(
      R"(keyId="primary",algorithm="ed25519",headers="$1",signature="$2")",
      {concatenated_header, *signature_base64}, nullptr);
}

std::string RequestSignedTokensUrlRequestBuilder::BuildBody() const {
  base::ListValue list;

  for (const auto& blinded_token : blinded_tokens_) {
    if (std::optional<std::string> blinded_token_base64 =
            blinded_token.EncodeBase64()) {
      list.Append(*blinded_token_base64);
    }
  }

  std::string json;
  CHECK(base::JSONWriter::Write(
      base::DictValue().Set("blindedTokens", std::move(list)), &json));
  return json;
}

}  // namespace brave_ads
