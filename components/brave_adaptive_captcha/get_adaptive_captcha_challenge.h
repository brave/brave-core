/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_GET_ADAPTIVE_CAPTCHA_CHALLENGE_H_
#define BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_GET_ADAPTIVE_CAPTCHA_CHALLENGE_H_

#include <string>

#include "base/callback.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_adaptive_captcha/environment.h"
#include "brave/components/brave_adaptive_captcha/url_loader.h"

// GET /v3/captcha/challenge/{payment_id}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_NOT_FOUND (404)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {
//   "captchaID": "ae07288c-d078-11eb-b8bc-0242ac130003"
// }

namespace brave_adaptive_captcha {

using OnGetAdaptiveCaptchaChallenge =
    base::OnceCallback<void(const std::string&)>;

class GetAdaptiveCaptchaChallenge {
 public:
  explicit GetAdaptiveCaptchaChallenge(UrlLoader* url_loader);
  ~GetAdaptiveCaptchaChallenge();

  void Request(Environment environment,
               const std::string& payment_id,
               OnGetAdaptiveCaptchaChallenge callback);

 private:
  std::string GetUrl(Environment environment, const std::string& payment_id);

  bool CheckStatusCode(int status_code);

  bool ParseBody(const std::string& body, std::string* captcha_id);

  void OnRequest(OnGetAdaptiveCaptchaChallenge callback,
                 const UrlLoader::UrlResponse& response_body);

  UrlLoader* url_loader_;
};

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_GET_ADAPTIVE_CAPTCHA_CHALLENGE_H_
