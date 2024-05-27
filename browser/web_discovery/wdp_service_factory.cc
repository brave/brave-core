/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/wdp_service_factory.h"

#include "base/path_service.h"
#include "brave/components/web_discovery/browser/wdp_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_paths.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace web_discovery {

WDPService* WDPServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<WDPService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

WDPServiceFactory* WDPServiceFactory::GetInstance() {
  static base::NoDestructor<WDPServiceFactory> instance;
  return instance.get();
}

WDPServiceFactory::WDPServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WDPService",
          BrowserContextDependencyManager::GetInstance()) {}

WDPServiceFactory::~WDPServiceFactory() = default;

KeyedService* WDPServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  base::FilePath user_data_dir =
      base::PathService::CheckedGet(chrome::DIR_USER_DATA);
  return new WDPService(g_browser_process->local_state(),
                        user_prefs::UserPrefs::Get(context), user_data_dir,
                        shared_url_loader_factory);
}

content::BrowserContext* WDPServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Prevents creation of service instance for incognito/OTR profiles
  return context->IsOffTheRecord() ? nullptr : context;
}

bool WDPServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace web_discovery
