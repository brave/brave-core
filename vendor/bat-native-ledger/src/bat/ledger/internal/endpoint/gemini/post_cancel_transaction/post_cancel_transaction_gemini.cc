/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/gemini/post_cancel_transaction/post_cancel_transaction_gemini.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_utils.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

std::string GetUrl(const std::string& tx_ref) {
  return ledger::endpoint::gemini::GetApiServerUrl(
      base::StringPrintf("/v1/payment/cancel/%s", tx_ref.c_str()));
}

}  // namespace

namespace ledger {
namespace endpoint {
namespace gemini {

PostCancelTransaction::PostCancelTransaction(LedgerImpl* ledger)
    : ledger_(ledger) {
  DCHECK(ledger_);
}

PostCancelTransaction::~PostCancelTransaction() = default;

void PostCancelTransaction::Request(const std::string& token,
                                    const std::string& tx_ref,
                                    PostCancelTransactionCallback callback) {
  auto url_callback =
      std::bind(&PostCancelTransaction::OnRequest, this, _1, callback);
  auto request = type::UrlRequest::New();
  request->url = GetUrl(tx_ref);
  request->method = type::UrlMethod::POST;
  request->headers = RequestAuthorization(token);
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostCancelTransaction::OnRequest(const type::UrlResponse& response,
                                      PostCancelTransactionCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  callback(result);
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
