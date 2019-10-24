/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/attestation/attestation_androidx.h"
#include "bat/ledger/internal/request/attestation_requests.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_attestation {

AttestationAndroid::AttestationAndroid(bat_ledger::LedgerImpl* ledger) :
    Attestation(ledger) {
}

AttestationAndroid::~AttestationAndroid() = default;

void AttestationAndroid::Start(
    const std::string& payload,
    StartCallback callback) {
  auto url_callback = std::bind(&AttestationAndroid::OnStart,
      this,
      _1,
      _2,
      _3,
      callback);

  const std::string url =
      braveledger_request_util::GetStartAttestationAndroidUrl();

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", ledger_->GetPaymentId());

  std::string json;
  base::JSONWriter::Write(body, &json);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void AttestationAndroid::OnStart(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    StartCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, response);
}

void AttestationAndroid::Confirm(
    const std::string& solution,
    ConfirmCallback callback)  {
  // make sure to handle HTTP reponses like before
}

}  // namespace braveledger_attestation
