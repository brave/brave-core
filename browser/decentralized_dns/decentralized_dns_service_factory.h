/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace decentralized_dns {

class DecentralizedDnsService;

class DecentralizedDnsServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static DecentralizedDnsService* GetForContext(
      content::BrowserContext* context);
  static DecentralizedDnsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<DecentralizedDnsServiceFactory>;

  DecentralizedDnsServiceFactory();
  ~DecentralizedDnsServiceFactory() override;

  DecentralizedDnsServiceFactory(const DecentralizedDnsServiceFactory&) =
      delete;
  DecentralizedDnsServiceFactory& operator=(
      const DecentralizedDnsServiceFactory&) = delete;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace decentralized_dns

#endif  // BRAVE_BROWSER_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_FACTORY_H_
