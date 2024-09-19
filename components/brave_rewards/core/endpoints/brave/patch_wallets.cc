/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/patch_wallets.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/request_signer.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PatchWallets::Error;
using Result = PatchWallets::Result;

namespace {

constexpr char kPatchWalletsPathPrefix[] = "/v4/wallets/";

Result ParseBody(RewardsEngine& engine, const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* message = dict.FindString("message");
  if (!message) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  if (message->find("payment id does not match http signature key id") !=
      std::string::npos) {
    // HTTP 403
    engine.LogError(FROM_HERE) << "Invalid request";
    return base::unexpected(Error::kInvalidRequest);
  } else if (message->find("request signature verification failure") !=
             std::string::npos) {
    // HTTP 403
    engine.LogError(FROM_HERE) << "Request signature verification failure";
    return base::unexpected(Error::kRequestSignatureVerificationFailure);
  } else {
    engine.LogError(FROM_HERE) << "Unknown message";
    return base::unexpected(Error::kUnknownMessage);
  }
}

}  // namespace

// static
Result PatchWallets::ProcessResponse(RewardsEngine& engine,
                                     const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return {};
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      engine.LogError(FROM_HERE) << "Invalid request";
      return base::unexpected(Error::kInvalidRequest);
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      engine.LogError(FROM_HERE) << "Bad request signature";
      return base::unexpected(Error::kBadRequestSignature);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      return ParseBody(engine, response.body);
    case net::HTTP_CONFLICT:  // HTTP 409
      engine.LogError(FROM_HERE) << "geo_country already declared";
      return base::unexpected(Error::kGeoCountryAlreadyDeclared);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      engine.LogError(FROM_HERE) << "Unexpected error";
      return base::unexpected(Error::kUnexpectedError);
    default:
      engine.LogError(FROM_HERE)
          << "Unexpected status code! (HTTP " << response.status_code << ')';
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

PatchWallets::PatchWallets(RewardsEngine& engine, std::string&& geo_country)
    : RequestBuilder(engine), geo_country_(std::move(geo_country)) {}

PatchWallets::~PatchWallets() = default;

std::optional<std::string> PatchWallets::Url() const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet is null";
    return std::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());

  return engine_->Get<EnvironmentConfig>()
      .rewards_grant_url()
      .Resolve(base::StrCat({kPatchWalletsPathPrefix, wallet->payment_id}))
      .spec();
}

mojom::UrlMethod PatchWallets::Method() const {
  return mojom::UrlMethod::PATCH;
}

std::optional<std::vector<std::string>> PatchWallets::Headers(
    const std::string& content) const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet is null";
    return std::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());
  DCHECK(!wallet->recovery_seed.empty());

  auto signer = RequestSigner::FromRewardsWallet(*wallet);
  if (!signer) {
    engine_->LogError(FROM_HERE) << "Unable to sign request";
    return std::nullopt;
  }

  return signer->GetSignedHeaders(
      base::StrCat({"patch ", kPatchWalletsPathPrefix, wallet->payment_id}),
      content);
}

std::optional<std::string> PatchWallets::Content() const {
  if (geo_country_.empty()) {
    engine_->LogError(FROM_HERE) << "geo_country_ is empty";
    return std::nullopt;
  }

  base::Value::Dict content;
  content.Set("geoCountry", geo_country_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    engine_->LogError(FROM_HERE) << "Failed to write content to JSON";
    return std::nullopt;
  }

  return json;
}

std::string PatchWallets::ContentType() const {
  return kApplicationJson;
}

}  // namespace brave_rewards::internal::endpoints
