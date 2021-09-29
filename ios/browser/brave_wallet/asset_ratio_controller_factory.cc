/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/asset_ratio_controller_factory.h"

#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojom::AssetRatioController* AssetRatioControllerFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<AssetRatioController*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
AssetRatioControllerFactory* AssetRatioControllerFactory::GetInstance() {
  return base::Singleton<AssetRatioControllerFactory>::get();
}

AssetRatioControllerFactory::AssetRatioControllerFactory()
    : BrowserStateKeyedServiceFactory(
          "AssetRatioController",
          BrowserStateDependencyManager::GetInstance()) {}

AssetRatioControllerFactory::~AssetRatioControllerFactory() = default;

std::unique_ptr<KeyedService>
AssetRatioControllerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<AssetRatioController> asset_ratio_controller(
      new AssetRatioController(browser_state->GetSharedURLLoaderFactory()));
  return asset_ratio_controller;
}

bool AssetRatioControllerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* AssetRatioControllerFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
