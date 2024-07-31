/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/web_discovery_service_factory.h"

#include "base/path_service.h"
#include "brave/components/web_discovery/browser/web_discovery_service.h"
#include "brave/components/web_discovery/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_paths.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace web_discovery {

WebDiscoveryService* WebDiscoveryServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<WebDiscoveryService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

WebDiscoveryServiceFactory* WebDiscoveryServiceFactory::GetInstance() {
  static base::NoDestructor<WebDiscoveryServiceFactory> instance;
  return instance.get();
}

WebDiscoveryServiceFactory::WebDiscoveryServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WebDiscoveryService",
          BrowserContextDependencyManager::GetInstance()) {}

WebDiscoveryServiceFactory::~WebDiscoveryServiceFactory() = default;

KeyedService* WebDiscoveryServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  base::FilePath user_data_dir =
      base::PathService::CheckedGet(chrome::DIR_USER_DATA);
  return new WebDiscoveryService(g_browser_process->local_state(),
                                 user_prefs::UserPrefs::Get(context),
                                 user_data_dir, shared_url_loader_factory);
}

content::BrowserContext* WebDiscoveryServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(features::kWebDiscoveryNative)) {
    return nullptr;
  }
  // Prevents creation of service instance for incognito/OTR profiles
  return context->IsOffTheRecord() ? nullptr : context;
}

bool WebDiscoveryServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace web_discovery
