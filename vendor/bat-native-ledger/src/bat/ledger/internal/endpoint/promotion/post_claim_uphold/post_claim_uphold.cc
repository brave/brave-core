/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_claim_uphold/post_claim_uphold.h"

#include <map>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

PostClaimUphold::PostClaimUphold(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostClaimUphold::~PostClaimUphold() = default;

std::string PostClaimUphold::GetUrl() {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  const std::string& path = base::StringPrintf("/v3/wallet/uphold/%s/claim",
                                               wallet->payment_id.c_str());

  return GetServerUrl(path);
}

std::string PostClaimUphold::GeneratePayload(const double user_funds,
                                             const std::string& address) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value denomination(base::Value::Type::DICTIONARY);
  denomination.SetStringKey("amount", base::NumberToString(user_funds));
  denomination.SetStringKey("currency", "BAT");

  base::Value octets(base::Value::Type::DICTIONARY);
  octets.SetKey("denomination", std::move(denomination));
  octets.SetStringKey("destination", address);
  std::string octets_json;
  base::JSONWriter::Write(octets, &octets_json);

  const std::string header_digest = util::Security::DigestValue(octets_json);

  std::vector<std::map<std::string, std::string>> headers;
  headers.push_back({{"digest", header_digest}});

  const std::string header_signature =
      util::Security::Sign(headers, "primary", wallet->recovery_seed);

  base::Value signed_reqeust(base::Value::Type::DICTIONARY);
  signed_reqeust.SetStringKey("octets", octets_json);
  signed_reqeust.SetKey("body", std::move(octets));

  base::Value headers_dict(base::Value::Type::DICTIONARY);
  headers_dict.SetStringKey("digest", header_digest);
  headers_dict.SetStringKey("signature", header_signature);
  signed_reqeust.SetKey("headers", std::move(headers_dict));

  std::string signed_request_json;
  base::JSONWriter::Write(signed_reqeust, &signed_request_json);

  std::string signed_request_base64;
  base::Base64Encode(signed_request_json, &signed_request_base64);

  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("signedLinkingRequest", signed_request_base64);
  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

type::Result PostClaimUphold::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Forbidden");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Not found");
    return type::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Not found");
    return type::Result::ALREADY_EXISTS;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void PostClaimUphold::Request(const double user_funds,
                              const std::string& address,
                              PostClaimUpholdCallback callback) {
  auto url_callback =
      std::bind(&PostClaimUphold::OnRequest, this, _1, address, callback);
  const std::string& payload = GeneratePayload(user_funds, address);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = payload;
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostClaimUphold::OnRequest(const type::UrlResponse& response,
                                const std::string& address,
                                PostClaimUpholdCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code), address);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
