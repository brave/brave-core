// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SKUS_SKUS_SDK_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SKUS_SKUS_SDK_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/skus/browser/skus_sdk_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace skus {

// Singleton that creates/deletes BraveRendererUpdater as new Profiles are
// created/shutdown.
class SkusSdkServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static skus::SkusSdkService* GetForContext(content::BrowserContext* context);
  static SkusSdkServiceFactory* GetInstance();

  SkusSdkServiceFactory(const SkusSdkServiceFactory&) = delete;
  SkusSdkServiceFactory& operator=(const SkusSdkServiceFactory&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<SkusSdkServiceFactory>;

  SkusSdkServiceFactory();
  ~SkusSdkServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

}  // namespace skus

#endif  // BRAVE_BROWSER_SKUS_SKUS_SDK_SERVICE_FACTORY_H_
