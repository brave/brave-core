/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/post_transaction_commit/post_transaction_commit.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace uphold {

PostTransactionCommit::PostTransactionCommit(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostTransactionCommit::~PostTransactionCommit() = default;

std::string PostTransactionCommit::GetUrl(
    const std::string& address,
    const std::string& transaction_id) {
  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/transactions/%s/commit",
      address.c_str(),
      transaction_id.c_str());

  return GetServerUrl(path);
}

mojom::Result PostTransactionCommit::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Unauthorized access");
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

void PostTransactionCommit::Request(
    const std::string& token,
    const std::string& address,
    const std::string& transaction_id,
    PostTransactionCommitCallback callback) {
  auto url_callback = std::bind(&PostTransactionCommit::OnRequest,
      this,
      _1,
      callback);

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(address, transaction_id);
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostTransactionCommit::OnRequest(const mojom::UrlResponse& response,
                                      PostTransactionCommitCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
