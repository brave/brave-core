/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/email_aliases/email_aliases_service_factory.h"

#include <utility>

#include "base/check_deref.h"
#include "brave/browser/brave_account/brave_account_dependent_services_factory.h"
#include "brave/browser/brave_account/brave_account_service_factory.h"
#include "brave/components/brave_account/brave_account_dependent_services.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/mojom/dependent_services.mojom.h"
#include "brave/components/email_aliases/email_aliases_service.h"
#include "brave/components/email_aliases/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"

namespace email_aliases {

// static
EmailAliasesService* EmailAliasesServiceFactory::GetServiceForProfile(
    Profile* profile) {
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
    : ProfileKeyedServiceFactory("EmailAliasesService") {
  DependsOn(brave_account::BraveAccountDependentServicesFactory::GetInstance());
  DependsOn(brave_account::BraveAccountServiceFactory::GetInstance());
}

EmailAliasesServiceFactory::~EmailAliasesServiceFactory() = default;

bool EmailAliasesServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

std::unique_ptr<KeyedService>
EmailAliasesServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  PrefService* pref_service = user_prefs::UserPrefs::Get(context);
  if (!pref_service ||
      !features::IsEmailAliasesEnabledForProfile(*pref_service)) {
    return nullptr;
  }

  mojo::PendingRemote<brave_account::mojom::Authentication> brave_account_auth;
  CHECK_DEREF(brave_account::BraveAccountServiceFactory::GetFor(context))
      .BindInterface(brave_account_auth.InitWithNewPipeAndPassReceiver());

  mojo::PendingRemote<brave_account::mojom::DependentServiceRegistrar>
      dependent_service_registrar;
  CHECK_DEREF(
      brave_account::BraveAccountDependentServicesFactory::GetFor(context))
      .BindInterface(
          dependent_service_registrar.InitWithNewPipeAndPassReceiver());

  return std::make_unique<EmailAliasesService>(
      std::move(brave_account_auth), std::move(dependent_service_registrar),
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess(),
      *pref_service);
}

}  // namespace email_aliases
