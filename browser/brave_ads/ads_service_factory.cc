/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_factory.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/browser/brave_ads/ad_units/notification_ad/notification_ad_platform_bridge.h"
#include "brave/browser/brave_ads/ads_service_delegate.h"
#include "brave/browser/brave_ads/device_id/device_id_impl.h"
#include "brave/browser/brave_ads/services/bat_ads_service_factory_impl.h"
#include "brave/browser/brave_ads/tooltips/ads_tooltips_delegate_impl.h"
#include "brave/browser/brave_ads/virtual_pref_provider_delegate.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/public/ads_util.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/rewards_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/network_context_getter.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#endif

namespace brave_ads {

namespace {

network::mojom::NetworkContext* GetNetworkContextForProfile(
    content::BrowserContext* context) {
  // Retrieves the `NetworkContext` from the default storage partition on
  // demand. A cached raw pointer will become invalid if the network service
  // crashes or restarts, so callers must not persist the returned pointer
  // beyond the current task.
  return context->GetDefaultStoragePartition()->GetNetworkContext();
}

}  // namespace

// static
AdsService* AdsServiceFactory::GetForProfile(Profile* profile) {
  if (!profile->IsRegularProfile()) {
    return nullptr;
  }

  if (!brave_rewards::IsSupported(profile->GetPrefs())) {
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
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  DependsOn(brave_rewards::RewardsServiceFactory::GetInstance());
#endif
  DependsOn(HistoryServiceFactory::GetInstance());
  DependsOn(brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::
                GetInstance());
  DependsOn(HostContentSettingsMapFactory::GetInstance());
}

AdsServiceFactory::~AdsServiceFactory() = default;

std::unique_ptr<AdsTooltipsDelegateImpl>
AdsServiceFactory::CreateAdsTooltipsDelegate() const {
#if BUILDFLAG(IS_ANDROID)
  return nullptr;
#else
  return std::make_unique<AdsTooltipsDelegateImpl>();
#endif
}

std::unique_ptr<KeyedService>
AdsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);

  auto* prefs = profile->GetPrefs();
  auto* local_state = g_browser_process->local_state();

  auto* default_store_partition = profile->GetDefaultStoragePartition();

  auto* brave_adaptive_captcha_service =
      brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetForProfile(
          profile);
  CHECK(brave_adaptive_captcha_service);

  auto delegate = std::make_unique<AdsServiceDelegate>(
      *profile, local_state, *brave_adaptive_captcha_service,
      std::make_unique<NotificationAdPlatformBridge>(*profile));

  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  auto* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);

  auto http_client = std::make_unique<HttpClient>(
      *local_state,
      default_store_partition->GetURLLoaderFactoryForBrowserProcess(),
      base::BindRepeating(&GetNetworkContextForProfile, context),
      /*use_ohttp_staging=*/IsStagingEnvironment(*prefs));

  ProfileManager* const profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);

  return std::make_unique<AdsServiceImpl>(
      std::move(delegate), prefs, local_state, std::move(http_client),
      std::make_unique<VirtualPrefProviderDelegate>(
          *profile, profile_manager->GetProfileAttributesStorage()),
      brave::GetChannelName(), profile->GetPath(), CreateAdsTooltipsDelegate(),
      std::make_unique<DeviceIdImpl>(),
      std::make_unique<BatAdsServiceFactoryImpl>(),
      g_brave_browser_process->resource_component(), history_service,
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
      rewards_service,
#endif
      host_content_settings_map);
}

bool AdsServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace brave_ads
