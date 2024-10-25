/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_service_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/webui/brave_rewards_source.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/brave_rewards/extension_rewards_notification_service_observer.h"
#include "brave/browser/brave_rewards/extension_rewards_service_observer.h"
#include "extensions/browser/event_router_factory.h"
#endif

namespace brave_rewards {

RewardsService* testing_service_ = nullptr;

// static
RewardsService* RewardsServiceFactory::GetForProfile(Profile* profile) {
  if (testing_service_) {
    return testing_service_;
  }

  if (!IsSupportedForProfile(profile)) {
    return nullptr;
  }

  return static_cast<RewardsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
RewardsServiceFactory* RewardsServiceFactory::GetInstance() {
  static base::NoDestructor<RewardsServiceFactory> instance;
  return instance.get();
}

RewardsServiceFactory::RewardsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "RewardsService",
          BrowserContextDependencyManager::GetInstance()) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DependsOn(extensions::EventRouterFactory::GetInstance());
#endif
  DependsOn(brave_wallet::BraveWalletServiceFactory::GetInstance());
}

std::unique_ptr<KeyedService>
RewardsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  std::unique_ptr<RewardsServiceObserver> extension_observer = nullptr;
  std::unique_ptr<RewardsNotificationServiceObserver> notification_observer =
      nullptr;
  auto* profile = Profile::FromBrowserContext(context);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_observer =
      std::make_unique<ExtensionRewardsServiceObserver>(profile);
  notification_observer =
      std::make_unique<ExtensionRewardsNotificationServiceObserver>(profile);
#endif
  // Set up the rewards data source
  content::URLDataSource::Add(profile,
                              std::make_unique<BraveRewardsSource>(profile));

  // BitmapFetcherServiceFactory has private ProfileKeyedServiceFactory so we
  // can't add `DependsOn` to ensure proper lifetime management.
  auto request_image_callback = base::BindRepeating(
      [](Profile* profile, const GURL& url,
         BitmapFetcherService::BitmapFetchedCallback callback,
         const net::NetworkTrafficAnnotationTag& tag) {
        auto* bitmap_fetcher_service =
            BitmapFetcherServiceFactory::GetForBrowserContext(profile);
        if (bitmap_fetcher_service) {
          return bitmap_fetcher_service
              ->RequestImageWithNetworkTrafficAnnotationTag(
                  url, std::move(callback), tag);
        } else {
          return BitmapFetcherService::REQUEST_ID_INVALID;
        }
      },
      profile);
  auto cancel_request_image_callback = base::BindRepeating(
      [](Profile* profile, BitmapFetcherService::RequestId request_id) {
        auto* bitmap_fetcher_service =
            BitmapFetcherServiceFactory::GetForBrowserContext(profile);
        if (bitmap_fetcher_service) {
          return bitmap_fetcher_service->CancelRequest(request_id);
        }
      },
      profile);

  std::unique_ptr<RewardsServiceImpl> rewards_service =
      std::make_unique<RewardsServiceImpl>(
          profile->GetPrefs(), profile->GetPath(),
          FaviconServiceFactory::GetForProfile(
              profile, ServiceAccessType::EXPLICIT_ACCESS),
          request_image_callback, cancel_request_image_callback,
          profile->GetDefaultStoragePartition(),
          brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
              context));
  rewards_service->Init(std::move(extension_observer),
                        std::move(notification_observer));
  return rewards_service;
}

// static
void RewardsServiceFactory::SetServiceForTesting(RewardsService* service) {
  testing_service_ = service;
}

bool RewardsServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace brave_rewards
