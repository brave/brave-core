/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/payment/payment_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace payment {

PostTransactionGemini::PostTransactionGemini(LedgerImpl* ledger)
    : ledger_(ledger) {
  DCHECK(ledger_);
}

PostTransactionGemini::~PostTransactionGemini() = default;

std::string PostTransactionGemini::GetUrl(const std::string& order_id) {
  const std::string path =
      base::StringPrintf("/v1/orders/%s/transactions/gemini", order_id.c_str());

  return GetServerUrl(path);
}

std::string PostTransactionGemini::GeneratePayload(
    const type::SKUTransaction& transaction) {
  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("externalTransactionId",
                    transaction.external_transaction_id);
  body.SetStringKey("kind", "gemini");

  std::string json;
  base::JSONWriter::Write(body, &json);
  return json;
}

type::Result PostTransactionGemini::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized transaction suffix");
    return type::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "External transaction id already submitted");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_CREATED) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void PostTransactionGemini::Request(const type::SKUTransaction& transaction,
                                    PostTransactionGeminiCallback callback) {
  auto url_callback =
      std::bind(&PostTransactionGemini::OnRequest, this, _1, callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(transaction.order_id);
  request->content = GeneratePayload(transaction);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  BLOG(0, "External Transaction ID: " << transaction.external_transaction_id
                                      << "for " << transaction.amount);

  ledger_->LoadURL(std::move(request), url_callback);
}

void PostTransactionGemini::OnRequest(const type::UrlResponse& response,
                                      PostTransactionGeminiCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  BLOG_IF(0, CheckStatusCode(response.status_code) != type::Result::LEDGER_OK,
          "Error creating gemini transaction on the payment server");
  BLOG_IF(0, CheckStatusCode(response.status_code) == type::Result::LEDGER_OK,
          "Gemini transaction successful on the payment server");

  callback(CheckStatusCode(response.status_code));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
