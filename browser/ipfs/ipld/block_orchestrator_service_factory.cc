// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ipfs/ipld/block_orchestrator_service_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "brave/components/ipfs/ipld/block_orchestrator_service.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"


namespace ipfs::ipld {

// static
BlockOrchestratorServiceFactory* BlockOrchestratorServiceFactory::GetInstance() {
  static base::NoDestructor<BlockOrchestratorServiceFactory> instance;
  return instance.get();
}

// static
BlockOrchestratorService* BlockOrchestratorServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!ipfs::IpfsServiceFactory::IsIpfsEnabled(context)) {
    return nullptr;
  }
  return static_cast<BlockOrchestratorService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BlockOrchestratorServiceFactory::BlockOrchestratorServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BlockOrchestratorService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ipfs::IpfsServiceFactory::GetInstance());
}

BlockOrchestratorServiceFactory::~BlockOrchestratorServiceFactory() = default;

KeyedService* BlockOrchestratorServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BlockOrchestratorService(user_prefs::UserPrefs::Get(context));
}

content::BrowserContext* BlockOrchestratorServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace ipfs
