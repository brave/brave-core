/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/gemini/post_transaction/post_transaction_gemini.h"

#include <utility>

#include "base/base64.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace gemini {

PostTransaction::PostTransaction(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostTransaction::~PostTransaction() = default;

std::string PostTransaction::GetUrl() {
  return GetApiServerUrl("/v1/payments/pay");
}

std::string PostTransaction::GeneratePayload(
    const ::ledger::gemini::Transaction& transaction) {
  base::Value::Dict payload;
  payload.Set("tx_ref", base::GenerateGUID());
  payload.Set("amount", base::StringPrintf("%f", transaction.amount));
  payload.Set("currency", "BAT");
  payload.Set("destination", transaction.address);

  std::string json;
  base::JSONWriter::Write(payload, &json);

  std::string base64;
  base::Base64Encode(base::StringPiece(json), &base64);
  return base64;
}

mojom::Result PostTransaction::ParseBody(const std::string& body,
                                         std::string* transfer_id,
                                         std::string* transfer_status) {
  DCHECK(transfer_id);
  DCHECK(transfer_status);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* transfer_id_str = dict.FindString("tx_ref");
  if (!transfer_id_str || transfer_id_str->empty()) {
    BLOG(0, "Missing transfer id");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* transfer_status_str = dict.FindString("status");
  if (!transfer_status_str) {
    BLOG(0, "Missing transfer status");
    return mojom::Result::LEDGER_ERROR;
  }

  *transfer_id = *transfer_id_str;
  *transfer_status = *transfer_status_str;

  return mojom::Result::LEDGER_OK;
}

void PostTransaction::Request(const std::string& token,
                              const ::ledger::gemini::Transaction& transaction,
                              PostTransactionCallback callback) {
  auto url_callback =
      std::bind(&PostTransaction::OnRequest, this, _1, callback);

  auto request = mojom::UrlRequest::New();
  auto payload = GeneratePayload(transaction);

  request->url = GetUrl();
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  request->headers.push_back("X-GEMINI-PAYLOAD: " + payload);

  BLOG(0, "Initiating gemini transaction to: " << transaction.address << "for "
                                               << transaction.amount);

  ledger_->LoadURL(std::move(request), url_callback);
}

void PostTransaction::OnRequest(const mojom::UrlResponse& response,
                                PostTransactionCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  mojom::Result result = CheckStatusCode(response.status_code);

  BLOG_IF(0, result != mojom::Result::LEDGER_OK, "Gemini transaction failed");
  BLOG_IF(0, result == mojom::Result::LEDGER_OK,
          "Gemini transaction successful");

  if (result != mojom::Result::LEDGER_OK) {
    callback(result, "");
    return;
  }

  std::string id;
  std::string transfer_status;
  result = ParseBody(response.body, &id, &transfer_status);

  if (result == mojom::Result::LEDGER_OK) {
    if (transfer_status == "Error") {
      BLOG(0, "Transfer error");
      callback(mojom::Result::LEDGER_ERROR, "");
      return;
    }

    if (transfer_status != "Completed") {
      BLOG(1, "Transfer not yet completed (status: " << transfer_status << ")");
      callback(mojom::Result::RETRY, id);
      return;
    }
  }

  callback(result, id);
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
