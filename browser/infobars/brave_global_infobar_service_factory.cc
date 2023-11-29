/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_global_infobar_service_factory.h"

#include "brave/browser/infobars/brave_global_infobar_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

// static
BraveGlobalInfobarServiceFactory*
BraveGlobalInfobarServiceFactory::GetInstance() {
  static base::NoDestructor<BraveGlobalInfobarServiceFactory> instance;
  return instance.get();
}

// static
BraveGlobalInfobarService*
BraveGlobalInfobarServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BraveGlobalInfobarService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BraveGlobalInfobarServiceFactory::BraveGlobalInfobarServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveGlobalInfobarService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveGlobalInfobarServiceFactory::~BraveGlobalInfobarServiceFactory() = default;

KeyedService* BraveGlobalInfobarServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveGlobalInfobarService(user_prefs::UserPrefs::Get(context));
}
