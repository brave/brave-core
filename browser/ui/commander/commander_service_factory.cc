// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commander/commander_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/ui/commander/commander_service.h"
#include "brave/components/commander/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_context.h"

namespace commander {

// static
CommanderServiceFactory* CommanderServiceFactory::GetInstance() {
  static base::NoDestructor<CommanderServiceFactory> instance;
  return instance.get();
}

// static
CommanderService* CommanderServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<CommanderService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

CommanderServiceFactory::CommanderServiceFactory()
    : ProfileKeyedServiceFactory(
          "CommanderService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .WithGuest(ProfileSelection::kOwnInstance)
              .WithSystem(ProfileSelection::kNone)
              .Build()) {}
CommanderServiceFactory::~CommanderServiceFactory() = default;

std::unique_ptr<KeyedService>
CommanderServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<CommanderService>(
      Profile::FromBrowserContext(context));
}

void CommanderServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kCommanderFrecencies);
}

}  // namespace commander
