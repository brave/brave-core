/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_factory.h"

#include <memory>

#include "base/time/time.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/browser/brave_ads/tooltips/ads_tooltips_delegate_impl.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#endif

namespace brave_ads {

// static
AdsService* AdsServiceFactory::GetForProfile(Profile* profile) {
  if (!brave::IsRegularProfile(profile)) {
    return nullptr;
  }

  return static_cast<AdsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AdsServiceFactory* AdsServiceFactory::GetInstance() {
  return base::Singleton<AdsServiceFactory>::get();
}

AdsServiceFactory::AdsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(NotificationDisplayServiceFactory::GetInstance());
  DependsOn(dom_distiller::DomDistillerServiceFactory::GetInstance());
  DependsOn(brave_rewards::RewardsServiceFactory::GetInstance());
  DependsOn(HistoryServiceFactory::GetInstance());
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  DependsOn(brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::
                GetInstance());
#endif
}

AdsServiceFactory::~AdsServiceFactory() {}

KeyedService* AdsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  auto* brave_adaptive_captcha_service =
      brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetInstance()
          ->GetForProfile(profile);
#endif
  std::unique_ptr<AdsServiceImpl> ads_service(
      new AdsServiceImpl(profile,
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
                         brave_adaptive_captcha_service,
                         std::make_unique<AdsTooltipsDelegateImpl>(profile),
#endif
                         HistoryServiceFactory::GetInstance()->GetForProfile(
                             profile, ServiceAccessType::EXPLICIT_ACCESS)));
  return ads_service.release();
}

bool AdsServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace brave_ads
