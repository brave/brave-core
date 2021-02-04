/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/unstoppable_domains/unstoppable_domains_service_factory.h"

#include <memory>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/unstoppable_domains/unstoppable_domains_service_delegate_impl.h"
#include "brave/components/unstoppable_domains/unstoppable_domains_service.h"
#include "brave/components/unstoppable_domains/utils.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace unstoppable_domains {

UnstoppableDomainsServiceFactory::UnstoppableDomainsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "UnstoppableDomainsService",
          BrowserContextDependencyManager::GetInstance()) {}

UnstoppableDomainsServiceFactory::~UnstoppableDomainsServiceFactory() {}

// static
UnstoppableDomainsServiceFactory*
UnstoppableDomainsServiceFactory::GetInstance() {
  return base::Singleton<UnstoppableDomainsServiceFactory>::get();
}

// static
UnstoppableDomainsService* UnstoppableDomainsServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsUnstoppableDomainsEnabled())
    return nullptr;

  return static_cast<UnstoppableDomainsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

KeyedService* UnstoppableDomainsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new UnstoppableDomainsService(
      std::make_unique<UnstoppableDomainsServiceDelegateImpl>(), context,
      g_brave_browser_process ? g_brave_browser_process->local_state()
                              : nullptr);
}

}  // namespace unstoppable_domains
