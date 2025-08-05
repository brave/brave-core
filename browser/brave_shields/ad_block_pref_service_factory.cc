/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/ad_block_pref_service_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/ad_block_pref_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/proxy_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/proxy_config/pref_proxy_config_tracker.h"
#include "net/proxy_resolution/proxy_config_service.h"

namespace brave_shields {

// static
AdBlockPrefService* AdBlockPrefServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdBlockPrefService*>(
      GetInstance()->GetServiceForBrowserContext(context,
                                                 /*create_service=*/true));
}

// static
AdBlockPrefServiceFactory* AdBlockPrefServiceFactory::GetInstance() {
  static base::NoDestructor<AdBlockPrefServiceFactory> instance;
  return instance.get();
}

AdBlockPrefServiceFactory::AdBlockPrefServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdBlockPrefService",
          BrowserContextDependencyManager::GetInstance()) {}

AdBlockPrefServiceFactory::~AdBlockPrefServiceFactory() = default;

std::unique_ptr<KeyedService>
AdBlockPrefServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  auto service = std::make_unique<AdBlockPrefService>(
      g_brave_browser_process->ad_block_service(), profile->GetPrefs(),
      g_browser_process->local_state());

  auto pref_proxy_config_tracker =
      ProxyServiceFactory::CreatePrefProxyConfigTrackerOfProfile(
          profile->GetPrefs(), nullptr);
  auto proxy_config_service = ProxyServiceFactory::CreateProxyConfigService(
      pref_proxy_config_tracker.get(), profile);
  service->StartProxyTracker(std::move(pref_proxy_config_tracker),
                             std::move(proxy_config_service));
  return service;
}

content::BrowserContext* AdBlockPrefServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

bool AdBlockPrefServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace brave_shields
