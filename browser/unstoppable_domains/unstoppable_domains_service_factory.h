/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace unstoppable_domains {

class UnstoppableDomainsService;

class UnstoppableDomainsServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static UnstoppableDomainsService* GetForContext(
      content::BrowserContext* context);
  static UnstoppableDomainsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<UnstoppableDomainsServiceFactory>;

  UnstoppableDomainsServiceFactory();
  ~UnstoppableDomainsServiceFactory() override;

  UnstoppableDomainsServiceFactory(const UnstoppableDomainsServiceFactory&) =
      delete;
  UnstoppableDomainsServiceFactory& operator=(
      const UnstoppableDomainsServiceFactory&) = delete;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace unstoppable_domains

#endif  // BRAVE_BROWSER_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_FACTORY_H_
