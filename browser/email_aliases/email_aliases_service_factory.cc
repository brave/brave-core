/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/email_aliases/email_aliases_service_factory.h"

#include <utility>

#include "brave/components/email_aliases/email_aliases_service.h"
#include "brave/components/email_aliases/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"

namespace email_aliases {

// static
EmailAliasesService* EmailAliasesServiceFactory::GetServiceForProfile(
    Profile* profile) {
  if (!base::FeatureList::IsEnabled(features::kEmailAliases)) {
    return nullptr;
  }
  return static_cast<EmailAliasesService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
void EmailAliasesServiceFactory::BindForProfile(
    Profile* profile,
    mojo::PendingReceiver<mojom::EmailAliasesService> receiver) {
  auto* service = GetServiceForProfile(profile);
  if (service) {
    service->BindInterface(std::move(receiver));
  }
}

// static
EmailAliasesServiceFactory* EmailAliasesServiceFactory::GetInstance() {
  static base::NoDestructor<EmailAliasesServiceFactory> instance;
  return instance.get();
}

EmailAliasesServiceFactory::EmailAliasesServiceFactory()
    : ProfileKeyedServiceFactory(
          "EmailAliasesService",
          ProfileSelections::BuildRedirectedInIncognito()) {}

EmailAliasesServiceFactory::~EmailAliasesServiceFactory() = default;

std::unique_ptr<KeyedService>
EmailAliasesServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<EmailAliasesService>(
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess(),
      user_prefs::UserPrefs::Get(context), g_browser_process->os_crypt_async());
}

}  // namespace email_aliases
