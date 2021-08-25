/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/get_adaptive_captcha_challenge.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_adaptive_captcha {

GetAdaptiveCaptchaChallenge::GetAdaptiveCaptchaChallenge(
    std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper)
    : api_request_helper_(std::move(api_request_helper)) {
  DCHECK(api_request_helper_);
}

GetAdaptiveCaptchaChallenge::~GetAdaptiveCaptchaChallenge() = default;

std::string GetAdaptiveCaptchaChallenge::GetUrl(const std::string& payment_id) {
  const std::string path =
      base::StringPrintf("/v3/captcha/challenge/%s", payment_id.c_str());

  return GetServerUrl(path);
}

bool GetAdaptiveCaptchaChallenge::CheckStatusCode(int status_code) {
  if (status_code == net::HTTP_NOT_FOUND) {
    VLOG(1) << "No captcha scheduled for given payment id";
    return false;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    LOG(ERROR) << "Failed to retrieve the captcha";
    return false;
  }

  if (status_code != net::HTTP_OK) {
    LOG(ERROR) << "Unexpected HTTP status: " << status_code;
    return false;
  }

  return true;
}

bool GetAdaptiveCaptchaChallenge::ParseBody(const std::string& body,
                                            std::string* captcha_id) {
  DCHECK(captcha_id);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    LOG(ERROR) << "Invalid JSON";
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    LOG(ERROR) << "Invalid JSON";
    return false;
  }

  const std::string* captcha_id_value = dictionary->FindStringKey("captchaID");
  if (!captcha_id_value) {
    LOG(ERROR) << "Missing captcha id";
    return false;
  }

  *captcha_id = *captcha_id_value;

  return true;
}

void GetAdaptiveCaptchaChallenge::Request(
    const std::string& payment_id,
    OnGetAdaptiveCaptchaChallenge callback) {
  auto api_request_helper_callback =
      base::BindOnce(&GetAdaptiveCaptchaChallenge::OnResponse,
                     base::Unretained(this), std::move(callback));
  api_request_helper_->Request("GET", GURL(GetUrl(payment_id)), "", "", false,
                               std::move(api_request_helper_callback));
}

void GetAdaptiveCaptchaChallenge::OnResponse(
    OnGetAdaptiveCaptchaChallenge callback,
    int response_code,
    const std::string& response_body,
    const base::flat_map<std::string, std::string>& response_headers) {
  bool result = CheckStatusCode(response_code);
  if (!result) {
    std::move(callback).Run("");
    return;
  }

  std::string captcha_id;
  result = ParseBody(response_body, &captcha_id);
  if (!result) {
    std::move(callback).Run("");
    return;
  }

  std::move(callback).Run(captcha_id);
}

}  // namespace brave_adaptive_captcha
