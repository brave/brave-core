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
    return base::unexpected(Error::kKycRequired);
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
    BLOG(0, "Mismatched provider account regions!");
    return base::unexpected(Error::kMismatchedProviderAccountRegions);
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
      return base::unexpected(Error::kKycRequired);
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
mojom::Result PostConnect::ToLegacyResult(const Result& result) {
  if (!result.has_value()) {
    switch (result.error()) {
      case Error::kFailedToCreateRequest:
        return mojom::Result::LEDGER_ERROR;
      case Error::kFlaggedWallet:  // HTTP 400
        return mojom::Result::FLAGGED_WALLET;
      case Error::kMismatchedProviderAccountRegions:  // HTTP 400
        return mojom::Result::MISMATCHED_PROVIDER_ACCOUNT_REGIONS;
      case Error::kRegionNotSupported:  // HTTP 400
        return mojom::Result::REGION_NOT_SUPPORTED;
      case Error::kUnknownMessage:  // HTTP 400, HTTP 403
        return mojom::Result::LEDGER_ERROR;
      case Error::kKycRequired:  // HTTP 403, HTTP 404
        return mojom::Result::NOT_FOUND;
      case Error::kMismatchedProviderAccounts:  // HTTP 403
        return mojom::Result::MISMATCHED_PROVIDER_ACCOUNTS;
      case Error::kRequestSignatureVerificationFailure:  // HTTP 403
        return mojom::Result::REQUEST_SIGNATURE_VERIFICATION_FAILURE;
      case Error::kTransactionVerificationFailure:  // HTTP 403
        return mojom::Result::UPHOLD_TRANSACTION_VERIFICATION_FAILURE;
      case Error::kDeviceLimitReached:  // HTTP 409
        return mojom::Result::DEVICE_LIMIT_REACHED;
      case Error::kUnexpectedError:  // HTTP 500
        return mojom::Result::LEDGER_ERROR;
      case Error::kUnexpectedStatusCode:  // HTTP xxx
        return mojom::Result::LEDGER_ERROR;
      case Error::kFailedToParseBody:
        return mojom::Result::LEDGER_ERROR;
    }
  }

  return mojom::Result::LEDGER_OK;
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
