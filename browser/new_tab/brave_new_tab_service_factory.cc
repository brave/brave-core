/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/brave_new_tab_service_factory.h"

#include "brave/browser/new_tab/brave_new_tab_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
BraveNewTabServiceFactory* BraveNewTabServiceFactory::GetInstance() {
  return base::Singleton<BraveNewTabServiceFactory>::get();
}

BraveNewTabServiceFactory::~BraveNewTabServiceFactory() = default;

BraveNewTabServiceFactory::BraveNewTabServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveNewTabService",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* BraveNewTabServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveNewTabService(context);
}

// static
BraveNewTabService* BraveNewTabServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  return static_cast<BraveNewTabService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
