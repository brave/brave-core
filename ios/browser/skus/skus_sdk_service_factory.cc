/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/skus/skus_sdk_service_factory.h"

#include "brave/components/skus/browser/skus_sdk_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace skus {

// static
mojom::SkusSdk* SkusSdkServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return nullptr; // Currently don't have the correct type to return
  // return static_cast<SkusSdkService*>(
      // GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
SkusSdkServiceFactory* SkusSdkServiceFactory::GetInstance() {
  return base::Singleton<SkusSdkServiceFactory>::get();
}

SkusSdkServiceFactory::SkusSdkServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "SkusSdkService",
          BrowserStateDependencyManager::GetInstance()) {}

SkusSdkServiceFactory::~SkusSdkServiceFactory() = default;

std::unique_ptr<KeyedService>
SkusSdkServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  if (browser_state->IsOffTheRecord()) {
    return nullptr;
  }
  std::unique_ptr<SkusSdkService> sku_service(
      new SkusSdkService(browser_state->GetPrefs(),
                         browser_state->GetSharedURLLoaderFactory()));
  return sku_service;
}

bool SkusSdkServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* SkusSdkServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace skus
