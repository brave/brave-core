/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_factory.h"

#include <memory>

#include "base/threading/sequence_bound.h"
#include "brave/browser/brave_ads/device_id/device_id_impl.h"
#include "brave/browser/brave_federated/brave_federated_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "brave/components/brave_federated/brave_federated_service.h"
#include "brave/components/brave_federated/data_store_service.h"
#include "brave/components/brave_federated/notification_ad_task_constants.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#endif

namespace brave_ads {

// static
AdsService* AdsServiceFactory::GetForProfile(Profile* profile) {
  if (!brave_rewards::IsSupportedForProfile(profile)) {
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
  DependsOn(brave_federated::BraveFederatedServiceFactory::GetInstance());
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  DependsOn(brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::
                GetInstance());
#endif
}

AdsServiceFactory::~AdsServiceFactory() = default;

std::unique_ptr<AdsTooltipsDelegateImpl>
AdsServiceFactory::CreateAdsTooltipsDelegate(Profile* profile) const {
#if BUILDFLAG(IS_ANDROID)
  return nullptr;
#else
  return std::make_unique<AdsTooltipsDelegateImpl>(profile);
#endif
}

KeyedService* AdsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  auto* brave_adaptive_captcha_service =
      brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetInstance()
          ->GetForProfile(profile);
#endif
  brave_federated::AsyncDataStore* notification_ad_async_data_store = nullptr;
  auto* federated_service =
      brave_federated::BraveFederatedServiceFactory::GetForBrowserContext(
          profile);
  if (federated_service) {
    notification_ad_async_data_store =
        federated_service->GetDataStoreService()->GetDataStore(
            brave_federated::kNotificationAdTaskName);
  }

  auto* history_service = HistoryServiceFactory::GetInstance()->GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);

  auto* rewards_service =
      brave_rewards::RewardsServiceFactory::GetInstance()->GetForProfile(
          profile);

  std::unique_ptr<AdsServiceImpl> ads_service =
      std::make_unique<AdsServiceImpl>(
          profile,
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
          brave_adaptive_captcha_service, CreateAdsTooltipsDelegate(profile),
#endif
          std::make_unique<DeviceIdImpl>(), history_service, rewards_service,
          notification_ad_async_data_store);
  return ads_service.release();
}

bool AdsServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace brave_ads
