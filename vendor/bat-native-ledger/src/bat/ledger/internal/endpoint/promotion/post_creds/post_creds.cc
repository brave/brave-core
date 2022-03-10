/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/post_creds/post_creds.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/common/request_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

PostCreds::PostCreds(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostCreds::~PostCreds() = default;

std::string PostCreds::GetUrl(const std::string& promotion_id) {
  const std::string& path = base::StringPrintf(
      "/v1/promotions/%s",
      promotion_id.c_str());

  return GetServerUrl(path);
}

std::string PostCreds::GeneratePayload(base::Value::List&& blinded_creds) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", wallet->payment_id);
  body.SetKey("blindedCreds", base::Value(std::move(blinded_creds)));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

type::Result PostCreds::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Signature validation failed");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Incorrect blinded credentials");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_GONE) {
    BLOG(0, "Promotion is gone");
    return type::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result PostCreds::ParseBody(
    const std::string& body,
    std::string* claim_id) {
  DCHECK(claim_id);

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

  auto* id = dictionary->FindStringKey("claimId");
  if (!id || id->empty()) {
    BLOG(0, "Claim id is missing");
    return type::Result::LEDGER_ERROR;
  }

  *claim_id = *id;

  return type::Result::LEDGER_OK;
}

void PostCreds::Request(const std::string& promotion_id,
                        base::Value::List&& blinded_creds,
                        PostCredsCallback callback) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  const std::string& payload = GeneratePayload(std::move(blinded_creds));

  const auto headers = util::BuildSignHeaders(
      "post /v1/promotions/" + promotion_id,
      payload,
      wallet->payment_id,
      wallet->recovery_seed);

  auto url_callback = std::bind(&PostCreds::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(promotion_id);
  request->content = payload;
  request->headers = headers;
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostCreds::OnRequest(
    const type::UrlResponse& response,
    PostCredsCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  std::string claim_id;
  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, claim_id);
    return;
  }

  result = ParseBody(response.body, &claim_id);
  callback(result, claim_id);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
