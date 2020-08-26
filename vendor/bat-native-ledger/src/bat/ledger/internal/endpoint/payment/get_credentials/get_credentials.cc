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

GetCredentials::GetCredentials(bat_ledger::LedgerImpl* ledger):
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

ledger::Result GetCredentials::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_ACCEPTED) {
    return ledger::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::RETRY;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized claim id");
    return ledger::Result::RETRY;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::RETRY;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::RETRY;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result GetCredentials::ParseBody(
    const std::string& body,
    ledger::CredsBatch* batch) {
  DCHECK(batch);
  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::RETRY;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::RETRY;
  }

  auto* batch_proof = dictionary->FindStringKey("batchProof");
  if (!batch_proof) {
    BLOG(0, "Missing batch proof");
    return ledger::Result::RETRY;
  }

  auto* signed_creds = dictionary->FindListKey("signedCreds");
  if (!signed_creds) {
    BLOG(0, "Missing signed creds");
    return ledger::Result::RETRY;
  }

  auto* public_key = dictionary->FindStringKey("publicKey");
  if (!public_key) {
    BLOG(0, "Missing public key");
    return ledger::Result::RETRY;
  }

  batch->public_key = *public_key;
  batch->batch_proof = *batch_proof;
  base::JSONWriter::Write(*signed_creds, &batch->signed_creds);

  return ledger::Result::LEDGER_OK;
}

void GetCredentials::Request(
    const std::string& order_id,
    const std::string& item_id,
    GetCredentialsCallback callback) {
  auto url_callback = std::bind(&GetCredentials::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(order_id, item_id),
      {},
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void GetCredentials::OnRequest(
    const ledger::UrlResponse& response,
    GetCredentialsCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  ledger::Result result = CheckStatusCode(response.status_code);
  if (result != ledger::Result::LEDGER_OK) {
    callback(result, nullptr);
    return;
  }

  auto batch = ledger::CredsBatch::New();
  result = ParseBody(response.body, batch.get());
  callback(result, std::move(batch));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
