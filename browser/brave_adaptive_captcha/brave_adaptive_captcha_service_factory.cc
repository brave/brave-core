/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_delegate.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#endif

namespace {

class CaptchaDelegate
    : public brave_adaptive_captcha::BraveAdaptiveCaptchaDelegate {
 public:
  explicit CaptchaDelegate(content::BrowserContext* context)
      : context_(context) {}

  bool ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id) override {
#if BUILDFLAG(IS_ANDROID)
    return true;
#else
    // Because this is triggered from the adaptive captcha tooltip, this call
    // isn't associated with any particular `Browser` instance and we can use
    // the last active browser for this profile.
    auto* profile = Profile::FromBrowserContext(context_);
    auto* browser = chrome::FindTabbedBrowser(profile, false);
    if (!browser) {
      return false;
    }
    auto* coordinator =
        brave_rewards::RewardsPanelCoordinator::FromBrowser(browser);
    if (!coordinator) {
      return false;
    }
    return coordinator->ShowAdaptiveCaptcha();
#endif
  }

 private:
  raw_ptr<content::BrowserContext> context_ = nullptr;
};

}  // namespace

namespace brave_adaptive_captcha {

// static
BraveAdaptiveCaptchaServiceFactory*
BraveAdaptiveCaptchaServiceFactory::GetInstance() {
  static base::NoDestructor<BraveAdaptiveCaptchaServiceFactory> instance;
  return instance.get();
}

// static
BraveAdaptiveCaptchaService* BraveAdaptiveCaptchaServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile->IsRegularProfile()) {
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

BraveAdaptiveCaptchaServiceFactory::~BraveAdaptiveCaptchaServiceFactory() =
    default;

std::unique_ptr<KeyedService>
BraveAdaptiveCaptchaServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto url_loader_factory = context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess();
  return std::make_unique<BraveAdaptiveCaptchaService>(
      user_prefs::UserPrefs::Get(context), std::move(url_loader_factory),
      brave_rewards::RewardsServiceFactory::GetForProfile(
          Profile::FromBrowserContext(context)),
      std::make_unique<CaptchaDelegate>(context));
}

}  // namespace brave_adaptive_captcha
