/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_ads {

class SearchResultAdService;

class SearchResultAdServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static SearchResultAdService* GetForProfile(Profile* profile);

  static SearchResultAdServiceFactory* GetInstance();

  SearchResultAdServiceFactory(const SearchResultAdServiceFactory&) = delete;
  SearchResultAdServiceFactory& operator=(const SearchResultAdServiceFactory&) =
      delete;

 private:
  friend struct base::DefaultSingletonTraits<SearchResultAdServiceFactory>;

  SearchResultAdServiceFactory();
  ~SearchResultAdServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_FACTORY_H_
