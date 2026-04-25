// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tab_tracker_service_factory.h"

#include <memory>

#include "base/check.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace ai_chat {

// static
TabTrackerServiceFactory* TabTrackerServiceFactory::GetInstance() {
  static base::NoDestructor<TabTrackerServiceFactory> instance;
  return instance.get();
}

// static
TabTrackerService* TabTrackerServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  CHECK(context);

  return static_cast<TabTrackerService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

TabTrackerServiceFactory::TabTrackerServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "TabTrackerService",
          BrowserContextDependencyManager::GetInstance()) {}

TabTrackerServiceFactory::~TabTrackerServiceFactory() = default;

bool TabTrackerServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

content::BrowserContext* TabTrackerServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // If AIChat isn't allowed for this context, we don't need a
  // TabTrackerService.
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return context;
}

std::unique_ptr<KeyedService>
TabTrackerServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<TabTrackerService>();
}

}  // namespace ai_chat
