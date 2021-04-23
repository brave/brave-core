/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/decentralized_dns/decentralized_dns_service_factory.h"

#include <memory>

#include "brave/browser/decentralized_dns/decentralized_dns_service_delegate_impl.h"
#include "brave/components/decentralized_dns/decentralized_dns_service.h"
#include "brave/components/decentralized_dns/utils.h"
#include "chrome/browser/browser_process.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace decentralized_dns {

DecentralizedDnsServiceFactory::DecentralizedDnsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "DecentralizedDnsService",
          BrowserContextDependencyManager::GetInstance()) {}

DecentralizedDnsServiceFactory::~DecentralizedDnsServiceFactory() {}

// static
DecentralizedDnsServiceFactory* DecentralizedDnsServiceFactory::GetInstance() {
  return base::Singleton<DecentralizedDnsServiceFactory>::get();
}

// static
DecentralizedDnsService* DecentralizedDnsServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsDecentralizedDnsEnabled())
    return nullptr;

  return static_cast<DecentralizedDnsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

KeyedService* DecentralizedDnsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new DecentralizedDnsService(
      std::make_unique<DecentralizedDnsServiceDelegateImpl>(), context,
      g_browser_process ? g_browser_process->local_state() : nullptr);
}

}  // namespace decentralized_dns
