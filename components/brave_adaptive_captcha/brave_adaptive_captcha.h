/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_H_
#define BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_H_

#include <memory>
#include <string>

#include "brave/components/brave_adaptive_captcha/environment.h"
#include "brave/components/brave_adaptive_captcha/get_adaptive_captcha_challenge.h"
#include "brave/components/brave_adaptive_captcha/url_loader.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_adaptive_captcha {

// This manages the adaptive captcha functionality.
class BraveAdaptiveCaptcha {
 public:
  explicit BraveAdaptiveCaptcha(content::BrowserContext* context);
  ~BraveAdaptiveCaptcha();

  BraveAdaptiveCaptcha(const BraveAdaptiveCaptcha&) = delete;
  BraveAdaptiveCaptcha& operator=(const BraveAdaptiveCaptcha&) = delete;

  // Returns the appropriate url for downloading a scheduled captcha with given
  // |captcha_id| for the given |payment_id|.
  static std::string GetScheduledCaptchaUrl(const std::string& payment_id,
                                            const std::string& captcha_id);

  // Retrieves the captcha scheduled for the given |payment_id|, if
  // any. If there is a scheduled captcha that the user must solve in
  // order to proceed, |callback| will return the captcha id;
  // otherwise, |callback| will return the empty string.
  void GetScheduledCaptcha(const std::string& payment_id,
                           OnGetAdaptiveCaptchaChallenge callback);

  Environment environment() const { return environment_; }
  void set_environment(Environment environment) { environment_ = environment; }

 private:
  static Environment environment_;
  content::BrowserContext* context_;
  UrlLoader url_loader_;
  std::unique_ptr<GetAdaptiveCaptchaChallenge> captcha_challenge_;
};

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_H_
