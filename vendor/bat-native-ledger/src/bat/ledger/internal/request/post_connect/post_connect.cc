/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/request/post_connect/post_connect.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger::request {
namespace {

mojom::Result ParseBody(const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid body!");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* message = dict.FindString("message");
  if (!message) {
    BLOG(0, "message is missing!");
    return mojom::Result::LEDGER_ERROR;
  }

  if (message->find("KYC required") != std::string::npos) {
    // HTTP 403: Uphold
    return mojom::Result::NOT_FOUND;
  } else if (message->find("mismatched provider accounts") !=
             std::string::npos) {
    // HTTP 403: bitFlyer, Gemini, Uphold
    return mojom::Result::MISMATCHED_PROVIDER_ACCOUNTS;
  } else if (message->find("transaction verification failure") !=
             std::string::npos) {
    // HTTP 403: Uphold
    return mojom::Result::UPHOLD_TRANSACTION_VERIFICATION_FAILURE;
  } else if (message->find("request signature verification failure") !=
             std::string::npos) {
    // HTTP 403: bitFlyer, Gemini
    return mojom::Result::REQUEST_SIGNATURE_VERIFICATION_FAILURE;
  } else if (message->find("unable to link - unusual activity") !=
             std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    return mojom::Result::FLAGGED_WALLET;
  } else if (message->find("region not supported") != std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    return mojom::Result::REGION_NOT_SUPPORTED;
  } else if (message->find("mismatched provider account regions") !=
             std::string::npos) {
    // HTTP 400: bitFlyer, Gemini, Uphold
    return mojom::Result::MISMATCHED_PROVIDER_ACCOUNT_REGIONS;
  } else {
    // bitFlyer, Gemini, Uphold
    BLOG(0, "Unknown message!");
    return mojom::Result::LEDGER_ERROR;
  }
}

mojom::Result ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_BAD_REQUEST:
      BLOG(0, "Invalid request");
      return ParseBody(response.body);
    case net::HTTP_FORBIDDEN:
      BLOG(0, "Forbidden");
      return ParseBody(response.body);
    case net::HTTP_NOT_FOUND:
      BLOG(0, "Not found");
      return mojom::Result::NOT_FOUND;
    case net::HTTP_CONFLICT:
      BLOG(0, "Conflict");
      return mojom::Result::DEVICE_LIMIT_REACHED;
    case net::HTTP_INTERNAL_SERVER_ERROR:
      BLOG(0, "Internal server error");
      return mojom::Result::LEDGER_ERROR;
    default:
      if (response.status_code != net::HTTP_OK) {
        BLOG(0, "Unexpected HTTP status: " << response.status_code);
        return mojom::Result::LEDGER_ERROR;
      } else {
        return mojom::Result::LEDGER_OK;
      }
  }
}

}  // namespace

// static
void PostConnect::OnResponse(Callback callback,
                             const mojom::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);
  std::move(callback).Run(ProcessResponse(response));
}

PostConnect::PostConnect(LedgerImpl* ledger) : RequestBuilder(ledger) {}

PostConnect::~PostConnect() = default;

absl::optional<std::string> PostConnect::Url() const {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  return ledger::endpoint::promotion::GetServerUrl(
      base::StringPrintf(Path(), wallet->payment_id.c_str()));
}

absl::optional<std::vector<std::string>> PostConnect::Headers() const {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  const auto content = Content();
  if (!content) {
    return absl::nullopt;
  }

  return util::BuildSignHeaders(
      "post " + base::StringPrintf(Path(), wallet->payment_id.c_str()),
      *content, wallet->payment_id, wallet->recovery_seed);
}

std::string PostConnect::ContentType() const {
  return "application/json; charset=utf-8";
}

}  // namespace ledger::request
