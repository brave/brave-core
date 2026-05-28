// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_chat/tab_activator_factory.h"

#include "brave/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/browser/ui/ai_chat/tab_activator.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace ai_chat {

// static
TabActivatorFactory* TabActivatorFactory::GetInstance() {
  static base::NoDestructor<TabActivatorFactory> instance;
  return instance.get();
}

// static
TabActivator* TabActivatorFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<TabActivator*>(
      GetInstance()->GetServiceForBrowserContext(context, /*create=*/true));
}

TabActivatorFactory::TabActivatorFactory()
    : BrowserContextKeyedServiceFactory(
          "TabActivator",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TabTrackerServiceFactory::GetInstance());
}

TabActivatorFactory::~TabActivatorFactory() = default;

bool TabActivatorFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

content::BrowserContext* TabActivatorFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Off-the-record and other ephemeral profiles don't need their own
  // activator; their tabs are still reachable via the parent profile's
  // BrowserList iteration, and we don't want to install a callback on a
  // separate TabTrackerService instance.
  return Profile::FromBrowserContext(context)->IsRegularProfile() ? context
                                                                  : nullptr;
}

std::unique_ptr<KeyedService>
TabActivatorFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  auto* tab_tracker = TabTrackerServiceFactory::GetForBrowserContext(context);
  return std::make_unique<TabActivator>(profile, tab_tracker);
}

}  // namespace ai_chat
