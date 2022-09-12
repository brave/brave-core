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
  base::Value::Dict payload;
  payload.Set("currency_code", "BAT");
  payload.Set("amount", base::StringPrintf("%f", transaction.amount));
  payload.Set("dry_run", dry_run);
  payload.Set("deposit_id", transaction.address);
  payload.Set("transfer_id", base::GenerateGUID());
  if (dry_run) {
    base::Value::Dict dry_run_option;
    dry_run_option.Set("request_api_transfer_status", "SUCCESS");
    dry_run_option.Set("process_time_sec", 5);
    dry_run_option.Set("status_api_transfer_status", "SUCCESS");
    payload.Set("dry_run_option", std::move(dry_run_option));
  }

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

mojom::Result PostTransaction::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Unauthorized access");
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Account not found");
    return mojom::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Conflict");
    return mojom::Result::IN_PROGRESS;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

mojom::Result PostTransaction::ParseBody(const std::string& body,
                                         std::string* transfer_id,
                                         std::string* transfer_status,
                                         std::string* message) {
  DCHECK(transfer_id);
  DCHECK(transfer_status);
  DCHECK(message);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* transfer_id_str = dict.FindString("transfer_id");
  if (!transfer_id_str) {
    BLOG(0, "Missing transfer id");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* transfer_status_str = dict.FindString("transfer_status");
  if (!transfer_status_str) {
    BLOG(0, "Missing transfer status");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* message_str = dict.FindString("message");

  *transfer_id = *transfer_id_str;
  *transfer_status = *transfer_status_str;
  *message = message_str ? *message_str : "";

  return mojom::Result::LEDGER_OK;
}

void PostTransaction::Request(
    const std::string& token,
    const ::ledger::bitflyer::Transaction& transaction,
    const bool dry_run,
    PostTransactionCallback callback) {
  auto url_callback =
      std::bind(&PostTransaction::OnRequest, this, _1, callback);

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(transaction, dry_run);
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostTransaction::OnRequest(const mojom::UrlResponse& response,
                                PostTransactionCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  mojom::Result result = CheckStatusCode(response.status_code);

  if (result != mojom::Result::LEDGER_OK &&
      result != mojom::Result::IN_PROGRESS) {
    return callback(result, "");
  }

  std::string id;
  std::string transfer_status;
  std::string message;
  result = ParseBody(response.body, &id, &transfer_status, &message);
  if (result == mojom::Result::LEDGER_OK && transfer_status != "SUCCESS") {
    BLOG(0, "Transfer failed (status: " << transfer_status << ")");
    BLOG_IF(0, !message.empty(), message);

    return callback(transfer_status == "SESSION_TIME_OUT"
                        ? mojom::Result::EXPIRED_TOKEN
                        : mojom::Result::LEDGER_ERROR,
                    "");
  }

  callback(result, id);
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
