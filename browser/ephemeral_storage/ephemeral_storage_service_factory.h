/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace ephemeral_storage {
class EphemeralStorageService;
}

class EphemeralStorageServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static ephemeral_storage::EphemeralStorageService* GetForContext(
      content::BrowserContext* context);
  static EphemeralStorageServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<EphemeralStorageServiceFactory>;

  EphemeralStorageServiceFactory();
  ~EphemeralStorageServiceFactory() override;

  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_FACTORY_H_
