/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_renderer_updater_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/profiles/brave_renderer_updater.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

BraveRendererUpdaterFactory::BraveRendererUpdaterFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveRendererUpdater",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(brave_wallet::BraveWalletServiceFactory::GetInstance());
}

BraveRendererUpdaterFactory::~BraveRendererUpdaterFactory() = default;

// static
BraveRendererUpdaterFactory* BraveRendererUpdaterFactory::GetInstance() {
  static base::NoDestructor<BraveRendererUpdaterFactory> instance;
  return instance.get();
}

// static
BraveRendererUpdater* BraveRendererUpdaterFactory::GetForProfile(
    Profile* profile) {
  return static_cast<BraveRendererUpdater*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

KeyedService* BraveRendererUpdaterFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(context);

  auto* keyring_service =
      brave_wallet_service ? brave_wallet_service->keyring_service() : 0;
  return new BraveRendererUpdater(static_cast<Profile*>(context),
                                  keyring_service,
                                  g_browser_process->local_state());
}

bool BraveRendererUpdaterFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

content::BrowserContext* BraveRendererUpdaterFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
