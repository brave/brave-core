/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_devicecheck/post_devicecheck.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

// POST /v1/devicecheck/attestations
//
// Request body:
// {
//   "paymentId": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
//   "publicKeyHash": "f3f2f3ffqdwfqwfwqfd"
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

PostDevicecheck::PostDevicecheck(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostDevicecheck::~PostDevicecheck() = default;

std::string PostDevicecheck::GetUrl() {
  return GetServerUrl("/v1/devicecheck/attestations");
}

std::string PostDevicecheck::GeneratePayload(const std::string& key) {
  const std::string payment_id = ledger_->state()->GetPaymentId();
  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("publicKeyHash", key);
  dictionary.SetStringKey("paymentId", payment_id);
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

ledger::Result PostDevicecheck::CheckStatusCode(const int status_code) {
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

ledger::Result PostDevicecheck::ParseBody(
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

void PostDevicecheck::Request(
    const std::string& key,
    PostDevicecheckCallback callback) {
  auto url_callback = std::bind(&PostDevicecheck::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(),
      {},
      GeneratePayload(key),
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void PostDevicecheck::OnRequest(
    const ledger::UrlResponse& response,
    PostDevicecheckCallback callback) {
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
