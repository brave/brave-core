/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/bitflyer/post_claim/post_claim_bitflyer.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/mojom_structs.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

const char kDevelopment[] = "https://grant.rewards.brave.software";
const char kStaging[] = "https://grant.rewards.bravesoftware.com";
const char kProduction[] = "https://grant.rewards.brave.com";

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  switch (ledger::_environment) {
    case ledger::type::Environment::DEVELOPMENT:
      url = kDevelopment;
      break;
    case ledger::type::Environment::STAGING:
      url = kStaging;
      break;
    case ledger::type::Environment::PRODUCTION:
      url = kProduction;
      break;
  }

  return url + path;
}

std::string GetPath(const std::string& payment_id) {
  return base::StringPrintf("/v3/wallet/bitflyer/%s/claim", payment_id.c_str());
}

}  // namespace

namespace ledger {
namespace endpoint {
namespace bitflyer {

PostClaimBitflyer::PostClaimBitflyer(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostClaimBitflyer::~PostClaimBitflyer() = default;

std::string PostClaimBitflyer::GetUrl() {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  const std::string path = GetPath(wallet->payment_id);

  return GetServerUrl(path);
}

std::string PostClaimBitflyer::GeneratePayload(
    const std::string& linking_info) {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("linkingInfo", linking_info);
  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

type::Result PostClaimBitflyer::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Not found");
    return type::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Conflict");
    return type::Result::ALREADY_EXISTS;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void PostClaimBitflyer::Request(const std::string& linking_info,
                                PostClaimBitflyerCallback callback) {
  auto url_callback =
      std::bind(&PostClaimBitflyer::OnRequest, this, _1, callback);
  const std::string payload = GeneratePayload(linking_info);

  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const auto sign_url =
      base::StringPrintf("post %s", GetPath(wallet->payment_id).c_str());
  auto headers = util::BuildSignHeaders(sign_url, payload, wallet->payment_id,
                                        wallet->recovery_seed);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = payload;
  request->headers = headers;
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostClaimBitflyer::OnRequest(const type::UrlResponse& response,
                                  PostClaimBitflyerCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
