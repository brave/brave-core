/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/attestation/attestation_androidx.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_attestation.h"
#include "bat/ledger/internal/response/response_attestation.h"

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
    BLOG(0, "Parsing of solution failed");
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Parsing of solution failed");
    return;
  }

  const auto* nonce = dictionary->FindStringKey("nonce");
  if (!nonce) {
    BLOG(0, "Nonce is missing");
    return;
  }

  const auto* token = dictionary->FindStringKey("token");
  if (!token) {
    BLOG(0, "Token is missing");
    return;
  }

  result->SetStringKey("nonce", *nonce);
  result->SetStringKey("token", *token);
}

void AttestationAndroid::Start(
    const std::string& payload,
    StartCallback callback) {
  auto url_callback = std::bind(&AttestationAndroid::OnStart,
      this,
      _1,
      callback);

  const std::string url =
      braveledger_request_util::GetStartAttestationAndroidUrl();

  auto payment_id = base::Value(ledger_->state()->GetPaymentId());
  base::Value payment_ids(base::Value::Type::LIST);
  payment_ids.Append(std::move(payment_id));

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetKey("paymentIds", std::move(payment_ids));

  std::string json;
  base::JSONWriter::Write(body, &json);

  ledger_->LoadURL(
      url,
      {},
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void AttestationAndroid::OnStart(
    const ledger::UrlResponse& response,
    StartCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  const ledger::Result result =
      braveledger_response_util::CheckStartAttestation(response);
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Failed to start attestation");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, response.body);
}

void AttestationAndroid::Confirm(
    const std::string& solution,
    ConfirmCallback callback)  {
  base::Value parsed_solution(base::Value::Type::DICTIONARY);
  ParseClaimSolution(solution, &parsed_solution);

  if (parsed_solution.DictSize() != 2) {
    BLOG(0, "Solution is wrong: " << solution);
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
      callback);

  ledger_->LoadURL(
      url,
      {},
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::PUT,
      url_callback);
}

void AttestationAndroid::OnConfirm(
    const ledger::UrlResponse& response,
    ConfirmCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  const ledger::Result result =
      braveledger_response_util::CheckConfirmAttestation(response);
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Failed to confirm attestation");
    callback(result);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_attestation
