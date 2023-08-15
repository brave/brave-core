/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_uphold.h"

#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/security_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"

namespace brave_rewards::internal::endpoints {

PostConnectUphold::PostConnectUphold(RewardsEngineImpl& engine,
                                     std::string&& address)
    : PostConnect(engine), address_(std::move(address)) {}

PostConnectUphold::~PostConnectUphold() = default;

absl::optional<std::string> PostConnectUphold::Content() const {
  if (address_.empty()) {
    BLOG(0, "address_ is empty!");
    return absl::nullopt;
  }

  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  DCHECK(!wallet->recovery_seed.empty());

  base::Value::Dict denomination;
  denomination.Set("amount", "0");
  denomination.Set("currency", "BAT");

  base::Value::Dict body;
  body.Set("denomination", std::move(denomination));
  body.Set("destination", address_);

  std::string octets;
  if (!base::JSONWriter::Write(body, &octets)) {
    BLOG(0, "Failed to write octets to JSON!");
    return absl::nullopt;
  }

  std::string digest = util::Security::DigestValue(octets);
  std::string signature = util::Security::Sign(
      {{{"digest", digest}}}, "primary", wallet->recovery_seed);
  if (signature.empty()) {
    BLOG(0, "Failed to create signature!");
    return absl::nullopt;
  }

  base::Value::Dict headers;
  headers.Set("digest", std::move(digest));
  headers.Set("signature", std::move(signature));

  base::Value::Dict request;
  request.Set("body", std::move(body));
  request.Set("headers", std::move(headers));
  request.Set("octets", std::move(octets));

  std::string json_request;
  if (!base::JSONWriter::Write(std::move(request), &json_request)) {
    BLOG(0, "Failed to write request to JSON!");
    return absl::nullopt;
  }

  std::string signedLinkingRequest;
  base::Base64Encode(std::move(json_request), &signedLinkingRequest);

  base::Value::Dict content;
  content.Set("signedLinkingRequest", std::move(signedLinkingRequest));

  std::string json;
  if (!base::JSONWriter::Write(std::move(content), &json)) {
    BLOG(0, "Failed to write content to JSON!");
    return absl::nullopt;
  }

  return json;
}

absl::optional<std::vector<std::string>> PostConnectUphold::Headers(
    const std::string&) const {
  return std::vector<std::string>{};
}

const char* PostConnectUphold::Path() const {
  return "/v3/wallet/uphold/%s/claim";
}

}  // namespace brave_rewards::internal::endpoints
