/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/payment/post_transaction_anon/post_transaction_anon.h"

#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/payment/payment_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/common/request_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace payment {

PostTransactionAnon::PostTransactionAnon(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostTransactionAnon::~PostTransactionAnon() = default;

std::string PostTransactionAnon::GetUrl(const std::string& order_id) {
  const std::string path = base::StringPrintf(
      "/v1/orders/%s/transactions/anonymousCard",
      order_id.c_str());

  return GetServerUrl(path);
}

std::string PostTransactionAnon::GeneratePayload(
    const double amount,
    const std::string& order_id,
    const std::string& destination) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value denomination(base::Value::Type::DICTIONARY);
  denomination.SetStringKey("amount", base::StringPrintf("%g", amount));
  denomination.SetStringKey("currency", "BAT");

  base::Value octets(base::Value::Type::DICTIONARY);
  octets.SetKey("denomination", std::move(denomination));
  octets.SetStringKey("destination", destination);

  std::string octets_json;
  base::JSONWriter::Write(octets, &octets_json);

  const auto sign_headers = util::GetSignHeaders(
      order_id,
      octets_json,
      "primary",
      wallet->recovery_seed,
      true);

  base::Value headers(base::Value::Type::DICTIONARY);
  headers.SetStringKey("digest", sign_headers.at("digest"));
  headers.SetStringKey("idempotency-key", order_id);
  headers.SetStringKey("signature", sign_headers.at("signature"));

  base::Value transaction(base::Value::Type::DICTIONARY);
  transaction.SetKey("headers", std::move(headers));
  transaction.SetStringKey("octets", octets_json);

  std::string transaction_json;
  base::JSONWriter::Write(transaction, &transaction_json);
  std::string transaction_base64;
  base::Base64Encode(transaction_json, &transaction_base64);

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", wallet->payment_id);
  body.SetStringKey("kind", "anonymous-card");
  body.SetStringKey("transaction", transaction_base64);

  std::string json;
  base::JSONWriter::Write(body, &json);
  return json;
}

type::Result PostTransactionAnon::CheckStatusCode(const int status_code) {
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

void PostTransactionAnon::Request(
    const double amount,
    const std::string& order_id,
    const std::string& destination,
    PostTransactionAnonCallback callback) {
  auto url_callback = std::bind(&PostTransactionAnon::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(order_id);
  request->content = GeneratePayload(amount, order_id, destination);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostTransactionAnon::OnRequest(
    const type::UrlResponse& response,
    PostTransactionAnonCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
