/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/attestation/attestation_desktop.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace attestation {

AttestationDesktop::AttestationDesktop(LedgerImpl* ledger) :
    Attestation(ledger),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
}

AttestationDesktop::~AttestationDesktop() = default;

type::Result AttestationDesktop::ParseClaimSolution(
    const std::string& response,
    int* x,
    int* y,
    std::string* captcha_id) {
  DCHECK(x && y && captcha_id);

  absl::optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return type::Result::LEDGER_ERROR;
  }

  const auto* id = dictionary->FindStringKey("captchaId");
  if (!id) {
    BLOG(0, "Captcha id is wrong");
    return type::Result::LEDGER_ERROR;
  }

  const auto x_parse = dictionary->FindIntKey("x");
  if (!x_parse) {
    BLOG(0, "X is wrong");
    return type::Result::LEDGER_ERROR;
  }

  const auto y_parse = dictionary->FindIntKey("y");
  if (!y_parse) {
    BLOG(0, "Y is wrong");
    return type::Result::LEDGER_ERROR;
  }

  *x = *x_parse;
  *y = *y_parse;
  *captcha_id = *id;
  return type::Result::LEDGER_OK;
}

void AttestationDesktop::Start(
    const std::string& payload,
    StartCallback callback) {
  auto url_callback = std::bind(&AttestationDesktop::DownloadCaptchaImage,
      this,
      _1,
      _2,
      _3,
      callback);

  promotion_server_->post_captcha()->Request(url_callback);
}

void AttestationDesktop::DownloadCaptchaImage(
    const type::Result result,
    const std::string& hint,
    const std::string& captcha_id,
    StartCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto url_callback = std::bind(&AttestationDesktop::OnDownloadCaptchaImage,
      this,
      _1,
      _2,
      hint,
      captcha_id,
      callback);

  promotion_server_->get_captcha()->Request(captcha_id, url_callback);
}

void AttestationDesktop::OnDownloadCaptchaImage(
    const type::Result result,
    const std::string& image,
    const std::string& hint,
    const std::string& captcha_id,
    StartCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("hint", hint);
  dictionary.SetStringKey("captchaId", captcha_id);
  dictionary.SetStringKey("captchaImage", image);

  std::string json;
  base::JSONWriter::Write(dictionary, &json);
  callback(type::Result::LEDGER_OK, json);
}

void AttestationDesktop::Confirm(
    const std::string& solution,
    ConfirmCallback callback) {
  int x;
  int y;
  std::string captcha_id;
  const type::Result result =
      ParseClaimSolution(solution, &x, &y, &captcha_id);

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to parse solution");
    callback(result);
    return;
  }

  auto url_callback = std::bind(&AttestationDesktop::OnConfirm,
      this,
      _1,
      callback);

  promotion_server_->put_captcha()->Request(
      x,
      y,
      captcha_id,
      url_callback);
}

void AttestationDesktop::OnConfirm(
    const type::Result result,
    ConfirmCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to confirm attestation");
    callback(result);
    return;
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace attestation
}  // namespace ledger
