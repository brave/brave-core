// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_controller_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/favicon/core/favicon_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/service_access_type.h"
#include "content/public/browser/browser_context.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

// static
BraveNewsControllerFactory* BraveNewsControllerFactory::GetInstance() {
  static base::NoDestructor<BraveNewsControllerFactory> instance;
  return instance.get();
}

// static
BraveNewsController* BraveNewsControllerFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<BraveNewsController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
mojo::PendingRemote<mojom::BraveNewsController>
BraveNewsControllerFactory::GetRemoteService(content::BrowserContext* context) {
  return static_cast<BraveNewsController*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
BraveNewsController* BraveNewsControllerFactory::GetControllerForContext(
    content::BrowserContext* context) {
  return static_cast<BraveNewsController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BraveNewsControllerFactory::BraveNewsControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveNewsControllerFactory",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(brave_ads::AdsServiceFactory::GetInstance());
  DependsOn(FaviconServiceFactory::GetInstance());
  DependsOn(HistoryServiceFactory::GetInstance());
}

BraveNewsControllerFactory::~BraveNewsControllerFactory() = default;

KeyedService* BraveNewsControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!brave::IsRegularProfile(context)) {
    return nullptr;
  }
  auto* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  auto* favicon_service = FaviconServiceFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile);
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  return new BraveNewsController(profile->GetPrefs(), favicon_service,
                                 ads_service, history_service,
                                 profile->GetURLLoaderFactory());
}

}  // namespace brave_news
