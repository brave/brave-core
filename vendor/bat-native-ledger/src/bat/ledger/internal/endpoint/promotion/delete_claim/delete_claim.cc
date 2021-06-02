/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/promotion/delete_claim/delete_claim.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

std::string GetPath(const std::string& custodian,
                    const std::string& payment_id) {
  return base::StringPrintf("/v3/wallet/%s/%s/claim", custodian.c_str(),
                            payment_id.c_str());
}

}  // namespace

namespace ledger {
namespace endpoint {
namespace promotion {

DeleteClaim::DeleteClaim(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

DeleteClaim::~DeleteClaim() = default;

std::string DeleteClaim::GetUrl(const std::string& custodian) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  const std::string path = GetPath(custodian, wallet->payment_id);
  return GetServerUrl(path);
}

type::Result DeleteClaim::CheckStatusCode(const int status_code) {
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

void DeleteClaim::Request(const std::string& custodian,
                          DeleteClaimCallback callback) {
  auto url_callback = std::bind(&DeleteClaim::OnRequest, this, _1, callback);
  const std::string payload;

  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const auto sign_url = base::StringPrintf(
      "delete %s", GetPath(custodian, wallet->payment_id).c_str());
  auto headers = util::BuildSignHeaders(sign_url, payload, wallet->payment_id,
                                        wallet->recovery_seed);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(custodian);
  request->content = payload;
  request->content_type = "application/json; charset=utf-8";
  request->headers = headers;
  request->method = type::UrlMethod::DEL;
  ledger_->LoadURL(std::move(request), url_callback);
}

void DeleteClaim::OnRequest(const type::UrlResponse& response,
                            DeleteClaimCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
