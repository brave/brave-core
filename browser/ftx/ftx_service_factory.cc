/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ftx/ftx_service_factory.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/ftx/browser/ftx_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
FTXServiceFactory* FTXServiceFactory::GetInstance() {
  return base::Singleton<FTXServiceFactory>::get();
}

// static
FTXService*FTXServiceFactory::GetForProfile(
    Profile* profile) {
  if (!brave::IsRegularProfile(profile)) {
    return nullptr;
  }

  return static_cast<FTXService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

FTXServiceFactory::FTXServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "FTXService",
          BrowserContextDependencyManager::GetInstance()) {
}

FTXServiceFactory::~FTXServiceFactory() {
}

KeyedService* FTXServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new FTXService(Profile::FromBrowserContext(context));
}

content::BrowserContext* FTXServiceFactory::GetBrowserContextToUse(
      content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
