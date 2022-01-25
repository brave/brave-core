/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_rewards/rewards_panel_helper.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

class CaptchaDelegate
    : public brave_adaptive_captcha::BraveAdaptiveCaptchaDelegate {
 public:
  explicit CaptchaDelegate(content::BrowserContext* context)
      : context_(context) {}

  bool ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id) override {
    return brave_rewards::ShowAdaptiveCaptchaPanel(context_);
  }

 private:
  raw_ptr<content::BrowserContext> context_ = nullptr;
};

}  // namespace

namespace brave_adaptive_captcha {

// static
BraveAdaptiveCaptchaServiceFactory*
BraveAdaptiveCaptchaServiceFactory::GetInstance() {
  return base::Singleton<BraveAdaptiveCaptchaServiceFactory>::get();
}

// static
BraveAdaptiveCaptchaService* BraveAdaptiveCaptchaServiceFactory::GetForProfile(
    Profile* profile) {
  if (!brave::IsRegularProfile(profile)) {
    return nullptr;
  }

  return static_cast<BraveAdaptiveCaptchaService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

BraveAdaptiveCaptchaServiceFactory::BraveAdaptiveCaptchaServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveAdaptiveCaptchaService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(brave_rewards::RewardsServiceFactory::GetInstance());
}

BraveAdaptiveCaptchaServiceFactory::~BraveAdaptiveCaptchaServiceFactory() {}

KeyedService* BraveAdaptiveCaptchaServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto url_loader_factory = context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess();
  return new BraveAdaptiveCaptchaService(
      user_prefs::UserPrefs::Get(context), std::move(url_loader_factory),
      brave_rewards::RewardsServiceFactory::GetForProfile(
          Profile::FromBrowserContext(context)),
      std::make_unique<CaptchaDelegate>(context));
}

}  // namespace brave_adaptive_captcha
