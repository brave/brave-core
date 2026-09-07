/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/brave_account_service_factory.h"

#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace brave_account {

// static
BraveAccountServiceFactory* BraveAccountServiceFactory::GetInstance() {
  static base::NoDestructor<BraveAccountServiceFactory> instance;
  return instance.get();
}

// static
BraveAccountService* BraveAccountServiceFactory::GetFor(
    content::BrowserContext* context) {
  CHECK(context);
  return static_cast<BraveAccountService*>(
      GetInstance()->GetServiceForContext(context, true));
}

BraveAccountServiceFactory::BraveAccountServiceFactory()
    : ProfileKeyedServiceFactory("BraveAccountService") {
  CHECK(features::IsBraveAccountEnabled());
}

BraveAccountServiceFactory::~BraveAccountServiceFactory() = default;

bool BraveAccountServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

bool BraveAccountServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

std::unique_ptr<KeyedService>
BraveAccountServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  CHECK(context);
  auto* profile = Profile::FromBrowserContext(context);
  return std::make_unique<BraveAccountService>(
      profile->GetPrefs(),
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess(),
      g_browser_process->os_crypt_async(),
      base::BindRepeating(
          [](Profile* profile) -> syncer::BraveSyncServiceImpl* {
            if (!SyncServiceFactory::IsSyncAllowed(profile)) {
              return nullptr;
            }
            return static_cast<syncer::BraveSyncServiceImpl*>(
                SyncServiceFactory::GetForProfile(profile));
          },
          profile));
}

}  // namespace brave_account
