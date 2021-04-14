/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_provider_service_factory.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/search_engine_provider_service.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

// static
SearchEngineProviderService* SearchEngineProviderServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<SearchEngineProviderService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SearchEngineProviderServiceFactory*
SearchEngineProviderServiceFactory::GetInstance() {
  return base::Singleton<SearchEngineProviderServiceFactory>::get();
}

SearchEngineProviderServiceFactory::SearchEngineProviderServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SearchEngineProviderService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TemplateURLServiceFactory::GetInstance());
}

SearchEngineProviderServiceFactory::~SearchEngineProviderServiceFactory() {}

KeyedService* SearchEngineProviderServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // Guest profile in qwant region doesn't need special handling of alternative
  // search engine provider because its newtab doesn't have ddg toggle button.
  auto* profile = Profile::FromBrowserContext(context);
  if (brave::IsGuestProfile(profile) && !brave::IsRegionForQwant(profile)) {
    return new SearchEngineProviderService(profile);
  }

  return nullptr;
}

content::BrowserContext*
SearchEngineProviderServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool SearchEngineProviderServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

bool
SearchEngineProviderServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  // Service should be initialized when profile is created to set proper
  // provider to TemplateURLService.
  return true;
}

void SearchEngineProviderServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kUseAlternativeSearchEngineProvider, false);
  registry->RegisterDictionaryPref(kCachedNormalSearchProvider);
}
