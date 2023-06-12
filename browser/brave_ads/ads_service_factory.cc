/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/browser/brave_ads/device_id/device_id_impl.h"
#include "brave/browser/brave_ads/services/bat_ads_service_factory_impl.h"
#include "brave/browser/brave_federated/brave_federated_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "brave/components/brave_federated/brave_federated_service.h"
#include "brave/components/brave_federated/data_store_service.h"
#include "brave/components/brave_federated/notification_ad_task_constants.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

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
  static base::NoDestructor<AdsServiceFactory> instance;
  return instance.get();
}

AdsServiceFactory::AdsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(NotificationDisplayServiceFactory::GetInstance());
  DependsOn(brave_rewards::RewardsServiceFactory::GetInstance());
  DependsOn(HistoryServiceFactory::GetInstance());
  DependsOn(brave_federated::BraveFederatedServiceFactory::GetInstance());
  DependsOn(brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::
                GetInstance());
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
  auto* brave_adaptive_captcha_service =
      brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetInstance()
          ->GetForProfile(profile);
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
          profile, brave_adaptive_captcha_service,
          CreateAdsTooltipsDelegate(profile), std::make_unique<DeviceIdImpl>(),
          std::make_unique<BatAdsServiceFactoryImpl>(), history_service,
          rewards_service, notification_ad_async_data_store);
  return ads_service.release();
}

bool AdsServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace brave_ads
