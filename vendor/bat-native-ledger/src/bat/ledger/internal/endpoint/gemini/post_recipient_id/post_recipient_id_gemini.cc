/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/gemini/post_recipient_id/post_recipient_id_gemini.h"

#include <utility>

#include "base/base64.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace gemini {

PostRecipientId::PostRecipientId(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostRecipientId::~PostRecipientId() = default;

std::string PostRecipientId::GetUrl() {
  return GetApiServerUrl("/v1/payments/recipientIds");
}

type::Result PostRecipientId::ParseBody(const std::string& body,
                                        std::string* recipient_id) {
  DCHECK(recipient_id);

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

  const auto* result = dictionary->FindStringKey("result");
  if (!result || *result != "OK") {
    BLOG(0, "Failed creating recipient_id");
    return type::Result::LEDGER_ERROR;
  }

  const auto* id = dictionary->FindStringKey("recipient_id");
  if (!id) {
    BLOG(0, "Response missing a recipient_id");
    return type::Result::LEDGER_ERROR;
  }

  *recipient_id = *id;
  return type::Result::LEDGER_OK;
}

std::string PostRecipientId::GeneratePayload() {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("label", base::GenerateGUID());

  std::string json;
  base::JSONWriter::Write(payload, &json);

  std::string base64;
  base::Base64Encode(json, &base64);
  return base64;
}

void PostRecipientId::Request(const std::string& token,
                              PostRecipientIdCallback callback) {
  auto url_callback =
      std::bind(&PostRecipientId::OnRequest, this, _1, callback);

  auto request = type::UrlRequest::New();
  auto payload = GeneratePayload();

  request->url = GetUrl();
  request->method = type::UrlMethod::POST;
  request->headers = RequestAuthorization(token);
  request->headers.push_back("X-GEMINI-PAYLOAD: " + payload);

  ledger_->LoadURL(std::move(request), url_callback);
}

void PostRecipientId::OnRequest(const type::UrlResponse& response,
                                PostRecipientIdCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  auto header = response.headers.find("www-authenticate");
  if (header != response.headers.end()) {
    std::string auth_header = header->second;
    if (auth_header.find("unverified_account") != std::string::npos) {
      callback(type::Result::NOT_FOUND, "");
      return;
    }
  }

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, "");
    return;
  }

  std::string recipient_id;
  result = ParseBody(response.body, &recipient_id);
  callback(result, recipient_id);
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
