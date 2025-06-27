/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/email_aliases/email_aliases_service_factory.h"

#include <memory>

#include "brave/browser/ui/email_aliases/email_aliases_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace email_aliases {

// static
void EmailAliasesServiceFactory::BindForProfile(
    Profile* profile,
    mojo::PendingReceiver<mojom::EmailAliasesService> receiver) {
  auto* service = static_cast<EmailAliasesService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
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
    : BrowserContextKeyedServiceFactory(
          "EmailAliasesService",
          BrowserContextDependencyManager::GetInstance()) {}

EmailAliasesServiceFactory::~EmailAliasesServiceFactory() = default;

std::unique_ptr<KeyedService>
EmailAliasesServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<EmailAliasesService>(
      Profile::FromBrowserContext(context));
}

}  // namespace email_aliases
