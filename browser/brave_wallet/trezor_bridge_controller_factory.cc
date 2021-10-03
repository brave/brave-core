/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/trezor_bridge_controller_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/ui/webui/trezor_bridge/trezor_content_proxy.h"
#include "brave/components/brave_wallet/browser/trezor_bridge_controller.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_wallet {

// static
TrezorBridgeControllerFactory* TrezorBridgeControllerFactory::GetInstance() {
  return base::Singleton<TrezorBridgeControllerFactory>::get();
}

// static
mojo::PendingRemote<mojom::TrezorBridgeController>
TrezorBridgeControllerFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::TrezorBridgeController>();
  }

  return static_cast<TrezorBridgeController*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
TrezorBridgeController* TrezorBridgeControllerFactory::GetControllerForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<TrezorBridgeController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

TrezorBridgeControllerFactory::TrezorBridgeControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "TrezorBridgeController",
          BrowserContextDependencyManager::GetInstance()) {}

TrezorBridgeControllerFactory::~TrezorBridgeControllerFactory() {}

KeyedService* TrezorBridgeControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new TrezorBridgeController(
      context, std::make_unique<TrezorContentProxy>(context));
}

content::BrowserContext* TrezorBridgeControllerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
