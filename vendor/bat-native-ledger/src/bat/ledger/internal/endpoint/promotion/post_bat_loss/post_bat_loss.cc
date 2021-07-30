/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_bat_loss/post_bat_loss.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/common/request_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

PostBatLoss::PostBatLoss(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostBatLoss::~PostBatLoss() = default;

std::string PostBatLoss::GetUrl(const int32_t version) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  const std::string& path = base::StringPrintf(
      "/v1/wallets/%s/events/batloss/%d",
      wallet->payment_id.c_str(),
      version);

  return GetServerUrl(path);
}

std::string PostBatLoss::GeneratePayload(const double amount) {
  return base::StringPrintf(R"({"amount": %f})", amount);
}

type::Result PostBatLoss::CheckStatusCode(const int status_code) {
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

void PostBatLoss::Request(
    const double amount,
    const int32_t version,
    PostBatLossCallback callback) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string payload = GeneratePayload(amount);

  const std::string header_url = base::StringPrintf(
      "post /v1/wallets/%s/events/batloss/%d",
      wallet->payment_id.c_str(),
      version);

  const auto headers = util::BuildSignHeaders(
      header_url,
      payload,
      wallet->payment_id,
      wallet->recovery_seed);

  auto url_callback = std::bind(&PostBatLoss::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(version);
  request->content = payload;
  request->headers = headers;
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostBatLoss::OnRequest(
    const type::UrlResponse& response,
    PostBatLossCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
