/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"

#include <algorithm>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_adaptive_captcha/pref_names.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

std::string GetScheduledCaptchaUrl(const std::string& payment_id,
                                   const std::string& captcha_id) {
  DCHECK(!payment_id.empty());
  DCHECK(!captcha_id.empty());

  const std::string path = base::StringPrintf(
      "/v3/captcha/%s/%s", payment_id.c_str(), captcha_id.c_str());
  return brave_adaptive_captcha::ServerUtil::GetInstance()->GetServerUrl(path);
}

}  // namespace

namespace brave_adaptive_captcha {

constexpr int kScheduledCaptchaMaxFailedAttempts = 10;

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

BraveAdaptiveCaptchaService::BraveAdaptiveCaptchaService(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    brave_rewards::RewardsService* rewards_service,
    std::unique_ptr<BraveAdaptiveCaptchaDelegate> delegate)
    : prefs_(prefs),
      rewards_service_(rewards_service),
      delegate_(std::move(delegate)),
      api_request_helper_(kAnnotationTag, url_loader_factory),
      get_captcha_challenge_(
          std::make_unique<GetAdaptiveCaptchaChallenge>(&api_request_helper_)) {
  DCHECK(prefs);
  DCHECK(rewards_service);

  rewards_service_->AddObserver(this);
}

BraveAdaptiveCaptchaService::~BraveAdaptiveCaptchaService() {
  rewards_service_->RemoveObserver(this);
}

void BraveAdaptiveCaptchaService::GetScheduledCaptcha(
    const std::string& payment_id,
    OnGetAdaptiveCaptchaChallenge callback) {
  get_captcha_challenge_->Request(payment_id, std::move(callback));
}

bool BraveAdaptiveCaptchaService::GetScheduledCaptchaInfo(
    std::string* url,
    bool* max_attempts_exceeded) {
  DCHECK(url);
  DCHECK(max_attempts_exceeded);

  const std::string payment_id =
      prefs_->GetString(prefs::kScheduledCaptchaPaymentId);
  const std::string captcha_id = prefs_->GetString(prefs::kScheduledCaptchaId);
  if (payment_id.empty() || captcha_id.empty()) {
    return false;
  }

  const int failed_attempts =
      prefs_->GetInteger(prefs::kScheduledCaptchaFailedAttempts);

  *url = GetScheduledCaptchaUrl(payment_id, captcha_id);
  *max_attempts_exceeded =
      failed_attempts >= kScheduledCaptchaMaxFailedAttempts;

  return true;
}

void BraveAdaptiveCaptchaService::UpdateScheduledCaptchaResult(bool result) {
  if (!result) {
    const int failed_attempts =
        prefs_->GetInteger(prefs::kScheduledCaptchaFailedAttempts) + 1;
    prefs_->SetInteger(
        prefs::kScheduledCaptchaFailedAttempts,
        std::min(failed_attempts, kScheduledCaptchaMaxFailedAttempts));
    if (failed_attempts >= kScheduledCaptchaMaxFailedAttempts) {
      prefs_->SetBoolean(prefs::kScheduledCaptchaPaused, true);
    }
    return;
  }

  ClearScheduledCaptcha();
}

void BraveAdaptiveCaptchaService::ShowScheduledCaptcha(
    const std::string& payment_id,
    const std::string& captcha_id) {
  if (prefs_->GetBoolean(prefs::kScheduledCaptchaPaused)) {
    return;
  }

  prefs_->SetString(prefs::kScheduledCaptchaPaymentId, payment_id);
  prefs_->SetString(prefs::kScheduledCaptchaId, captcha_id);

  if (delegate_) {
    delegate_->ShowScheduledCaptcha(payment_id, captcha_id);
    return;
  }
}

void BraveAdaptiveCaptchaService::SnoozeScheduledCaptcha() {
  prefs_->SetString(prefs::kScheduledCaptchaPaymentId, "");
  prefs_->SetString(prefs::kScheduledCaptchaId, "");
}

void BraveAdaptiveCaptchaService::ClearScheduledCaptcha() {
  prefs_->SetInteger(prefs::kScheduledCaptchaFailedAttempts, 0);
  prefs_->SetString(prefs::kScheduledCaptchaPaymentId, "");
  prefs_->SetString(prefs::kScheduledCaptchaId, "");
  prefs_->SetBoolean(prefs::kScheduledCaptchaPaused, false);
}

void BraveAdaptiveCaptchaService::OnCompleteReset(const bool success) {
  ClearScheduledCaptcha();
}

// static
void BraveAdaptiveCaptchaService::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kScheduledCaptchaId, "");
  registry->RegisterStringPref(prefs::kScheduledCaptchaPaymentId, "");
  registry->RegisterIntegerPref(prefs::kScheduledCaptchaSnoozeCount, 0);
  registry->RegisterIntegerPref(prefs::kScheduledCaptchaFailedAttempts, 0);
  registry->RegisterBooleanPref(prefs::kScheduledCaptchaPaused, false);
}

}  // namespace brave_adaptive_captcha
