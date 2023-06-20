/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_

#include <string>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

// GET /v1/captchas/{captcha_id}.png
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {PNG data}

namespace brave_rewards::internal::endpoint::promotion {

using GetCaptchaCallback =
    base::OnceCallback<void(mojom::Result result, const std::string& image)>;

class GetCaptcha {
 public:
  void Request(const std::string& captcha_id, GetCaptchaCallback callback);

 private:
  std::string GetUrl(const std::string& captcha_id);

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body, std::string* image);

  void OnRequest(GetCaptchaCallback callback, mojom::UrlResponsePtr response);
};

}  // namespace brave_rewards::internal::endpoint::promotion

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_
