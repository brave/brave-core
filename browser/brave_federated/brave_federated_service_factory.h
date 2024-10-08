/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_FEDERATED_BRAVE_FEDERATED_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_FEDERATED_BRAVE_FEDERATED_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_federated/brave_federated_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_federated {

// Singleton that owns all BraveFederatedService and associates them
// with Profiles.
class BraveFederatedServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  BraveFederatedServiceFactory(const BraveFederatedServiceFactory&) = delete;
  BraveFederatedServiceFactory& operator=(const BraveFederatedServiceFactory&) =
      delete;

  static brave_federated::BraveFederatedService* GetForBrowserContext(
      content::BrowserContext* context);
  static BraveFederatedServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<BraveFederatedServiceFactory>;

  BraveFederatedServiceFactory();
  ~BraveFederatedServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace brave_federated

#endif  // BRAVE_BROWSER_BRAVE_FEDERATED_BRAVE_FEDERATED_SERVICE_FACTORY_H_
