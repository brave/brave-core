/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/brave_federated_learning/brave_federated_learning_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {
// Singleton that owns all BraveFederatedLearningService and associates them
// with Profiles.
class BraveFederatedLearningServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  BraveFederatedLearningServiceFactory(
      const BraveFederatedLearningServiceFactory&) = delete;
  BraveFederatedLearningServiceFactory& operator=(
      const BraveFederatedLearningServiceFactory&) = delete;

  static BraveFederatedLearningService* GetForBrowserContext(
      content::BrowserContext* context);
  static BraveFederatedLearningServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      BraveFederatedLearningServiceFactory>;

  BraveFederatedLearningServiceFactory();
  ~BraveFederatedLearningServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_FACTORY_H_
