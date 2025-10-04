// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/enabled_state_transition_service_factory.h"

#include <memory>

#include "base/check.h"
#include "base/no_destructor.h"
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/browser/ai_chat/enabled_state_transition_service.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"

namespace ai_chat {

// static
EnabledStateTransitionServiceFactory*
EnabledStateTransitionServiceFactory::GetInstance() {
  static base::NoDestructor<EnabledStateTransitionServiceFactory> instance;
  return instance.get();
}

// static
EnabledStateTransitionService*
EnabledStateTransitionServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  CHECK(context);
  if (!IsAllowedForContext(context, false)) {
    return nullptr;
  }
  return static_cast<EnabledStateTransitionService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

EnabledStateTransitionServiceFactory::EnabledStateTransitionServiceFactory()
    : ProfileKeyedServiceFactory(
          "EnabledStateTransitionService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {
  DependsOn(sidebar::SidebarServiceFactory::GetInstance());
}

EnabledStateTransitionServiceFactory::~EnabledStateTransitionServiceFactory() =
    default;

bool EnabledStateTransitionServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

std::unique_ptr<KeyedService>
EnabledStateTransitionServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<EnabledStateTransitionService>(context);
}

}  // namespace ai_chat
