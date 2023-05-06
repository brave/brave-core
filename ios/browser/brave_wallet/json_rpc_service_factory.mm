/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"

#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::JsonRpcService>
JsonRpcServiceFactory::GetForBrowserState(ChromeBrowserState* browser_state) {
  return static_cast<JsonRpcService*>(
             GetInstance()->GetServiceForBrowserState(browser_state, true))
      ->MakeRemote();
}

// static
JsonRpcService* JsonRpcServiceFactory::GetServiceForState(
    ChromeBrowserState* browser_state) {
  return static_cast<JsonRpcService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
JsonRpcServiceFactory* JsonRpcServiceFactory::GetInstance() {
  return base::Singleton<JsonRpcServiceFactory>::get();
}

JsonRpcServiceFactory::JsonRpcServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "JsonRpcService",
          BrowserStateDependencyManager::GetInstance()) {}

JsonRpcServiceFactory::~JsonRpcServiceFactory() = default;

std::unique_ptr<KeyedService> JsonRpcServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<JsonRpcService> json_rpc_service(new JsonRpcService(
      browser_state->GetSharedURLLoaderFactory(), browser_state->GetPrefs(),
      GetApplicationContext()->GetLocalState()));
  return json_rpc_service;
}

bool JsonRpcServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* JsonRpcServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
