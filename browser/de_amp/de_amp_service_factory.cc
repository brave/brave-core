/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/de_amp/de_amp_service_factory.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/de_amp/browser/de_amp_service.h"
#include "brave/components/de_amp/common/features.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"

namespace de_amp {

// static
DeAmpServiceFactory* DeAmpServiceFactory::GetInstance() {
  return base::Singleton<DeAmpServiceFactory>::get();
}

DeAmpService* DeAmpServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<DeAmpService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

DeAmpServiceFactory::DeAmpServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "DeAmpService",
          BrowserContextDependencyManager::GetInstance()) {}

DeAmpServiceFactory::~DeAmpServiceFactory() = default;

KeyedService* DeAmpServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // Don't create service if de_amp feature is disabled
  if (!base::FeatureList::IsEnabled(de_amp::features::kBraveDeAMP))
    return nullptr;

  return new DeAmpService(Profile::FromBrowserContext(context)->GetPrefs());
}

content::BrowserContext* DeAmpServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool DeAmpServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace de_amp
