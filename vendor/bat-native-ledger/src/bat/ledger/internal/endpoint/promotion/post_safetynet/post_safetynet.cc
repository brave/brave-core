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

namespace ledger {
namespace endpoint {
namespace promotion {

PostSafetynet::PostSafetynet(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostSafetynet::~PostSafetynet() = default;

std::string PostSafetynet::GetUrl() {
  return GetServerUrl("/v2/attestations/safetynet");
}

std::string PostSafetynet::GeneratePayload() {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value payment_ids(base::Value::Type::LIST);
  payment_ids.Append(base::Value(wallet->payment_id));

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetKey("paymentIds", std::move(payment_ids));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

type::Result PostSafetynet::CheckStatusCode(const int status_code) {
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

type::Result PostSafetynet::ParseBody(
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

void PostSafetynet::Request(PostSafetynetCallback callback) {
  auto url_callback = std::bind(&PostSafetynet::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload();
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostSafetynet::OnRequest(
    const type::UrlResponse& response,
    PostSafetynetCallback callback) {
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
