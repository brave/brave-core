/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/attestation/attestation_desktop.h"
#include "bat/ledger/internal/request/attestation_requests.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_attestation {

AttestationDesktop::AttestationDesktop(bat_ledger::LedgerImpl* ledger) :
    Attestation(ledger) {
}

AttestationDesktop::~AttestationDesktop() = default;

void AttestationDesktop::ParseCaptchaResponse(
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

  auto* captcha_image = dictionary->FindKey("captchaId");
  if (!captcha_image || !captcha_image->is_string()) {
    return;
  }

  auto* hint = dictionary->FindKey("hint");
  if (!hint || !hint->is_string()) {
    return;
  }

  result->SetStringKey("hint", dictionary->FindKey("hint")->GetString());
  result->SetStringKey(
      "captchaId",
      dictionary->FindKey("captchaId")->GetString());
}

void AttestationDesktop::Start(
    const std::string& payload,
    StartCallback callback) {

  auto url_callback = std::bind(&AttestationDesktop::OnStart,
      this,
      _1,
      _2,
      _3,
      callback);

  const std::string url =
      braveledger_request_util::GetStartAttestationDesktopUrl();

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

void AttestationDesktop::OnStart(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    StartCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  DownloadCaptchaImage(ledger::Result::LEDGER_OK, response, callback);
}

void AttestationDesktop::DownloadCaptchaImage(
    const ledger::Result result,
    const std::string& response,
    StartCallback callback) {
  base::Value dictionary(base::Value::Type::DICTIONARY);
  ParseCaptchaResponse(response, &dictionary);

  if (dictionary.DictEmpty()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  const auto* id = dictionary.FindStringKey("captchaId");
  if (!id) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  const std::string url = braveledger_request_util::GetCaptchaUrl(*id);
  auto url_callback = std::bind(&AttestationDesktop::OnDownloadCaptchaImage,
      this,
      _1,
      _2,
      _3,
      response,
      callback);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void AttestationDesktop::OnDownloadCaptchaImage(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& captcha_response,
    StartCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  base::Value dictionary(base::Value::Type::DICTIONARY);
  ParseCaptchaResponse(captcha_response, &dictionary);

  if (response_status_code != net::HTTP_OK || dictionary.DictEmpty()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  std::string encoded_image;
  base::Base64Encode(response, &encoded_image);
  encoded_image =
      base::StringPrintf("data:image/jpeg;base64,%s", encoded_image.c_str());
  dictionary.SetStringKey("captcha_image", encoded_image);

  std::string json;
  base::JSONWriter::Write(dictionary, &json);
  callback(ledger::Result::LEDGER_OK, json);
}

void AttestationDesktop::Confirm(
    const std::string& result,
    ConfirmCallback callback) {

}

}  // namespace braveledger_attestation
