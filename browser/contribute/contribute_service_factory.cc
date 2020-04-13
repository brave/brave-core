/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/contribute/contribute_service_factory.h"

#include "brave/components/contribute/browser/contribute_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
ContributeServiceFactory* ContributeServiceFactory::GetInstance() {
  return base::Singleton<ContributeServiceFactory>::get();
}

// static
ContributeService* ContributeServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<ContributeService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

ContributeServiceFactory::ContributeServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ContributeService",
          BrowserContextDependencyManager::GetInstance()) {
}

ContributeServiceFactory::~ContributeServiceFactory() {
}

KeyedService* ContributeServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new ContributeService(Profile::FromBrowserContext(context));
}

content::BrowserContext* ContributeServiceFactory::GetBrowserContextToUse(
      content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
