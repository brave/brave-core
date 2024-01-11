/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/post_wallets.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/request_signer.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostWallets::Error;
using Result = PostWallets::Result;

namespace {

Result ParseBody(const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  const auto* payment_id = value->GetDict().FindString("paymentId");
  if (!payment_id || payment_id->empty()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  return *payment_id;
}

}  // namespace

// static
Result PostWallets::ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_CREATED:  // HTTP 201
      return ParseBody(response.body);
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      BLOG(0, "Invalid request!");
      return base::unexpected(Error::kInvalidRequest);
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      BLOG(0, "Invalid public key!");
      return base::unexpected(Error::kInvalidPublicKey);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      BLOG(0, "Wallet generation disabled!");
      return base::unexpected(Error::kWalletGenerationDisabled);
    case net::HTTP_CONFLICT:  // HTTP 409
      BLOG(0, "Wallet already exists!");
      return base::unexpected(Error::kWalletAlreadyExists);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      BLOG(0, "Unexpected error!");
      return base::unexpected(Error::kUnexpectedError);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

PostWallets::PostWallets(RewardsEngineImpl& engine,
                         std::optional<std::string>&& geo_country)
    : RequestBuilder(engine), geo_country_(std::move(geo_country)) {}

PostWallets::~PostWallets() = default;

const char* PostWallets::Path() const {
  return geo_country_ ? "/v4/wallets" : "/v3/wallet/brave";
}

std::optional<std::string> PostWallets::Url() const {
  return engine_->Get<EnvironmentConfig>()
      .rewards_grant_url()
      .Resolve(Path())
      .spec();
}

std::optional<std::vector<std::string>> PostWallets::Headers(
    const std::string& content) const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return std::nullopt;
  }

  auto request_signer = RequestSigner::FromRewardsWallet(*wallet);
  if (!request_signer) {
    BLOG(0, "Unable to sign request");
    return std::nullopt;
  }

  request_signer->set_key_id(
      base::HexEncode(request_signer->signer().public_key()));

  return request_signer->GetSignedHeaders(std::string("post ") + Path(),
                                          content);
}

std::optional<std::string> PostWallets::Content() const {
  if (!geo_country_) {
    BLOG(1, "geo_country_ is null - creating old wallet.");
    return "";
  }

  if (geo_country_->empty()) {
    BLOG(0, "geo_country_ is empty!");
    return std::nullopt;
  }

  base::Value::Dict content;
  content.Set("geoCountry", *geo_country_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    BLOG(0, "Failed to write content to JSON!");
    return std::nullopt;
  }

  return json;
}

std::string PostWallets::ContentType() const {
  return kApplicationJson;
}

}  // namespace brave_rewards::internal::endpoints
