/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_adaptive_captcha/environment.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_adaptive_captcha {

Environment BraveAdaptiveCaptcha::environment_ = DEVELOPMENT;

net::NetworkTrafficAnnotationTag kAnnotationTag =
    net::DefineNetworkTrafficAnnotation("brave_adaptive_captcha_service", R"(
        semantics {
          sender:
            "Brave Adaptive Captcha service"
          description:
            "Fetches CAPTCHA data from Brave."
          trigger:
            "The Brave service indicates that it's time to solve a CAPTCHA."
          data: "Brave CAPTCHA data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");

BraveAdaptiveCaptcha::BraveAdaptiveCaptcha(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(kAnnotationTag, url_loader_factory),
      captcha_challenge_(
          std::make_unique<GetAdaptiveCaptchaChallenge>(&api_request_helper_)) {
}

BraveAdaptiveCaptcha::~BraveAdaptiveCaptcha() = default;

void BraveAdaptiveCaptcha::GetScheduledCaptcha(
    const std::string& payment_id,
    OnGetAdaptiveCaptchaChallenge callback) {
  captcha_challenge_->Request(environment_, payment_id, std::move(callback));
}

// static
std::string BraveAdaptiveCaptcha::GetScheduledCaptchaUrl(
    const std::string& payment_id,
    const std::string& captcha_id) {
  DCHECK(!payment_id.empty());
  DCHECK(!captcha_id.empty());

  const std::string path = base::StringPrintf(
      "/v3/captcha/%s/%s", payment_id.c_str(), captcha_id.c_str());
  return GetServerUrl(environment_, path);
}

}  // namespace brave_adaptive_captcha
