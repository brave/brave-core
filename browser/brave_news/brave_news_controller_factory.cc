// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_controller_factory.h"

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace brave_news {

// static
BraveNewsControllerFactory* BraveNewsControllerFactory::GetInstance() {
  return base::Singleton<BraveNewsControllerFactory>::get();
}

// static
BraveNewsController* BraveNewsControllerFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<BraveNewsController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BraveNewsControllerFactory::BraveNewsControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveNewsControllerFactory",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(brave_ads::AdsServiceFactory::GetInstance());
  DependsOn(HistoryServiceFactory::GetInstance());
}

BraveNewsControllerFactory::~BraveNewsControllerFactory() = default;

KeyedService* BraveNewsControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  auto* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile);
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  return new BraveNewsController(
      user_prefs::UserPrefs::Get(context), ads_service,  history_service,
      shared_url_loader_factory);
}

content::BrowserContext* BraveNewsControllerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // nanespace brave_news