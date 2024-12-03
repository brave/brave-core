/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_uphold.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/request_signer.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"

namespace brave_rewards::internal::endpoints {

PostConnectUphold::PostConnectUphold(RewardsEngine& engine,
                                     std::string&& address)
    : PostConnect(engine), address_(std::move(address)) {}

PostConnectUphold::~PostConnectUphold() = default;

std::optional<std::string> PostConnectUphold::Content() const {
  if (address_.empty()) {
    engine_->LogError(FROM_HERE) << "address_ is empty";
    return std::nullopt;
  }

  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet is null";
    return std::nullopt;
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
    engine_->LogError(FROM_HERE) << "Failed to write octets to JSON";
    return std::nullopt;
  }

  auto signer = RequestSigner::FromRewardsWallet(*wallet);
  if (!signer) {
    engine_->LogError(FROM_HERE) << "Unable to sign request";
    return std::nullopt;
  }

  std::string digest = RequestSigner::GetDigest(base::as_byte_span(octets));

  signer->set_key_id("primary");

  std::string signature = signer->SignHeaders(
      std::vector<std::pair<std::string, std::string>>{{"digest", digest}});

  if (signature.empty()) {
    engine_->LogError(FROM_HERE) << "Failed to create signature";
    return std::nullopt;
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
    engine_->LogError(FROM_HERE) << "Failed to write request to JSON";
    return std::nullopt;
  }

  base::Value::Dict content;
  content.Set("signedLinkingRequest",
              base::Base64Encode(std::move(json_request)));

  std::string json;
  if (!base::JSONWriter::Write(std::move(content), &json)) {
    engine_->LogError(FROM_HERE) << "Failed to write content to JSON";
    return std::nullopt;
  }

  return json;
}

std::optional<std::vector<std::string>> PostConnectUphold::Headers(
    const std::string&) const {
  return std::vector<std::string>{};
}

std::string PostConnectUphold::Path(base::cstring_view payment_id) const {
  return base::StringPrintf("/v3/wallet/uphold/%s/claim", payment_id.c_str());
}

}  // namespace brave_rewards::internal::endpoints
