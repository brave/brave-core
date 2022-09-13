/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/gemini/get_transaction/get_transaction_gemini.h"

#include <utility>

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

GetTransaction::GetTransaction(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

GetTransaction::~GetTransaction() = default;

std::string GetTransaction::GetUrl(const std::string& tx_ref) {
  return GetApiServerUrl(base::StringPrintf(
      "/v1/payment/%s/%s", GetClientId().c_str(), tx_ref.c_str()));
}

mojom::Result GetTransaction::ParseBody(const std::string& body,
                                        std::string* status) {
  DCHECK(status);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* tx_status = dict.FindString("status");
  if (!tx_status || tx_status->empty()) {
    BLOG(0, "Missing transfer status");
    return mojom::Result::LEDGER_ERROR;
  }

  *status = *tx_status;
  return mojom::Result::LEDGER_OK;
}

void GetTransaction::Request(const std::string& token,
                             const std::string& tx_ref,
                             GetTransactionCallback callback) {
  auto url_callback = std::bind(&GetTransaction::OnRequest, this, _1, callback);

  auto request = mojom::UrlRequest::New();

  request->url = GetUrl(tx_ref);
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::GET;

  ledger_->LoadURL(std::move(request), url_callback);
}

void GetTransaction::OnRequest(const mojom::UrlResponse& response,
                               GetTransactionCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  mojom::Result result = CheckStatusCode(response.status_code);

  if (result != mojom::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  std::string status;
  result = ParseBody(response.body, &status);

  if (result == mojom::Result::LEDGER_OK) {
    if (status == "Error") {
      BLOG(0, "Transfer error");
      callback(mojom::Result::LEDGER_ERROR);
      return;
    }

    if (status != "Completed") {
      BLOG(1, "Transfer not yet completed (status: " << status << ")");
      callback(mojom::Result::RETRY);
      return;
    }
  }

  callback(result);
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
