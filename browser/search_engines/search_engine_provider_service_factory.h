/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;
class SearchEngineProviderService;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

// The purpose of this factory is to configure proper search engine provider to
// private/tor profile before it is referenced.
// Also, this factory doesn't have public api. Instead, service is instantiated
// when profile is initialized.
class SearchEngineProviderServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  SearchEngineProviderServiceFactory(
      const SearchEngineProviderServiceFactory&) = delete;
  SearchEngineProviderServiceFactory& operator=(
      const SearchEngineProviderServiceFactory&) = delete;

  static SearchEngineProviderServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<SearchEngineProviderServiceFactory>;
  SearchEngineProviderServiceFactory();
  ~SearchEngineProviderServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_SERVICE_FACTORY_H_
