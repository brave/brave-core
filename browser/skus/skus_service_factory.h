// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SKUS_SKUS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SKUS_SKUS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace skus {

class SkusServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static mojo::PendingRemote<mojom::SkusService> GetForContext(
      content::BrowserContext* context);
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<skus::mojom::SkusService> receiver);
  static SkusServiceFactory* GetInstance();

  SkusServiceFactory(const SkusServiceFactory&) = delete;
  SkusServiceFactory& operator=(const SkusServiceFactory&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<SkusServiceFactory>;

  SkusServiceFactory();
  ~SkusServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

}  // namespace skus

#endif  // BRAVE_BROWSER_SKUS_SKUS_SERVICE_FACTORY_H_
