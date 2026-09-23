/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_DELEGATE_H_

#include <string>

namespace brave_adaptive_captcha {

class BraveAdaptiveCaptchaDelegate {
 public:
  virtual ~BraveAdaptiveCaptchaDelegate() = default;

  virtual bool ShowScheduledCaptcha(const std::string& payment_id,
                                    const std::string& captcha_id) = 0;
};

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_DELEGATE_H_
