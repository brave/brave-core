/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SKUS_SKUS_SDK_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_SKUS_SKUS_SDK_SERVICE_FACTORY_H_

#include <memory>

#include "base/memory/singleton.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class ChromeBrowserState;
class KeyedService;

namespace web {
class BrowserState;
}  // namespace web

namespace skus {

class SkusSdk;

class SkusSdkServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Creates the service if it doesn't exist already for |browser_state|.
  static mojom::SkusSdk* GetForBrowserState(
      ChromeBrowserState* browser_state);

  static SkusSdkServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<SkusSdkServiceFactory>;

  SkusSdkServiceFactory();
  ~SkusSdkServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  SkusSdkServiceFactory(const SkusSdkServiceFactory&) = delete;
  SkusSdkServiceFactory& operator=(const SkusSdkServiceFactory&) =
      delete;
};

}  // namespace skus

#endif  // BRAVE_IOS_BROWSER_SKUS_SKUS_SDK_SERVICE_FACTORY_H_
