/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_claim_brave/post_claim_brave.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

std::string GetPath(const std::string& payment_id) {
  return base::StringPrintf(
      "/v3/wallet/brave/%s/claim",
      payment_id.c_str());
}

}  // namespace

namespace ledger {
namespace endpoint {
namespace promotion {

PostClaimBrave::PostClaimBrave(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostClaimBrave::~PostClaimBrave() = default;

std::string PostClaimBrave::GetUrl() {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  const std::string& path = GetPath(wallet->payment_id);

  return GetServerUrl(path);
}

std::string PostClaimBrave::GeneratePayload(
    const std::string& destination_payment_id) {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("depositDestination", destination_payment_id);
  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

type::Result PostClaimBrave::CheckStatusCode(const int status_code) {
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
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void PostClaimBrave::Request(
    const std::string& destination_payment_id,
    PostClaimBraveCallback callback) {
  auto url_callback = std::bind(&PostClaimBrave::OnRequest,
      this,
      _1,
      callback);
  const std::string payload = GeneratePayload(destination_payment_id);

  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const auto sign_url =  base::StringPrintf(
      "post %s",
      GetPath(wallet->payment_id).c_str());
  auto headers = util::BuildSignHeaders(
      sign_url,
      payload,
      wallet->payment_id,
      wallet->recovery_seed);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = payload;
  request->headers = headers;
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostClaimBrave::OnRequest(
    const type::UrlResponse& response,
    PostClaimBraveCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
