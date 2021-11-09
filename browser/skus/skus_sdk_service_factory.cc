// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/skus/skus_sdk_service_factory.h"

#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

// static
SkusSdkServiceFactory* SkusSdkServiceFactory::GetInstance() {
  return base::Singleton<SkusSdkServiceFactory>::get();
}

// static
SkusSdkService* SkusSdkServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<SkusSdkService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

SkusSdkServiceFactory::SkusSdkServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SkusSdkService",
          BrowserContextDependencyManager::GetInstance()) {}

SkusSdkServiceFactory::~SkusSdkServiceFactory() = default;

KeyedService* SkusSdkServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // Skus functionality not supported in private / Tor / guest windows
  if (!brave::IsRegularProfile(context)) {
    return nullptr;
  }

  return new SkusSdkService(Profile::FromBrowserContext(context)->GetPrefs(),
                            context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess());
}

content::BrowserContext* SkusSdkServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
