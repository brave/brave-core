/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/confirmations/internal/request_signed_tokens_request.h"
#include "bat/confirmations/internal/ads_serve_helper.h"
#include "bat/confirmations/internal/string_helper.h"
#include "bat/confirmations/internal/security_helper.h"

#include "base/logging.h"
#include "base/json/json_writer.h"
#include "base/values.h"

namespace confirmations {

RequestSignedTokensRequest::RequestSignedTokensRequest() = default;

RequestSignedTokensRequest::~RequestSignedTokensRequest() = default;

// POST /v1/confirmation/token/{payment_id}

std::string RequestSignedTokensRequest::BuildUrl(
    const WalletInfo& wallet_info) const {
  DCHECK(!wallet_info.payment_id.empty());

  std::string endpoint = "/v1/confirmation/token/";
  endpoint += wallet_info.payment_id;

  return helper::AdsServe::GetURL().append(endpoint);
}

URLRequestMethod RequestSignedTokensRequest::GetMethod() const {
  return URLRequestMethod::POST;
}

std::string RequestSignedTokensRequest::BuildBody(
    const std::vector<BlindedToken>& tokens) const {
  DCHECK_NE(tokens.size(), 0UL);

  base::Value list(base::Value::Type::LIST);
  for (const auto& token : tokens) {
    auto token_base64 = token.encode_base64();
    auto token_value = base::Value(token_base64);
    list.Append(std::move(token_value));
  }

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetKey("blindedTokens", base::Value(std::move(list)));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

std::vector<std::string> RequestSignedTokensRequest::BuildHeaders(
    const std::string& body,
    const WalletInfo& wallet_info) const {
  std::string digest_header = "digest: ";
  digest_header += BuildDigestHeaderValue(body);

  std::string signature_header = "signature: ";
  signature_header += BuildSignatureHeaderValue(body, wallet_info);

  std::string accept_header = "accept: ";
  accept_header += GetAcceptHeaderValue();

  return {
    digest_header,
    signature_header,
    accept_header
  };
}

std::string RequestSignedTokensRequest::BuildDigestHeaderValue(
    const std::string& body) const {
  DCHECK(!body.empty());

  auto body_sha256 = helper::Security::GetSHA256(body);
  auto body_sha256_base64 = helper::Security::GetBase64(body_sha256);
  return "SHA-256=" + body_sha256_base64;
}

std::string RequestSignedTokensRequest::BuildSignatureHeaderValue(
    const std::string& body,
    const WalletInfo& wallet_info) const {
  DCHECK(!body.empty());
  DCHECK(!wallet_info.private_key.empty());

  auto digest_header_value = BuildDigestHeaderValue(body);

  auto private_key = helper::String::decode_hex(wallet_info.private_key);

  return helper::Security::Sign({{"digest", digest_header_value}}, "primary",
      private_key);
}

std::string RequestSignedTokensRequest::GetAcceptHeaderValue() const {
  return "application/json";
}

std::string RequestSignedTokensRequest::GetContentType() const {
  return "application/json";
}

}  // namespace confirmations
