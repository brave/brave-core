/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_safetynet/post_safetynet.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

// POST /v2/attestations/safetynet
//
// Request body:
// {
//   "paymentIds": [
//     "83b3b77b-e7c3-455b-adda-e476fa0656d2"
//   ]
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "nonce": "c4645786-052f-402f-8593-56af2f7a21ce"
// }

namespace ledger {
namespace endpoint {
namespace promotion {

PostSafetynet::PostSafetynet(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostSafetynet::~PostSafetynet() = default;

std::string PostSafetynet::GetUrl() {
  return GetServerUrl("/v2/attestations/safetynet");
}

std::string PostSafetynet::GeneratePayload() {
  auto payment_id = base::Value(ledger_->state()->GetPaymentId());
  base::Value payment_ids(base::Value::Type::LIST);
  payment_ids.Append(std::move(payment_id));

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetKey("paymentIds", std::move(payment_ids));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

ledger::Result PostSafetynet::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Invalid token");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result PostSafetynet::ParseBody(
    const std::string& body,
    std::string* nonce) {
  DCHECK(nonce);

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

  auto* nonce_string = dictionary->FindStringKey("nonce");
  if (!nonce_string) {
    BLOG(0, "Nonce is wrong");
    return ledger::Result::LEDGER_ERROR;
  }

  *nonce = *nonce_string;
  return ledger::Result::LEDGER_OK;
}

void PostSafetynet::Request(PostSafetynetCallback callback) {
  auto url_callback = std::bind(&PostSafetynet::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(),
      {},
      GeneratePayload(),
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void PostSafetynet::OnRequest(
    const ledger::UrlResponse& response,
    PostSafetynetCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  std::string nonce;
  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, nonce);
    return;
  }

  result = ParseBody(response.body, &nonce);
  callback(result, nonce);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
