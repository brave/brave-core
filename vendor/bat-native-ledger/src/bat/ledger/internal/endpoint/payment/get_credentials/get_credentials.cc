/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/payment/get_credentials/get_credentials.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/payment/payment_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace payment {

GetCredentials::GetCredentials(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetCredentials::~GetCredentials() = default;

std::string GetCredentials::GetUrl(
    const std::string& order_id,
    const std::string& item_id) {
  const std::string path = base::StringPrintf(
      "/v1/orders/%s/credentials/%s",
      order_id.c_str(),
      item_id.c_str());

  return GetServerUrl(path);
}

type::Result GetCredentials::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_ACCEPTED) {
    return type::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::RETRY;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized claim id");
    return type::Result::RETRY;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return type::Result::RETRY;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::RETRY;
  }

  return type::Result::LEDGER_OK;
}

type::Result GetCredentials::ParseBody(
    const std::string& body,
    type::CredsBatch* batch) {
  DCHECK(batch);
  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::RETRY;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return type::Result::RETRY;
  }

  auto* batch_proof = dictionary->FindStringKey("batchProof");
  if (!batch_proof) {
    BLOG(0, "Missing batch proof");
    return type::Result::RETRY;
  }

  auto* signed_creds = dictionary->FindListKey("signedCreds");
  if (!signed_creds) {
    BLOG(0, "Missing signed creds");
    return type::Result::RETRY;
  }

  auto* public_key = dictionary->FindStringKey("publicKey");
  if (!public_key) {
    BLOG(0, "Missing public key");
    return type::Result::RETRY;
  }

  batch->public_key = *public_key;
  batch->batch_proof = *batch_proof;
  base::JSONWriter::Write(*signed_creds, &batch->signed_creds);

  return type::Result::LEDGER_OK;
}

void GetCredentials::Request(
    const std::string& order_id,
    const std::string& item_id,
    GetCredentialsCallback callback) {
  auto url_callback = std::bind(&GetCredentials::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(order_id, item_id);
  ledger_->LoadURL(std::move(request), url_callback);
}

void GetCredentials::OnRequest(
    const type::UrlResponse& response,
    GetCredentialsCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  type::Result result = CheckStatusCode(response.status_code);
  if (result != type::Result::LEDGER_OK) {
    callback(result, nullptr);
    return;
  }

  auto batch = type::CredsBatch::New();
  result = ParseBody(response.body, batch.get());
  callback(result, std::move(batch));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
