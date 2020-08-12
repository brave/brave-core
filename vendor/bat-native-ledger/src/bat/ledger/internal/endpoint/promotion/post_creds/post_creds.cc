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
#include "bat/ledger/internal/request/request_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

// POST /v1/promotions/{promotion_id}
//
// Request body:
// {
//   "paymentId": "ff50981d-47de-4210-848d-995e186901a1",
//   "blindedCreds": [
//     "wqto9FnferrKUM0lcp2B0lecMQwArvUq3hWGCYlXiQo=",
//     "ZiSXpF61aZ/tL2MxkKzI5Vnw2aLJE2ln2FMHAtKc9Co="
//   ]
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_FORBIDDEN (403)
// HTTP_CONFLICT (409)
// HTTP_GONE (410)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {
//   "claimId": "53714048-9675-419e-baa3-369d85a2facb"
// }

namespace ledger {
namespace endpoint {
namespace promotion {

PostCreds::PostCreds(bat_ledger::LedgerImpl* ledger):
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

std::string PostCreds::GeneratePayload(
    std::unique_ptr<base::ListValue> blinded_creds) {
  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", ledger_->state()->GetPaymentId());
  body.SetKey("blindedCreds", base::Value(std::move(*blinded_creds)));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

ledger::Result PostCreds::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Signature validation failed");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Incorrect blinded credentials");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_GONE) {
    BLOG(0, "Promotion is gone");
    return ledger::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result PostCreds::ParseBody(
    const std::string& body,
    std::string* claim_id) {
  DCHECK(claim_id);

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

  auto* id = dictionary->FindStringKey("claimId");
  if (!id || id->empty()) {
    BLOG(0, "Claim id is missing");
    return ledger::Result::LEDGER_ERROR;
  }

  *claim_id = *id;

  return ledger::Result::LEDGER_OK;
}

void PostCreds::Request(
    const std::string& promotion_id,
    std::unique_ptr<base::ListValue> blinded_creds,
    PostCredsCallback callback) {
  if (!blinded_creds) {
    BLOG(0, "Blinded creds are null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  const std::string& payload = GeneratePayload(std::move(blinded_creds));

  const auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v1/promotions/" + promotion_id,
      payload,
      ledger_->state()->GetPaymentId(),
      ledger_->state()->GetRecoverySeed());

  auto url_callback = std::bind(&PostCreds::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(promotion_id),
      headers,
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void PostCreds::OnRequest(
    const ledger::UrlResponse& response,
    PostCredsCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  std::string claim_id;
  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, claim_id);
    return;
  }

  result = ParseBody(response.body, &claim_id);
  callback(result, claim_id);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
