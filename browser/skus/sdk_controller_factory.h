// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SKUS_SDK_CONTROLLER_FACTORY_H_
#define BRAVE_BROWSER_SKUS_SDK_CONTROLLER_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/skus/browser/sdk_controller.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace skus {

// Singleton that creates/deletes SdkControllerFactory as new Profiles are
// created/shutdown.
class SdkControllerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static mojo::PendingRemote<mojom::SdkController> GetForContext(
      content::BrowserContext* context);
  static skus::SdkController* GetControllerForContext(
      content::BrowserContext* context);
  static SdkControllerFactory* GetInstance();

  SdkControllerFactory(const SdkControllerFactory&) = delete;
  SdkControllerFactory& operator=(const SdkControllerFactory&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<SdkControllerFactory>;

  SdkControllerFactory();
  ~SdkControllerFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

}  // namespace skus

#endif  // BRAVE_BROWSER_SKUS_SDK_CONTROLLER_FACTORY_H_
