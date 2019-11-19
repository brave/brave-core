/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/attestation/attestation_iosx.h"
#include "bat/ledger/internal/request/attestation_requests.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_attestation {

AttestationIOS::AttestationIOS(bat_ledger::LedgerImpl* ledger) :
    Attestation(ledger) {
}

AttestationIOS::~AttestationIOS() = default;

std::string AttestationIOS::ParseStartPayload(
    const std::string& response) {
  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return "";
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return "";
  }

  auto* key = dictionary->FindKey("publicKey");
  if (!key || !key->is_string()) {
    return "";
  }

  return key->GetString();
}

void AttestationIOS::ParseClaimSolution(
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

  auto* blob = dictionary->FindKey("blob");
  if (!blob || !blob->is_string()) {
    return;
  }

  auto* signature = dictionary->FindKey("signature");
  if (!signature || !signature->is_string()) {
    return;
  }

  result->SetStringKey("nonce", nonce->GetString());
  result->SetStringKey("blob", blob->GetString());
  result->SetStringKey("signature", signature->GetString());
}

void AttestationIOS::Start(
    const std::string& payload,
    StartCallback callback) {
  const std::string key = ParseStartPayload(payload);
  const std::string payment_id = ledger_->GetPaymentId();

  if (key.empty()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("publicKeyHash", key);
  dictionary.SetStringKey("paymentId", payment_id);
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  auto url_callback = std::bind(&AttestationIOS::OnStart,
      this,
      _1,
      _2,
      _3,
      callback);

  const std::string url = braveledger_request_util::GetStartAttestationIOSUrl();

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void AttestationIOS::OnStart(
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

void AttestationIOS::Confirm(
    const std::string& solution,
    ConfirmCallback callback) {
  base::Value parsed_solution(base::Value::Type::DICTIONARY);
  ParseClaimSolution(solution, &parsed_solution);

  if (parsed_solution.DictSize() != 3) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("attestationBlob",
      *parsed_solution.FindStringKey("blob"));
  dictionary.SetStringKey("signature",
      *parsed_solution.FindStringKey("signature"));
  std::string payload;
  base::JSONWriter::Write(dictionary, &payload);

  const std::string nonce = *parsed_solution.FindStringKey("nonce");
  const std::string url =
      braveledger_request_util::GetConfirmAttestationIOSUrl(nonce);

  auto url_callback = std::bind(&AttestationIOS::OnConfirm,
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

void AttestationIOS::OnConfirm(
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
