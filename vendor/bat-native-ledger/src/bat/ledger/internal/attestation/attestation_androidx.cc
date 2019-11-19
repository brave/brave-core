/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
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

void AttestationAndroid::ParseClaimSolution(
    const std::string& response,
    base::Value* result) {
  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return;
  }

  auto* nonce = dictionary->FindKey("nonce");
  if (!nonce || !nonce->is_string()) {
    return;
  }

  auto* token = dictionary->FindKey("token");
  if (!token || !token->is_string()) {
    return;
  }

  result->SetStringKey("nonce", nonce->GetString());
  result->SetStringKey("token", token->GetString());
}

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

  auto payment_id = base::Value(ledger_->GetPaymentId());
  base::Value payment_ids(base::Value::Type::LIST);
  payment_ids.GetList().push_back(std::move(payment_id));

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetKey("paymentIds", std::move(payment_ids));

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
  base::Value parsed_solution(base::Value::Type::DICTIONARY);
  ParseClaimSolution(solution, &parsed_solution);

  if (parsed_solution.DictSize() != 2) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("token", *parsed_solution.FindStringKey("token"));
  std::string payload;
  base::JSONWriter::Write(dictionary, &payload);

  const std::string nonce = *parsed_solution.FindStringKey("nonce");
  const std::string url =
      braveledger_request_util::GetConfirmAttestationAndroidUrl(nonce);

  auto url_callback = std::bind(&AttestationAndroid::OnConfirm,
      this,
      _1,
      _2,
      _3,
      callback);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::PUT,
      url_callback);
}

void AttestationAndroid::OnConfirm(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ConfirmCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_attestation
