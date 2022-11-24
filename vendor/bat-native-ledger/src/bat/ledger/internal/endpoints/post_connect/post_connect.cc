/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoints/post_connect/post_connect.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger::endpoints {
using Error = PostConnect::Error;
using Result = PostConnect::Result;

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
      return {};
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      return ParseBody(response.body);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      return ParseBody(response.body);
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
        return base::unexpected(mojom::ConnectExternalWalletError::kUnexpected);
      case Error::kFlaggedWallet:  // HTTP 400
        return base::unexpected(
            mojom::ConnectExternalWalletError::kFlaggedWallet);
      case Error::kMismatchedCountries:  // HTTP 400
        return base::unexpected(
            mojom::ConnectExternalWalletError::kMismatchedCountries);
      case Error::kProviderUnavailable:  // HTTP 400
        return base::unexpected(
            mojom::ConnectExternalWalletError::kProviderUnavailable);
      case Error::kRegionNotSupported:  // HTTP 400
        return base::unexpected(
            mojom::ConnectExternalWalletError::kRegionNotSupported);
      case Error::kUnknownMessage:  // HTTP 400, HTTP 403
        return base::unexpected(mojom::ConnectExternalWalletError::kUnexpected);
      case Error::kKYCRequired:  // HTTP 403, HTTP 404
        return base::unexpected(
            mojom::ConnectExternalWalletError::kKYCRequired);
      case Error::kMismatchedProviderAccounts:  // HTTP 403
        return base::unexpected(
            mojom::ConnectExternalWalletError::kMismatchedProviderAccounts);
      case Error::kRequestSignatureVerificationFailure:  // HTTP 403
        return base::unexpected(mojom::ConnectExternalWalletError::
                                    kRequestSignatureVerificationFailure);
      case Error::kTransactionVerificationFailure:  // HTTP 403
        return base::unexpected(mojom::ConnectExternalWalletError::
                                    kUpholdTransactionVerificationFailure);
      case Error::kDeviceLimitReached:  // HTTP 409
        return base::unexpected(
            mojom::ConnectExternalWalletError::kDeviceLimitReached);
      case Error::kUnexpectedError:  // HTTP 500
        return base::unexpected(mojom::ConnectExternalWalletError::kUnexpected);
      case Error::kUnexpectedStatusCode:  // HTTP xxx
        return base::unexpected(mojom::ConnectExternalWalletError::kUnexpected);
      case Error::kFailedToParseBody:
        return base::unexpected(mojom::ConnectExternalWalletError::kUnexpected);
    }
  }

  return {};
}

PostConnect::PostConnect(LedgerImpl* ledger) : RequestBuilder(ledger) {}

PostConnect::~PostConnect() = default;

absl::optional<std::string> PostConnect::Url() const {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());

  return endpoint::promotion::GetServerUrl(
      base::StringPrintf(Path(), wallet->payment_id.c_str()));
}

absl::optional<std::vector<std::string>> PostConnect::Headers(
    const std::string& content) const {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());
  DCHECK(!wallet->recovery_seed.empty());

  return util::BuildSignHeaders(
      "post " + base::StringPrintf(Path(), wallet->payment_id.c_str()), content,
      wallet->payment_id, wallet->recovery_seed);
}

std::string PostConnect::ContentType() const {
  return kApplicationJson;
}
}  // namespace ledger::endpoints
