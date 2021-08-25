/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"

#include <algorithm>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/api_request_helper/api_request_helper.h"
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
  return brave_adaptive_captcha::GetServerUrl(path);
}

}  // namespace

namespace brave_adaptive_captcha {

constexpr int kScheduledCaptchaMaxFailedAttempts = 10;

const char kScheduledCaptchaId[] = "brave.rewards.scheduled_captcha.id";
const char kScheduledCaptchaPaymentId[] =
    "brave.rewards.scheduled_captcha.payment_id";
const char kScheduledCaptchaSnoozeCount[] =
    "brave.rewards.scheduled_captcha.snooze_count";
const char kScheduledCaptchaFailedAttempts[] =
    "brave.rewards.scheduled_captcha.failed_attempts";
const char kScheduledCaptchaPaused[] = "brave.rewards.scheduled_captcha.paused";

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
      captcha_challenge_(std::make_unique<GetAdaptiveCaptchaChallenge>(
          std::make_unique<api_request_helper::APIRequestHelper>(
              kAnnotationTag,
              std::move(url_loader_factory)))) {
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
  captcha_challenge_->Request(payment_id, std::move(callback));
}

bool BraveAdaptiveCaptchaService::GetScheduledCaptchaInfo(
    std::string* url,
    bool* max_attempts_exceeded) {
  DCHECK(url);
  DCHECK(max_attempts_exceeded);

  const std::string payment_id = prefs_->GetString(kScheduledCaptchaPaymentId);
  const std::string captcha_id = prefs_->GetString(kScheduledCaptchaId);
  if (payment_id.empty() || captcha_id.empty()) {
    return false;
  }

  const int failed_attempts =
      prefs_->GetInteger(kScheduledCaptchaFailedAttempts);

  *url = GetScheduledCaptchaUrl(payment_id, captcha_id);
  *max_attempts_exceeded =
      failed_attempts >= kScheduledCaptchaMaxFailedAttempts;

  return true;
}

void BraveAdaptiveCaptchaService::UpdateScheduledCaptchaResult(bool result) {
  if (!result) {
    const int failed_attempts =
        prefs_->GetInteger(kScheduledCaptchaFailedAttempts) + 1;
    prefs_->SetInteger(
        kScheduledCaptchaFailedAttempts,
        std::min(failed_attempts, kScheduledCaptchaMaxFailedAttempts));
    if (failed_attempts >= kScheduledCaptchaMaxFailedAttempts) {
      prefs_->SetBoolean(kScheduledCaptchaPaused, true);
    }
    return;
  }

  ClearScheduledCaptcha();
}

void BraveAdaptiveCaptchaService::ShowScheduledCaptcha(
    const std::string& payment_id,
    const std::string& captcha_id) {
  if (prefs_->GetBoolean(kScheduledCaptchaPaused)) {
    return;
  }

  prefs_->SetString(kScheduledCaptchaPaymentId, payment_id);
  prefs_->SetString(kScheduledCaptchaId, captcha_id);

  if (delegate_) {
    delegate_->ShowScheduledCaptcha(payment_id, captcha_id);
    return;
  }
}

void BraveAdaptiveCaptchaService::SnoozeScheduledCaptcha() {
  const int snooze_count = prefs_->GetInteger(kScheduledCaptchaSnoozeCount);
  if (snooze_count >= 1) {
    return;
  }

  prefs_->SetString(kScheduledCaptchaPaymentId, "");
  prefs_->SetString(kScheduledCaptchaId, "");
  prefs_->SetInteger(kScheduledCaptchaSnoozeCount, snooze_count + 1);
}

void BraveAdaptiveCaptchaService::ClearScheduledCaptcha() {
  prefs_->SetInteger(kScheduledCaptchaFailedAttempts, 0);
  prefs_->SetInteger(kScheduledCaptchaSnoozeCount, 0);
  prefs_->SetString(kScheduledCaptchaPaymentId, "");
  prefs_->SetString(kScheduledCaptchaId, "");
  prefs_->SetBoolean(kScheduledCaptchaPaused, false);
}

void BraveAdaptiveCaptchaService::OnRecoverWallet(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result) {
  ClearScheduledCaptcha();
}

void BraveAdaptiveCaptchaService::OnCompleteReset(const bool success) {
  ClearScheduledCaptcha();
}

// static
void BraveAdaptiveCaptchaService::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kScheduledCaptchaId, "");
  registry->RegisterStringPref(kScheduledCaptchaPaymentId, "");
  registry->RegisterIntegerPref(kScheduledCaptchaSnoozeCount, 0);
  registry->RegisterIntegerPref(kScheduledCaptchaFailedAttempts, 0);
  registry->RegisterBooleanPref(kScheduledCaptchaPaused, false);
}

}  // namespace brave_adaptive_captcha
