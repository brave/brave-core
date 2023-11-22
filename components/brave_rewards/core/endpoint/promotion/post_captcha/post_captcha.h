/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CAPTCHA_POST_CAPTCHA_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CAPTCHA_POST_CAPTCHA_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

// POST /v1/captchas
//
// Request body:
// {
//   "paymentId": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
//
// Response body:
// {
//   "hint": "circle",
//   "captchaId": "d155d2d2-2627-425b-9be8-44ae9f541762"
// }

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace promotion {

using PostCaptchaCallback =
    base::OnceCallback<void(mojom::Result result,
                            const std::string& hint,
                            const std::string& captcha_id)>;

class PostCaptcha {
 public:
  explicit PostCaptcha(RewardsEngineImpl& engine);
  ~PostCaptcha();

  void Request(PostCaptchaCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload();

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body,
                          std::string* hint,
                          std::string* captcha_id);

  void OnRequest(PostCaptchaCallback callback, mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_CAPTCHA_POST_CAPTCHA_H_
