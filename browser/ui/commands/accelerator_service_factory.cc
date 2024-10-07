// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commands/accelerator_service_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "brave/browser/ui/commands/default_accelerators.h"
#include "brave/components/commands/browser/accelerator_pref_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_context.h"

namespace commands {

// static
AcceleratorServiceFactory* AcceleratorServiceFactory::GetInstance() {
  static base::NoDestructor<AcceleratorServiceFactory> instance;
  return instance.get();
}

// static
AcceleratorService* AcceleratorServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<AcceleratorService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

AcceleratorServiceFactory::AcceleratorServiceFactory()
    : ProfileKeyedServiceFactory(
          "AcceleratorServiceFactory",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              .Build()) {}

AcceleratorServiceFactory::~AcceleratorServiceFactory() = default;

void AcceleratorServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  AcceleratorPrefManager::RegisterProfilePrefs(registry);
}

KeyedService* AcceleratorServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  DCHECK(profile);

  auto [accelerators, system_managed] = GetDefaultAccelerators();
  return new AcceleratorService(profile->GetPrefs(), std::move(accelerators),
                                std::move(system_managed));
}

}  // namespace commands
