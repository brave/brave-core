/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_service_factory.h"

#include <memory>

#include "brave/browser/brave_global_features.h"
#include "brave/browser/extensions/brave_extension_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/chrome_extension_system_factory.h"
#include "extensions/browser/extension_system.h"

namespace extensions {

// static
BraveExtensionServiceFactory* BraveExtensionServiceFactory::GetInstance() {
  static base::NoDestructor<BraveExtensionServiceFactory> instance;
  return instance.get();
}

BraveExtensionServiceFactory::BraveExtensionServiceFactory()
    : ProfileKeyedServiceFactory(
          "BraveExtensionService",
          // Extensions (and their blocklist enforcement) are shared with the
          // original profile; mirror BlocklistFactory's selections.
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              .WithGuest(ProfileSelection::kRedirectedToOriginal)
              .WithAshInternals(ProfileSelection::kRedirectedToOriginal)
              .Build()) {
  DependsOn(ChromeExtensionSystemFactory::GetInstance());
}

BraveExtensionServiceFactory::~BraveExtensionServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveExtensionServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BraveExtensionService>(
      ExtensionSystem::Get(context),
      BraveGlobalFeatures::FromGlobalFeatures(g_browser_process->GetFeatures())
          ->extension_malware_blocklist());
}

bool BraveExtensionServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  // Nothing else fetches this service, so it must be created eagerly for the
  // observer to be live when the list loads.
  return true;
}

}  // namespace extensions
