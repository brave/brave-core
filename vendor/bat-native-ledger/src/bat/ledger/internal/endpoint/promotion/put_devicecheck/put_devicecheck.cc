/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/put_devicecheck/put_devicecheck.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

PutDevicecheck::PutDevicecheck(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PutDevicecheck::~PutDevicecheck() = default;

std::string PutDevicecheck::GetUrl(const std::string& nonce) {
  const std::string path = base::StringPrintf(
      "/v1/devicecheck/attestations/%s",
      nonce.c_str());

  return GetServerUrl(path);
}

std::string PutDevicecheck::GeneratePayload(
    const std::string& blob,
    const std::string& signature) {
  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey("attestationBlob", blob);
  dictionary.SetStringKey("signature", signature);
  std::string payload;
  base::JSONWriter::Write(dictionary, &payload);

  return payload;
}

type::Result PutDevicecheck::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Invalid solution");
    return type::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Failed to verify captcha solution");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void PutDevicecheck::Request(
      const std::string& blob,
      const std::string& signature,
      const std::string& nonce,
      PutDevicecheckCallback callback) {
  auto url_callback = std::bind(&PutDevicecheck::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(nonce);
  request->content = GeneratePayload(blob, signature);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::PUT;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PutDevicecheck::OnRequest(
    const type::UrlResponse& response,
    PutDevicecheckCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
