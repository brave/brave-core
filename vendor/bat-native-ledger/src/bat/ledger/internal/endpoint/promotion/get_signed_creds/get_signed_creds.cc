/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_signed_creds/get_signed_creds.h"

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

GetSignedCreds::GetSignedCreds(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetSignedCreds::~GetSignedCreds() = default;

std::string GetSignedCreds::GetUrl(
    const std::string& promotion_id,
    const std::string& claim_id) {
  const std::string& path = base::StringPrintf(
      "/v1/promotions/%s/claims/%s",
      promotion_id.c_str(),
      claim_id.c_str());

  return GetServerUrl(path);
}

ledger::Result GetSignedCreds::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_ACCEPTED) {
    return ledger::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized claim id");
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

ledger::Result GetSignedCreds::ParseBody(
    const std::string& body,
    ledger::CredsBatch* batch) {
  DCHECK(batch);

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

  auto* batch_proof = dictionary->FindStringKey("batchProof");
  if (!batch_proof) {
    BLOG(0, "Missing batch proof");
    return ledger::Result::LEDGER_ERROR;
  }

  auto* signed_creds = dictionary->FindListKey("signedCreds");
  if (!signed_creds) {
    BLOG(0, "Missing signed creds");
    return ledger::Result::LEDGER_ERROR;
  }

  auto* public_key = dictionary->FindStringKey("publicKey");
  if (!public_key) {
    BLOG(0, "Missing public key");
    return ledger::Result::LEDGER_ERROR;
  }

  base::JSONWriter::Write(*signed_creds, &batch->signed_creds);
  batch->public_key = *public_key;
  batch->batch_proof = *batch_proof;

  return ledger::Result::LEDGER_OK;
}

void GetSignedCreds::Request(
    const std::string& promotion_id,
    const std::string& claim_id,
    GetSignedCredsCallback callback) {
  auto url_callback = std::bind(&GetSignedCreds::OnRequest,
      this,
      _1,
      callback);

  ledger_->LoadURL(
      GetUrl(promotion_id, claim_id),
      {},
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void GetSignedCreds::OnRequest(
    const ledger::UrlResponse& response,
    GetSignedCredsCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, nullptr);
    return;
  }

  ledger::CredsBatch batch;
  result = ParseBody(response.body, &batch);
  callback(result, ledger::CredsBatch::New(batch));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
