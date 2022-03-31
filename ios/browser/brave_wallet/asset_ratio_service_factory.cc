/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"

#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::AssetRatioService>
AssetRatioServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<AssetRatioService*>(
             GetInstance()->GetServiceForBrowserState(browser_state, true))
      ->MakeRemote();
}

// static
AssetRatioService* AssetRatioServiceFactory::GetServiceForState(
    ChromeBrowserState* browser_state) {
  return static_cast<AssetRatioService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
AssetRatioServiceFactory* AssetRatioServiceFactory::GetInstance() {
  return base::Singleton<AssetRatioServiceFactory>::get();
}

AssetRatioServiceFactory::AssetRatioServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "AssetRatioService",
          BrowserStateDependencyManager::GetInstance()) {}

AssetRatioServiceFactory::~AssetRatioServiceFactory() = default;

std::unique_ptr<KeyedService> AssetRatioServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<AssetRatioService> asset_ratio_service(
      new AssetRatioService(browser_state->GetSharedURLLoaderFactory()));
  return asset_ratio_service;
}

bool AssetRatioServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* AssetRatioServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
