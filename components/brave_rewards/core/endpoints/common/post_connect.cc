/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/common/post_connect.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/request_signer.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostConnect::Error;
using Result = PostConnect::Result;
using mojom::ConnectExternalWalletResult;

namespace {

Result ParseGeoCountry(const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* geo_country = dict.FindString("geoCountry");
  if (!geo_country || geo_country->empty()) {
    BLOG(0, "Missing geoCountry response field");
    return base::unexpected(Error::kFailedToParseBody);
  }

  return *geo_country;
}

Result ParseErrorMessage(const std::string& body) {
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

  if (message->find("KYC required") != std::string::npos) {
    // HTTP 403: Uphold
    BLOG(0, "KYC required!");
    return base::unexpected(Error::kKYCRequired);
  } else if (message->find("mismatched provider accounts") !=
             std::string::npos) {
    // HTTP 403: bitFlyer, Gemini, Uphold
    BLOG(0, "Mismatched provider accounts!");
    return base::unexpected(Error::kMismatchedProviderAccounts);
  } else if (message->find("transaction verification failure") !=
             std::string::npos) {
    // HTTP 403: Uphold
    BLOG(0, "Transaction verification failure!");
    return base::unexpected(Error::kTransactionVerificationFailure);
  } else if (message->find("request signature verification failure") !=
             std::string::npos) {
    // HTTP 403: bitFlyer, Gemini
    BLOG(0, "Request signature verification failure!");
    return base::unexpected(Error::kRequestSignatureVerificationFailure);
  } else if (message->find("unable to link - unusual activity") !=
             std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    BLOG(0, "Flagged wallet!");
    return base::unexpected(Error::kFlaggedWallet);
  } else if (message->find("region not supported") != std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    BLOG(0, "Region not supported!");
    return base::unexpected(Error::kRegionNotSupported);
  } else if (message->find("mismatched provider account regions") !=
             std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    BLOG(0, "Mismatched countries!");
    return base::unexpected(Error::kMismatchedCountries);
  } else if (message->find("is temporarily unavailable") != std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    BLOG(0, "Provider unavailable!");
    return base::unexpected(Error::kProviderUnavailable);
  } else {
    // bitFlyer, Gemini, Uphold
    BLOG(0, "Unknown message!");
    return base::unexpected(Error::kUnknownMessage);
  }
}

}  // namespace

// static
Result PostConnect::ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseGeoCountry(response.body);
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      return ParseErrorMessage(response.body);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      return ParseErrorMessage(response.body);
    case net::HTTP_NOT_FOUND:  // HTTP 404
      BLOG(0, "KYC required!");
      return base::unexpected(Error::kKYCRequired);
    case net::HTTP_CONFLICT:  // HTTP 409
      BLOG(0, "Device limit reached!");
      return base::unexpected(Error::kDeviceLimitReached);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      BLOG(0, "Unexpected error!");
      return base::unexpected(Error::kUnexpectedError);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

// static
ConnectExternalWalletResult PostConnect::ToConnectExternalWalletResult(
    const Result& result) {
  if (!result.has_value()) {
    switch (result.error()) {
      case Error::kFailedToCreateRequest:
        return ConnectExternalWalletResult::kUnexpected;
      case Error::kFlaggedWallet:  // HTTP 400
        return ConnectExternalWalletResult::kFlaggedWallet;
      case Error::kMismatchedCountries:  // HTTP 400
        return ConnectExternalWalletResult::kMismatchedCountries;
      case Error::kProviderUnavailable:  // HTTP 400
        return ConnectExternalWalletResult::kProviderUnavailable;
      case Error::kRegionNotSupported:  // HTTP 400
        return ConnectExternalWalletResult::kRegionNotSupported;
      case Error::kUnknownMessage:  // HTTP 400, HTTP 403
        return ConnectExternalWalletResult::kUnexpected;
      case Error::kKYCRequired:  // HTTP 403, HTTP 404
        return ConnectExternalWalletResult::kKYCRequired;
      case Error::kMismatchedProviderAccounts:  // HTTP 403
        return ConnectExternalWalletResult::kMismatchedProviderAccounts;
      case Error::kRequestSignatureVerificationFailure:  // HTTP 403
        return ConnectExternalWalletResult::
            kRequestSignatureVerificationFailure;
      case Error::kTransactionVerificationFailure:  // HTTP 403
        return ConnectExternalWalletResult::
            kUpholdTransactionVerificationFailure;
      case Error::kDeviceLimitReached:  // HTTP 409
        return ConnectExternalWalletResult::kDeviceLimitReached;
      case Error::kUnexpectedError:  // HTTP 500
        return ConnectExternalWalletResult::kUnexpected;
      case Error::kUnexpectedStatusCode:  // HTTP xxx
        return ConnectExternalWalletResult::kUnexpected;
      case Error::kFailedToParseBody:
        return ConnectExternalWalletResult::kUnexpected;
    }
  }

  return ConnectExternalWalletResult::kSuccess;
}

PostConnect::PostConnect(RewardsEngineImpl& engine) : RequestBuilder(engine) {}

PostConnect::~PostConnect() = default;

std::optional<std::string> PostConnect::Url() const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return std::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());

  return engine_->Get<EnvironmentConfig>()
      .rewards_grant_url()
      .Resolve(base::StringPrintf(Path(), wallet->payment_id.c_str()))
      .spec();
}

std::optional<std::vector<std::string>> PostConnect::Headers(
    const std::string& content) const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return std::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());
  DCHECK(!wallet->recovery_seed.empty());

  auto signer = RequestSigner::FromRewardsWallet(*wallet);
  if (!signer) {
    BLOG(0, "Unable to sign request");
    return std::nullopt;
  }

  return signer->GetSignedHeaders(
      "post " + base::StringPrintf(Path(), wallet->payment_id.c_str()),
      content);
}

std::string PostConnect::ContentType() const {
  return kApplicationJson;
}
}  // namespace brave_rewards::internal::endpoints
