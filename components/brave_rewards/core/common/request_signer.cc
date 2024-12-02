/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/request_signer.h"

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "crypto/sha2.h"
#include "url/gurl.h"

namespace brave_rewards::internal {

namespace {

const char* GetMethodString(mojom::UrlMethod method) {
  switch (method) {
    case mojom::UrlMethod::GET:
      return "get";
    case mojom::UrlMethod::PUT:
      return "put";
    case mojom::UrlMethod::POST:
      return "post";
    case mojom::UrlMethod::PATCH:
      return "patch";
    case mojom::UrlMethod::DEL:
      return "delete";
  }
}

std::string GetRequestTarget(const mojom::UrlRequest& request) {
  GURL url(request.url);
  if (!url.is_valid()) {
    return "";
  }
  return base::StrCat({GetMethodString(request.method), " ", url.path()});
}

}  // namespace

RequestSigner::RequestSigner(const std::string& key_id, Signer signer)
    : key_id_(key_id), signer_(signer) {}

RequestSigner::~RequestSigner() = default;

RequestSigner::RequestSigner(const RequestSigner&) = default;
RequestSigner& RequestSigner::operator=(const RequestSigner&) = default;

std::string RequestSigner::GetDigest(base::span<const uint8_t> content) {
  return "SHA-256=" + base::Base64Encode(crypto::SHA256Hash(content));
}

std::optional<RequestSigner> RequestSigner::FromRewardsWallet(
    const mojom::RewardsWallet& rewards_wallet) {
  auto signer = Signer::FromRecoverySeed(rewards_wallet.recovery_seed);
  if (!signer) {
    return std::nullopt;
  }
  return RequestSigner(rewards_wallet.payment_id, *signer);
}

bool RequestSigner::SignRequest(mojom::UrlRequest& request) {
  std::string request_target = GetRequestTarget(request);
  if (request_target.empty()) {
    return false;
  }

  auto headers = GetSignedHeaders(request_target, request.content);
  for (auto& header : headers) {
    request.headers.push_back(std::move(header));
  }

  return true;
}

std::vector<std::string> RequestSigner::GetSignedHeaders(
    const std::string& request_target,
    const std::string& request_content) {
  CHECK(!request_target.empty());

  std::string digest = GetDigest(base::as_byte_span(request_content));

  std::vector<std::pair<std::string, std::string>> headers = {
      {"digest", digest}, {"(request-target)", request_target}};

  std::string signature = SignHeaders(headers);

  return {"digest: " + digest, "signature: " + signature,
          "accept: application/json"};
}

std::string RequestSigner::SignHeaders(
    base::span<const std::pair<std::string, std::string>> headers) {
  std::string header_names;
  std::string message;

  for (auto& [key, value] : headers) {
    if (!header_names.empty()) {
      header_names += " ";
      message += "\n";
    }
    header_names += key;
    message += key + ": " + value;
  }

  auto signed_message = signer_.SignMessage(base::as_byte_span(message));

  return base::StrCat(
      {"keyId=\"", key_id_, "\",algorithm=\"ed25519\",headers=\"", header_names,
       "\",signature=\"", base::Base64Encode(signed_message), "\""});
}

}  // namespace brave_rewards::internal
