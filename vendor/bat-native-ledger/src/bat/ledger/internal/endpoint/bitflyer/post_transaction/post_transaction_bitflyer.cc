/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/bitflyer/post_transaction/post_transaction_bitflyer.h"

#include <utility>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace bitflyer {

PostTransaction::PostTransaction(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostTransaction::~PostTransaction() = default;

std::string PostTransaction::GetUrl() {
  return GetServerUrl("/api/link/v1/coin/withdraw-to-deposit-id/request");
}

std::string PostTransaction::GeneratePayload(
    const ::ledger::bitflyer::Transaction& transaction,
    const bool dry_run) {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("currency_code", "BAT");
  payload.SetStringKey("amount", base::StringPrintf("%f", transaction.amount));
  payload.SetBoolKey("dry_run", false);
  payload.SetStringKey("deposit_id", transaction.address);
  payload.SetStringKey("transfer_id", base::GenerateGUID());
  if (dry_run) {
    base::Value dry_run_option(base::Value::Type::DICTIONARY);
    dry_run_option.SetStringKey("request_api_transfer_status", "SUCCESS");
    dry_run_option.SetIntKey("process_time_sec", 5);
    dry_run_option.SetStringKey("status_api_transfer_status", "SUCCESS");
    payload.SetKey("dry_run_option", std::move(dry_run_option));
  }

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

type::Result PostTransaction::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    return type::Result::EXPIRED_TOKEN;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Account not found");
    return type::Result::NOT_FOUND;
  }

  if (status_code != net::HTTP_OK) {
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result PostTransaction::ParseBody(const std::string& body,
                                        std::string* transfer_id,
                                        std::string* transfer_status,
                                        std::string* message) {
  DCHECK(transfer_id);
  DCHECK(transfer_status);
  DCHECK(message);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  const auto* transfer_id_str = dictionary->FindStringKey("transfer_id");
  if (!transfer_id_str) {
    BLOG(0, "Missing transfer id");
    return type::Result::LEDGER_ERROR;
  }

  const auto* transfer_status_str =
      dictionary->FindStringKey("transfer_status");
  if (!transfer_status_str) {
    BLOG(0, "Missing transfer status");
    return type::Result::LEDGER_ERROR;
  }

  const auto* message_str = dictionary->FindStringKey("message");

  *transfer_id = *transfer_id_str;
  *transfer_status = *transfer_status_str;
  *message = message_str ? *message_str : "";

  return type::Result::LEDGER_OK;
}

void PostTransaction::Request(
    const std::string& token,
    const ::ledger::bitflyer::Transaction& transaction,
    const bool dry_run,
    PostTransactionCallback callback) {
  auto url_callback =
      std::bind(&PostTransaction::OnRequest, this, _1, callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(transaction, dry_run);
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostTransaction::OnRequest(const type::UrlResponse& response,
                                PostTransactionCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, "");
    return;
  }

  std::string id;
  std::string transfer_status;
  std::string message;
  result = ParseBody(response.body, &id, &transfer_status, &message);
  if (result == type::Result::LEDGER_OK && transfer_status != "SUCCESS") {
    BLOG(0, "Transfer failed (status: " << transfer_status << ")");
    BLOG_IF(0, !message.empty(), message);
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  callback(result, id);
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
