/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_PUT_CAPTCHA_PUT_CAPTCHA_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_PUT_CAPTCHA_PUT_CAPTCHA_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

// PUT /v1/captchas/{captcha_id}
//
// Request body:
// {
//   "solution": {
//     "x": 10,
//     "y": 50
//   }
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_UNAUTHORIZED (401)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response Format (success):
// {Empty}
//
// Response Format (error):
// {
//   "message": "Error solving captcha",
//   "code": 401
// }

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace promotion {

using PutCaptchaCallback = base::OnceCallback<void(mojom::Result)>;

class PutCaptcha {
 public:
  explicit PutCaptcha(RewardsEngineImpl& engine);
  ~PutCaptcha();

  void Request(const int x,
               const int y,
               const std::string& captcha_id,
               PutCaptchaCallback callback);

 private:
  std::string GetUrl(const std::string& captcha_id);

  std::string GeneratePayload(const int x, const int y);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(PutCaptchaCallback callback, mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_PUT_CAPTCHA_PUT_CAPTCHA_H_
