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

namespace ledger {
namespace endpoint {
namespace promotion {

PostDevicecheck::PostDevicecheck(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostDevicecheck::~PostDevicecheck() = default;

std::string PostDevicecheck::GetUrl() {
  return GetServerUrl("/v1/devicecheck/attestations");
}

std::string PostDevicecheck::GeneratePayload(const std::string& key) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("publicKeyHash", key);
  dictionary.SetStringKey("paymentId", wallet->payment_id);
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

type::Result PostDevicecheck::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Invalid token");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result PostDevicecheck::ParseBody(
    const std::string& body,
    std::string* nonce) {
  DCHECK(nonce);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  auto* nonce_string = dictionary->FindStringKey("nonce");
  if (!nonce_string) {
    BLOG(0, "Nonce is wrong");
    return type::Result::LEDGER_ERROR;
  }

  *nonce = *nonce_string;
  return type::Result::LEDGER_OK;
}

void PostDevicecheck::Request(
    const std::string& key,
    PostDevicecheckCallback callback) {
  auto url_callback = std::bind(&PostDevicecheck::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(key);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostDevicecheck::OnRequest(
    const type::UrlResponse& response,
    PostDevicecheckCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  std::string nonce;
  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, nonce);
    return;
  }

  result = ParseBody(response.body, &nonce);
  callback(result, nonce);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
