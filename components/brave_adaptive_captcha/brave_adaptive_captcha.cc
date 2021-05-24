/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_adaptive_captcha/environment.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace brave_adaptive_captcha {

Environment BraveAdaptiveCaptcha::environment_ = DEVELOPMENT;

BraveAdaptiveCaptcha::BraveAdaptiveCaptcha(content::BrowserContext* context)
    : context_(context),
      url_loader_(context_->GetDefaultStoragePartition()
                      ->GetURLLoaderFactoryForBrowserProcess()),
      captcha_challenge_(
          std::make_unique<GetAdaptiveCaptchaChallenge>(&url_loader_)) {}

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
