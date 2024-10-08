// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ntp_background/brave_ntp_custom_background_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/ntp_background/brave_ntp_custom_background_service_delegate.h"
#include "brave/components/ntp_background_images/browser/brave_ntp_custom_background_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
ntp_background_images::BraveNTPCustomBackgroundService*
BraveNTPCustomBackgroundServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<ntp_background_images::BraveNTPCustomBackgroundService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BraveNTPCustomBackgroundServiceFactory*
BraveNTPCustomBackgroundServiceFactory::GetInstance() {
  static base::NoDestructor<BraveNTPCustomBackgroundServiceFactory> instance;
  return instance.get();
}

BraveNTPCustomBackgroundServiceFactory::BraveNTPCustomBackgroundServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveNTPCustomBackgroundService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveNTPCustomBackgroundServiceFactory::
    ~BraveNTPCustomBackgroundServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveNTPCustomBackgroundServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  // Custom NTP background is only used in normal profile.
  if (!Profile::FromBrowserContext(context)->IsRegularProfile()) {
    return nullptr;
  }

  return std::make_unique<
      ntp_background_images::BraveNTPCustomBackgroundService>(
      std::make_unique<BraveNTPCustomBackgroundServiceDelegate>(
          Profile::FromBrowserContext(context)));
}
