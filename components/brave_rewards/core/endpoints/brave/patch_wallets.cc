/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/patch_wallets.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/request_util.h"
#include "brave/components/brave_rewards/core/common/security_util.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PatchWallets::Error;
using Result = PatchWallets::Result;

namespace {

Result ParseBody(const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* message = dict.FindString("message");
  if (!message) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  if (message->find("payment id does not match http signature key id") !=
      std::string::npos) {
    // HTTP 403
    BLOG(0, "Invalid request!");
    return base::unexpected(Error::kInvalidRequest);
  } else if (message->find("request signature verification failure") !=
             std::string::npos) {
    // HTTP 403
    BLOG(0, "Request signature verification failure!");
    return base::unexpected(Error::kRequestSignatureVerificationFailure);
  } else {
    BLOG(0, "Unknown message!");
    return base::unexpected(Error::kUnknownMessage);
  }
}

}  // namespace

// static
Result PatchWallets::ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return {};
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      BLOG(0, "Invalid request!");
      return base::unexpected(Error::kInvalidRequest);
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      BLOG(0, "Bad request signature!");
      return base::unexpected(Error::kBadRequestSignature);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      return ParseBody(response.body);
    case net::HTTP_CONFLICT:  // HTTP 409
      BLOG(0, "geo_country already declared!");
      return base::unexpected(Error::kGeoCountryAlreadyDeclared);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      BLOG(0, "Unexpected error!");
      return base::unexpected(Error::kUnexpectedError);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

PatchWallets::PatchWallets(RewardsEngineImpl& engine, std::string&& geo_country)
    : RequestBuilder(engine), geo_country_(std::move(geo_country)) {}

PatchWallets::~PatchWallets() = default;

const char* PatchWallets::Path() const {
  return "/v4/wallets/%s";
}

absl::optional<std::string> PatchWallets::Url() const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());

  return endpoint::promotion::GetServerUrl(
      base::StringPrintf(Path(), wallet->payment_id.c_str()));
}

mojom::UrlMethod PatchWallets::Method() const {
  return mojom::UrlMethod::PATCH;
}

absl::optional<std::vector<std::string>> PatchWallets::Headers(
    const std::string& content) const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());
  DCHECK(!wallet->recovery_seed.empty());

  return util::BuildSignHeaders(
      "patch " + base::StringPrintf(Path(), wallet->payment_id.c_str()),
      content, wallet->payment_id, wallet->recovery_seed);
}

absl::optional<std::string> PatchWallets::Content() const {
  if (geo_country_.empty()) {
    BLOG(0, "geo_country_ is empty!");
    return absl::nullopt;
  }

  base::Value::Dict content;
  content.Set("geoCountry", geo_country_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    BLOG(0, "Failed to write content to JSON!");
    return absl::nullopt;
  }

  return json;
}

std::string PatchWallets::ContentType() const {
  return kApplicationJson;
}

}  // namespace brave_rewards::internal::endpoints
