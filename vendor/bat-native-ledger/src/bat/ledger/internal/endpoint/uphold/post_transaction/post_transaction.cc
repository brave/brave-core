/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/post_transaction/post_transaction.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace uphold {

PostTransaction::PostTransaction(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostTransaction::~PostTransaction() = default;

std::string PostTransaction::GetUrl(const std::string& address) {
  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/transactions",
      address.c_str());

  return GetServerUrl(path);
}

std::string PostTransaction::GeneratePayload(
    const braveledger_uphold::Transaction& transaction) {
  base::Value denomination(base::Value::Type::DICTIONARY);
  denomination.SetDoubleKey("amount", transaction.amount);
  denomination.SetStringKey("currency", "BAT");

  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("destination", transaction.address);
  payload.SetStringKey("message", transaction.message);
  payload.SetKey("denomination", std::move(denomination));

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

ledger::Result PostTransaction::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_ACCEPTED) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result PostTransaction::ParseBody(
    const std::string& body,
    std::string* id) {
  DCHECK(id);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* id_str = dictionary->FindStringKey("id");
  if (!id_str) {
    BLOG(0, "Missing id");
    return ledger::Result::LEDGER_ERROR;
  }

  *id = *id_str;

  return ledger::Result::LEDGER_OK;
}

void PostTransaction::Request(
    const std::string& token,
    const std::string& address,
    const braveledger_uphold::Transaction& transaction,
    PostTransactionCallback callback) {
  auto url_callback = std::bind(&PostTransaction::OnRequest,
      this,
      _1,
      callback);

  auto request = ledger::UrlRequest::New();
  request->url = GetUrl(address);
  request->content = GeneratePayload(transaction);
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = ledger::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostTransaction::OnRequest(
    const ledger::UrlResponse& response,
    PostTransactionCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, "");
    return;
  }

  std::string id;
  result = ParseBody(response.body, &id);
  callback(result, id);
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
