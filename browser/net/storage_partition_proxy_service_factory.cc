/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/storage_partition_proxy_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/net/storage_partition_proxy_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "content/public/browser/browser_context.h"

namespace brave {

// static
StoragePartitionProxyServiceFactory*
StoragePartitionProxyServiceFactory::GetInstance() {
  static base::NoDestructor<StoragePartitionProxyServiceFactory> instance;
  return instance.get();
}

// static
StoragePartitionProxyService*
StoragePartitionProxyServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<StoragePartitionProxyService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

StoragePartitionProxyServiceFactory::StoragePartitionProxyServiceFactory()
    : ProfileKeyedServiceFactory(
          "StoragePartitionProxyService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .Build()) {}

StoragePartitionProxyServiceFactory::~StoragePartitionProxyServiceFactory() =
    default;

std::unique_ptr<KeyedService>
StoragePartitionProxyServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  // Never bypass Tor's proxy or apply regular profile settings to Tor.
  if (context->IsTor()) {
    return nullptr;
  }
  return std::make_unique<StoragePartitionProxyService>(
      *Profile::FromBrowserContext(context)->GetPrefs(),
      *g_browser_process->local_state(), context->IsOffTheRecord());
}

}  // namespace brave
