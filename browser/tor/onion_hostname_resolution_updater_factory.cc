/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/onion_hostname_resolution_updater_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/tor/onion_hostname_resolution_updater.h"
#include "content/public/browser/browser_context.h"

namespace tor {

// static
OnionHostnameResolutionUpdaterFactory*
OnionHostnameResolutionUpdaterFactory::GetInstance() {
  static base::NoDestructor<OnionHostnameResolutionUpdaterFactory> instance;
  return instance.get();
}

OnionHostnameResolutionUpdaterFactory::OnionHostnameResolutionUpdaterFactory()
    : ProfileKeyedServiceFactory(
          "OnionHostnameResolutionUpdater",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .WithGuest(ProfileSelection::kOwnInstance)
              .Build()) {}

OnionHostnameResolutionUpdaterFactory::
    ~OnionHostnameResolutionUpdaterFactory() = default;

std::unique_ptr<KeyedService>
OnionHostnameResolutionUpdaterFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (context->IsTor()) {
    return nullptr;
  }
  return std::make_unique<OnionHostnameResolutionUpdater>(context);
}

bool OnionHostnameResolutionUpdaterFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

}  // namespace tor
