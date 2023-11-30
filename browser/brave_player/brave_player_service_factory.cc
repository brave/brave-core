/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_player/brave_player_service_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/brave_player/brave_player_service_delegate_impl.h"
#include "brave/components/brave_player/core/browser/brave_player_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_player {

BravePlayerServiceFactory::BravePlayerServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BravePlayerService",
          BrowserContextDependencyManager::GetInstance()) {}

BravePlayerServiceFactory::~BravePlayerServiceFactory() = default;

// static
BravePlayerServiceFactory& BravePlayerServiceFactory::GetInstance() {
  static base::NoDestructor<BravePlayerServiceFactory> instance;
  return *instance.get();
}

// static
BravePlayerService& BravePlayerServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return *static_cast<BravePlayerService*>(
      GetInstance().GetServiceForBrowserContext(context, /*create=*/true));
}

KeyedService* BravePlayerServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BravePlayerService(
      std::make_unique<BravePlayerServiceDelegateImpl>());
}

}  // namespace brave_player
