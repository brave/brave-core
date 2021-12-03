/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_delegate.h"
#include "brave/components/brave_adaptive_captcha/get_adaptive_captcha_challenge.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "components/keyed_service/core/keyed_service.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefRegistrySimple;
class PrefService;

namespace brave_adaptive_captcha {

extern const char kScheduledCaptchaId[];
extern const char kScheduledCaptchaPaymentId[];
extern const char kScheduledCaptchaSnoozeCount[];
extern const char kScheduledCaptchaFailedAttempts[];
extern const char kScheduledCaptchaPaused[];

// This manages the adaptive captcha functionality. Adaptive captchas provide a
// mechanism for the server to provide new types of captchas without requiring
// client changes.
class BraveAdaptiveCaptchaService
    : public KeyedService,
      public brave_rewards::RewardsServiceObserver {
 public:
  BraveAdaptiveCaptchaService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      brave_rewards::RewardsService* rewards_service,
      std::unique_ptr<BraveAdaptiveCaptchaDelegate> delegate);
  ~BraveAdaptiveCaptchaService() override;

  BraveAdaptiveCaptchaService(const BraveAdaptiveCaptchaService&) = delete;
  BraveAdaptiveCaptchaService& operator=(const BraveAdaptiveCaptchaService&) =
      delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Retrieves the captcha scheduled for the given |payment_id|, if
  // any. If there is a scheduled captcha that the user must solve in
  // order to proceed, |callback| will return the captcha id;
  // otherwise, |callback| will return the empty string.
  void GetScheduledCaptcha(const std::string& payment_id,
                           OnGetAdaptiveCaptchaChallenge callback);

  // Gets the metadata associated with the currently scheduled captcha.
  bool GetScheduledCaptchaInfo(std::string* url, bool* max_attempts_exceeded);

  // Updates the result for the currently scheduled captcha.
  void UpdateScheduledCaptchaResult(bool result);

  // Shows the scheduled captcha with the given |payment_id| and |captcha_id|.
  void ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id);

  // Snoozes the captcha.
  void SnoozeScheduledCaptcha();

  // Clears the currently scheduled captcha, if any.
  void ClearScheduledCaptcha();

 private:
  // brave_rewards::RewardsServiceObserver:
  void OnRecoverWallet(brave_rewards::RewardsService* rewards_service,
                       const ledger::type::Result result) override;
  void OnCompleteReset(const bool success) override;

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<brave_rewards::RewardsService> rewards_service_ =
      nullptr;  // NOT OWNED
  std::unique_ptr<BraveAdaptiveCaptchaDelegate> delegate_;
  std::unique_ptr<GetAdaptiveCaptchaChallenge> captcha_challenge_;
};

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_SERVICE_H_
