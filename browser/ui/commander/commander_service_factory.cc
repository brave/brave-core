// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commander/commander_service_factory.h"

#include "base/memory/singleton.h"
#include "brave/browser/ui/commander/commander_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace commander {

// static
CommanderServiceFactory* CommanderServiceFactory::GetInstance() {
  return base::Singleton<CommanderServiceFactory>::get();
}

// static
CommanderService* CommanderServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<CommanderService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

CommanderServiceFactory::CommanderServiceFactory()
    : ProfileKeyedServiceFactory("CommanderService",
                                 ProfileSelections::BuildForAllProfiles()) {}
CommanderServiceFactory::~CommanderServiceFactory() = default;

KeyedService* CommanderServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new CommanderService(static_cast<Profile*>(context));
}

}  // namespace commander
