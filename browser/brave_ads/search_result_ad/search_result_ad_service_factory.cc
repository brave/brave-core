/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/search_result_ad/search_result_ad_service_factory.h"

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_ads {

// static
SearchResultAdService* SearchResultAdServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<SearchResultAdService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create*/ true));
}

// static
SearchResultAdServiceFactory* SearchResultAdServiceFactory::GetInstance() {
  return base::Singleton<SearchResultAdServiceFactory>::get();
}

SearchResultAdServiceFactory::SearchResultAdServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SearchResultAdService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(AdsServiceFactory::GetInstance());
}

SearchResultAdServiceFactory::~SearchResultAdServiceFactory() = default;

KeyedService* SearchResultAdServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  return new SearchResultAdService(ads_service);
}

}  // namespace brave_ads
