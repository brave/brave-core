/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_provider_service_factory.h"

#include <string>

#include "base/no_destructor.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/search_engines/search_engines_pref_names.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/search_engines/normal_window_search_engine_provider_service_android.h"
#include "brave/browser/search_engines/private_window_search_engine_provider_service_android.h"
#else
#include "brave/browser/search_engines/normal_window_search_engine_provider_service.h"
#include "brave/browser/search_engines/private_window_search_engine_provider_service.h"
#include "brave/browser/search_engines/tor_window_search_engine_provider_service.h"
#endif

namespace {

// Factory owns service object.
KeyedService* InitializeSearchEngineProviderServiceIfNeeded(Profile* profile) {
#if BUILDFLAG(IS_ANDROID)
  if (profile->IsIncognitoProfile()) {
    return new PrivateWindowSearchEngineProviderServiceAndroid(profile);
  }

  if (profile->IsRegularProfile()) {
    return new NormalWindowSearchEngineProviderServiceAndroid(profile);
  }
#else
  // Set search engine handler for tor or private profile.
  if (profile->IsTor()) {
    return new TorWindowSearchEngineProviderService(profile);
  }

  if (profile->IsIncognitoProfile()) {
    return new PrivateWindowSearchEngineProviderService(profile);
  }

  if (profile->IsRegularProfile()) {
    return new NormalWindowSearchEngineProviderService(profile);
  }
#endif

  return nullptr;
}

}  // namespace

// static
SearchEngineProviderServiceFactory*
SearchEngineProviderServiceFactory::GetInstance() {
  static base::NoDestructor<SearchEngineProviderServiceFactory> instance;
  return instance.get();
}

SearchEngineProviderServiceFactory::SearchEngineProviderServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SearchEngineProviderService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TemplateURLServiceFactory::GetInstance());
}

SearchEngineProviderServiceFactory::~SearchEngineProviderServiceFactory() =
    default;

KeyedService* SearchEngineProviderServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return InitializeSearchEngineProviderServiceIfNeeded(
      Profile::FromBrowserContext(context));
}

content::BrowserContext*
SearchEngineProviderServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
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
#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(prefs::kDefaultSearchProviderByExtension,
                                false);
  registry->RegisterStringPref(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                               std::string(),
                               user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
#endif

  registry->RegisterDictionaryPref(
      prefs::kSyncedDefaultPrivateSearchProviderData,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}
