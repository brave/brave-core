/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/skus/sdk_controller_factory.h"

#include "brave/components/skus/browser/sdk_controller.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace skus {

// static
mojom::SdkController* SdkControllerFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<skus::SdkController*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
SdkControllerFactory* SdkControllerFactory::GetInstance() {
  return base::Singleton<SdkControllerFactory>::get();
}

SdkControllerFactory::SdkControllerFactory()
    : BrowserStateKeyedServiceFactory(
          "SkusSdkController",
          BrowserStateDependencyManager::GetInstance()) {}

SdkControllerFactory::~SdkControllerFactory() = default;

std::unique_ptr<KeyedService> SdkControllerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  if (browser_state->IsOffTheRecord()) {
    return nullptr;
  }
  std::unique_ptr<SdkController> sku_service(new SdkController(
      browser_state->GetPrefs(), browser_state->GetSharedURLLoaderFactory()));
  return sku_service;
}

bool SdkControllerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace skus
