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
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostConnect::Error;
using Result = PostConnect::Result;
using mojom::ConnectExternalWalletResult;

namespace {

Result ParseGeoCountry(RewardsEngine& engine, const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* geo_country = dict.FindString("geoCountry");
  if (!geo_country || geo_country->empty()) {
    engine.LogError(FROM_HERE) << "Missing geoCountry response field";
    return base::unexpected(Error::kFailedToParseBody);
  }

  return *geo_country;
}

Result ParseErrorMessage(RewardsEngine& engine, const std::string& body) {
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

  if (message->find("KYC required") != std::string::npos) {
    // HTTP 403: Uphold
    engine.LogError(FROM_HERE) << "KYC required";
    return base::unexpected(Error::kKYCRequired);
  } else if (message->find("mismatched provider accounts") !=
             std::string::npos) {
    // HTTP 403: bitFlyer, Gemini, Uphold
    engine.LogError(FROM_HERE) << "Mismatched provider accounts";
    return base::unexpected(Error::kMismatchedProviderAccounts);
  } else if (message->find("transaction verification failure") !=
             std::string::npos) {
    // HTTP 403: Uphold
    engine.LogError(FROM_HERE) << "Transaction verification failure";
    return base::unexpected(Error::kTransactionVerificationFailure);
  } else if (message->find("request signature verification failure") !=
             std::string::npos) {
    // HTTP 403: bitFlyer, Gemini
    engine.LogError(FROM_HERE) << "Request signature verification failure";
    return base::unexpected(Error::kRequestSignatureVerificationFailure);
  } else if (message->find("unable to link - unusual activity") !=
             std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    engine.LogError(FROM_HERE) << "Flagged wallet";
    return base::unexpected(Error::kFlaggedWallet);
  } else if (message->find("region not supported") != std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    engine.LogError(FROM_HERE) << "Region not supported";
    return base::unexpected(Error::kRegionNotSupported);
  } else if (message->find("mismatched provider account regions") !=
             std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    engine.LogError(FROM_HERE) << "Mismatched countries";
    return base::unexpected(Error::kMismatchedCountries);
  } else if (message->find("is temporarily unavailable") != std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    engine.LogError(FROM_HERE) << "Provider unavailable";
    return base::unexpected(Error::kProviderUnavailable);
  } else {
    // bitFlyer, Gemini, Uphold
    engine.LogError(FROM_HERE) << "Unknown message";
    return base::unexpected(Error::kUnknownMessage);
  }
}

}  // namespace

// static
Result PostConnect::ProcessResponse(RewardsEngine& engine,
                                    const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseGeoCountry(engine, response.body);
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      return ParseErrorMessage(engine, response.body);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      return ParseErrorMessage(engine, response.body);
    case net::HTTP_NOT_FOUND:  // HTTP 404
      engine.LogError(FROM_HERE) << "KYC required";
      return base::unexpected(Error::kKYCRequired);
    case net::HTTP_CONFLICT:  // HTTP 409
      engine.LogError(FROM_HERE) << "Device limit reached";
      return base::unexpected(Error::kDeviceLimitReached);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      engine.LogError(FROM_HERE) << "Unexpected error";
      return base::unexpected(Error::kUnexpectedError);
    default:
      engine.LogError(FROM_HERE)
          << "Unexpected status code! (HTTP " << response.status_code << ')';
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

PostConnect::PostConnect(RewardsEngine& engine) : RequestBuilder(engine) {}

PostConnect::~PostConnect() = default;

std::optional<std::string> PostConnect::Url() const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet is null";
    return std::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());

  return engine_->Get<EnvironmentConfig>()
      .rewards_grant_url()
      .Resolve(Path(wallet->payment_id))
      .spec();
}

std::optional<std::vector<std::string>> PostConnect::Headers(
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

  return signer->GetSignedHeaders("post " + Path(wallet->payment_id), content);
}

std::string PostConnect::ContentType() const {
  return kApplicationJson;
}
}  // namespace brave_rewards::internal::endpoints
